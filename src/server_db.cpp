#include "server_db.h"

ServerDB::~ServerDB(void)
{
    Unload();
}

ErrorType ServerDB::Load(const char *filename)
{
    // Unload any previously loaded data
    Unload();

    // Read file
    file.data = LoadFileData(filename, &file.length);
    if (!file.data || !file.length) {
        E_ERROR_RETURN(ErrorType::FileReadFailed, "Failed to read ServerDB file", 0);
    }
    this->filename = filename;

    // Verify fb
    flatbuffers::Verifier verifier(file.data, file.length);
    if (!DB::VerifyServerDBBuffer(verifier)) {
        E_ERROR_RETURN(ErrorType::PancakeVerifyFailed, "Failed to verify ServerDB\n", 0);
    }

    // Read fb
    flat = DB::GetServerDB(file.data);
    return ErrorType::Success;
}

void ServerDB::Unload(void)
{
    delete native;
    UnloadFileData(file.data);
    native = 0;
    flat = 0;
    file = {};
}

ErrorType ServerDB::Save(const char *filename = 0)
{
    if (!filename) filename = this->filename;

    // Write fb
    flatbuffers::FlatBufferBuilder fbb;
    auto newServerDB = flat->Pack(fbb, native);
    DB::FinishServerDBBuffer(fbb, newServerDB);

    // TODO: Save a backup before overwriting in case we corrupt it

    // Write file
    if (!SaveFileData(filename, fbb.GetBufferPointer(), fbb.GetSize())) {
        E_ERROR_RETURN(ErrorType::FileWriteFailed, "Failed to save ServerDB", 0);
    }

    // Reload new fb from file
    E_ERROR_RETURN(Load(filename), "Failed to load ServerDB", 0);
    return ErrorType::Success;
}

size_t ServerDB::Size(void)
{
    size_t size = 0;
    if (flat) {
        size = flat->servers()->size();
    }
    return size;
}

DB::ServerDBT *ServerDB::Data(void)
{
    if (!native) {
        if (flat) {
            native = flat->UnPack();
        } else {
            native = new DB::ServerDBT;
        }
    }
    return native;
}

ErrorType ServerDB::Add(const DB::ServerT &server)
{
    DB::ServerDBT *data = Data();
    data->servers.push_back(std::make_unique<DB::ServerT>(server));
    return ErrorType::Success;
}

ErrorType ServerDB::Replace(size_t index, const DB::ServerT &server)
{
    DB::ServerDBT *data = Data();
    if (index < data->servers.size()) {
        data->servers[index] = std::make_unique<DB::ServerT>(server);
    }
    return ErrorType::Success;
}

ErrorType ServerDB::Delete(size_t index)
{
    DB::ServerDBT *data = Data();
    if (index < data->servers.size()) {
        data->servers.erase(data->servers.begin() + index);
    }
    return ErrorType::Success;
}