#pragma once
#include "chat.h"
#include "controller.h"
#include "fbs.h"
#include "dlb_types.h"
#include "servers_generated.h"

struct World;

struct NetClient {
    char     serverHost       [HOSTNAME_LENGTH_MAX]{};
    size_t   serverHostLength {};
    uint16_t serverPort       {};
    size_t   usernameLength   {};
    char     username         [USERNAME_LENGTH_MAX]{};
    size_t   passwordLength   {};
    char     password         [PASSWORD_LENGTH_MAX]{};

    uint32_t connectionToken  {};
    ENetHost *client          {};
    ENetPeer *server          {};
    World    *serverWorld     {};
    double   lastInputSentAt  {};

    uint32_t inputSeq         {};  // seq # of input last sent to server
    RingBuffer<InputSample,   CL_INPUT_HISTORY> inputHistory {};
    RingBuffer<WorldSnapshot, CL_WORLD_HISTORY> worldHistory {};

    NetClient                     (void);
    ~NetClient                    (void);
    ErrorType OpenSocket          (void);
    ErrorType Connect             (const char *serverHost, unsigned short serverPort, const char *user, const char *password);
    ErrorType SendChatMessage     (const char *message, size_t messageLength);
    ErrorType SendSlotClick       (int slot, bool doubleClicked);
    ErrorType SendSlotScroll      (int slot, int scrollY);
    ErrorType SendSlotDrop        (int slot, uint32_t count);
    ErrorType SendTileInteract    (float worldX, float worldY);
    ErrorType SendPlayerInput     (void);
    void      PredictPlayer       (void);
    void      ReconcilePlayer     (void);
    ErrorType Receive             (void);
    bool      IsConnecting        (void) const;
    bool      IsConnected         (void) const;
    bool      IsDisconnected      (void) const;
    void      Disconnect          (void);
    void      CloseSocket         (void);
    bool      ConnectedAndSpawned (void) const;

    // TODO: Should the base multiplayer menu own this data?
    struct ServerDB {
        FBS_Buffer          file {};  // raw file buffer
        const DB::ServerDB *flat {};  // read-only flatbuffer

        ~ServerDB(void) {
            Unload();
        }

        ErrorType Load(const char *filename) {
            // Unload any previously loaded data
            Unload();

            // Read file
            file.data = LoadFileData(filename, &file.length);
            if (!file.data || !file.length) {
                E_CHECKMSG(ErrorType::FileReadFailed, "Failed to read ServerDB file");
            }
            this->filename = filename;

            // Verify fb
            flatbuffers::Verifier verifier(file.data, file.length);
            if (!DB::VerifyServerDBBuffer(verifier)) {
                E_CHECKMSG(ErrorType::PancakeVerifyFailed, "Failed to verify ServerDB\n");
            }

            // Read fb
            flat = DB::GetServerDB(file.data);
            return ErrorType::Success;
        }

        void Unload(void) {
            delete native;
            UnloadFileData(file.data);
            native = 0;
            flat = 0;
            file = {};
        }

        ErrorType Save(const char *filename = 0)
        {
            if (!filename) filename = this->filename;

            // Write fb
            flatbuffers::FlatBufferBuilder fbb;
            auto newServerDB = flat->Pack(fbb, native);
            DB::FinishServerDBBuffer(fbb, newServerDB);

            // TODO: Save a backup before overwriting in case we corrupt it

            // Write file
            if (!SaveFileData(filename, fbb.GetBufferPointer(), fbb.GetSize())) {
                E_CHECKMSG(ErrorType::FileWriteFailed, "Failed to save ServerDB");
            }

            // Reload new fb from file
            E_CHECKMSG(Load(filename), "Failed to load ServerDB");
            return ErrorType::Success;
        }

        size_t Size(void) {
            size_t size = 0;
            if (flat) {
                size = flat->servers()->size();
            }
            return size;
        }

        DB::ServerDBT *Data(void) {
            if (!native) {
                if (flat) {
                    native = flat->UnPack();
                } else {
                    native = new DB::ServerDBT;
                }
            }
            return native;
        }

        ErrorType Add(const DB::ServerT &server) {
            DB::ServerDBT *data = Data();
            data->servers.push_back(std::make_unique<DB::ServerT>(server));
            return ErrorType::Success;
        }

        ErrorType Replace(size_t index, const DB::ServerT &server) {
            DB::ServerDBT *data = Data();
            if (index < data->servers.size()) {
                data->servers[index] = std::make_unique<DB::ServerT>(server);
            }
            return ErrorType::Success;
        }

        ErrorType Delete(size_t index) {
            DB::ServerDBT *data = Data();
            if (index < data->servers.size()) {
                data->servers.erase(data->servers.begin() + index);
            }
            return ErrorType::Success;
        }

    private:
        const char *LOG_SRC = "ServerDB";
        const char    *filename {};  // name of last loaded file
        DB::ServerDBT *native   {};  // mutable native data structure
    } server_db{};

private:
    const char *LOG_SRC = "NetClient";
    NetMessage tempMsg {};
    ENetBuffer rawPacket {};

    ErrorType   SaveDefaultServerDB (const char *filename);
    ErrorType   SendRaw             (const void *data, size_t size);
    ErrorType   SendMsg             (NetMessage &message);
    ErrorType   Auth                (void);
    void        ProcessMsg          (ENetPacket &packet);
    const char *ServerStateString   (void);
};