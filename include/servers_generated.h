// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SERVERS_DB_H_
#define FLATBUFFERS_GENERATED_SERVERS_DB_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 2 &&
              FLATBUFFERS_VERSION_MINOR == 0 &&
              FLATBUFFERS_VERSION_REVISION == 8,
             "Non-compatible flatbuffers version included");

namespace DB {

struct Server;
struct ServerBuilder;
struct ServerT;

struct ServerDB;
struct ServerDBBuilder;
struct ServerDBT;

struct ServerT : public flatbuffers::NativeTable {
  typedef Server TableType;
  static FLATBUFFERS_CONSTEXPR_CPP11 const char *GetFullyQualifiedName() {
    return "DB.ServerT";
  }
  std::string desc{};
  std::string host{};
  uint16_t port = 0;
  std::string user{};
  std::string pass{};
};

struct Server FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ServerT NativeTableType;
  typedef ServerBuilder Builder;
  static FLATBUFFERS_CONSTEXPR_CPP11 const char *GetFullyQualifiedName() {
    return "DB.Server";
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_DESC = 4,
    VT_HOST = 6,
    VT_PORT = 8,
    VT_USER = 10,
    VT_PASS = 12
  };
  const flatbuffers::String *desc() const {
    return GetPointer<const flatbuffers::String *>(VT_DESC);
  }
  flatbuffers::String *mutable_desc() {
    return GetPointer<flatbuffers::String *>(VT_DESC);
  }
  bool KeyCompareLessThan(const Server *o) const {
    return *desc() < *o->desc();
  }
  int KeyCompareWithValue(const char *_desc) const {
    return strcmp(desc()->c_str(), _desc);
  }
  const flatbuffers::String *host() const {
    return GetPointer<const flatbuffers::String *>(VT_HOST);
  }
  flatbuffers::String *mutable_host() {
    return GetPointer<flatbuffers::String *>(VT_HOST);
  }
  uint16_t port() const {
    return GetField<uint16_t>(VT_PORT, 0);
  }
  bool mutate_port(uint16_t _port = 0) {
    return SetField<uint16_t>(VT_PORT, _port, 0);
  }
  const flatbuffers::String *user() const {
    return GetPointer<const flatbuffers::String *>(VT_USER);
  }
  flatbuffers::String *mutable_user() {
    return GetPointer<flatbuffers::String *>(VT_USER);
  }
  const flatbuffers::String *pass() const {
    return GetPointer<const flatbuffers::String *>(VT_PASS);
  }
  flatbuffers::String *mutable_pass() {
    return GetPointer<flatbuffers::String *>(VT_PASS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_DESC) &&
           verifier.VerifyString(desc()) &&
           VerifyOffset(verifier, VT_HOST) &&
           verifier.VerifyString(host()) &&
           VerifyField<uint16_t>(verifier, VT_PORT, 2) &&
           VerifyOffset(verifier, VT_USER) &&
           verifier.VerifyString(user()) &&
           VerifyOffset(verifier, VT_PASS) &&
           verifier.VerifyString(pass()) &&
           verifier.EndTable();
  }
  ServerT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(ServerT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<Server> Pack(flatbuffers::FlatBufferBuilder &_fbb, const ServerT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct ServerBuilder {
  typedef Server Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_desc(flatbuffers::Offset<flatbuffers::String> desc) {
    fbb_.AddOffset(Server::VT_DESC, desc);
  }
  void add_host(flatbuffers::Offset<flatbuffers::String> host) {
    fbb_.AddOffset(Server::VT_HOST, host);
  }
  void add_port(uint16_t port) {
    fbb_.AddElement<uint16_t>(Server::VT_PORT, port, 0);
  }
  void add_user(flatbuffers::Offset<flatbuffers::String> user) {
    fbb_.AddOffset(Server::VT_USER, user);
  }
  void add_pass(flatbuffers::Offset<flatbuffers::String> pass) {
    fbb_.AddOffset(Server::VT_PASS, pass);
  }
  explicit ServerBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Server> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Server>(end);
    fbb_.Required(o, Server::VT_DESC);
    return o;
  }
};

inline flatbuffers::Offset<Server> CreateServer(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> desc = 0,
    flatbuffers::Offset<flatbuffers::String> host = 0,
    uint16_t port = 0,
    flatbuffers::Offset<flatbuffers::String> user = 0,
    flatbuffers::Offset<flatbuffers::String> pass = 0) {
  ServerBuilder builder_(_fbb);
  builder_.add_pass(pass);
  builder_.add_user(user);
  builder_.add_host(host);
  builder_.add_desc(desc);
  builder_.add_port(port);
  return builder_.Finish();
}

inline flatbuffers::Offset<Server> CreateServerDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *desc = nullptr,
    const char *host = nullptr,
    uint16_t port = 0,
    const char *user = nullptr,
    const char *pass = nullptr) {
  auto desc__ = desc ? _fbb.CreateString(desc) : 0;
  auto host__ = host ? _fbb.CreateString(host) : 0;
  auto user__ = user ? _fbb.CreateString(user) : 0;
  auto pass__ = pass ? _fbb.CreateString(pass) : 0;
  return DB::CreateServer(
      _fbb,
      desc__,
      host__,
      port,
      user__,
      pass__);
}

flatbuffers::Offset<Server> CreateServer(flatbuffers::FlatBufferBuilder &_fbb, const ServerT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

struct ServerDBT : public flatbuffers::NativeTable {
  typedef ServerDB TableType;
  static FLATBUFFERS_CONSTEXPR_CPP11 const char *GetFullyQualifiedName() {
    return "DB.ServerDBT";
  }
  std::vector<std::unique_ptr<DB::ServerT>> servers{};
  ServerDBT() = default;
  ServerDBT(const ServerDBT &o);
  ServerDBT(ServerDBT&&) FLATBUFFERS_NOEXCEPT = default;
  ServerDBT &operator=(ServerDBT o) FLATBUFFERS_NOEXCEPT;
};

struct ServerDB FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ServerDBT NativeTableType;
  typedef ServerDBBuilder Builder;
  static FLATBUFFERS_CONSTEXPR_CPP11 const char *GetFullyQualifiedName() {
    return "DB.ServerDB";
  }
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SERVERS = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<DB::Server>> *servers() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<DB::Server>> *>(VT_SERVERS);
  }
  flatbuffers::Vector<flatbuffers::Offset<DB::Server>> *mutable_servers() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<DB::Server>> *>(VT_SERVERS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SERVERS) &&
           verifier.VerifyVector(servers()) &&
           verifier.VerifyVectorOfTables(servers()) &&
           verifier.EndTable();
  }
  ServerDBT *UnPack(const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  void UnPackTo(ServerDBT *_o, const flatbuffers::resolver_function_t *_resolver = nullptr) const;
  static flatbuffers::Offset<ServerDB> Pack(flatbuffers::FlatBufferBuilder &_fbb, const ServerDBT* _o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);
};

struct ServerDBBuilder {
  typedef ServerDB Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_servers(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<DB::Server>>> servers) {
    fbb_.AddOffset(ServerDB::VT_SERVERS, servers);
  }
  explicit ServerDBBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<ServerDB> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ServerDB>(end);
    return o;
  }
};

inline flatbuffers::Offset<ServerDB> CreateServerDB(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<DB::Server>>> servers = 0) {
  ServerDBBuilder builder_(_fbb);
  builder_.add_servers(servers);
  return builder_.Finish();
}

inline flatbuffers::Offset<ServerDB> CreateServerDBDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    std::vector<flatbuffers::Offset<DB::Server>> *servers = nullptr) {
  auto servers__ = servers ? _fbb.CreateVectorOfSortedTables<DB::Server>(servers) : 0;
  return DB::CreateServerDB(
      _fbb,
      servers__);
}

flatbuffers::Offset<ServerDB> CreateServerDB(flatbuffers::FlatBufferBuilder &_fbb, const ServerDBT *_o, const flatbuffers::rehasher_function_t *_rehasher = nullptr);

inline ServerT *Server::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  auto _o = std::unique_ptr<ServerT>(new ServerT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void Server::UnPackTo(ServerT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = desc(); if (_e) _o->desc = _e->str(); }
  { auto _e = host(); if (_e) _o->host = _e->str(); }
  { auto _e = port(); _o->port = _e; }
  { auto _e = user(); if (_e) _o->user = _e->str(); }
  { auto _e = pass(); if (_e) _o->pass = _e->str(); }
}

inline flatbuffers::Offset<Server> Server::Pack(flatbuffers::FlatBufferBuilder &_fbb, const ServerT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return CreateServer(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<Server> CreateServer(flatbuffers::FlatBufferBuilder &_fbb, const ServerT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const ServerT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _desc = _fbb.CreateString(_o->desc);
  auto _host = _o->host.empty() ? 0 : _fbb.CreateString(_o->host);
  auto _port = _o->port;
  auto _user = _o->user.empty() ? 0 : _fbb.CreateString(_o->user);
  auto _pass = _o->pass.empty() ? 0 : _fbb.CreateString(_o->pass);
  return DB::CreateServer(
      _fbb,
      _desc,
      _host,
      _port,
      _user,
      _pass);
}

inline ServerDBT::ServerDBT(const ServerDBT &o) {
  servers.reserve(o.servers.size());
  for (const auto &servers_ : o.servers) { servers.emplace_back((servers_) ? new DB::ServerT(*servers_) : nullptr); }
}

inline ServerDBT &ServerDBT::operator=(ServerDBT o) FLATBUFFERS_NOEXCEPT {
  std::swap(servers, o.servers);
  return *this;
}

inline ServerDBT *ServerDB::UnPack(const flatbuffers::resolver_function_t *_resolver) const {
  auto _o = std::unique_ptr<ServerDBT>(new ServerDBT());
  UnPackTo(_o.get(), _resolver);
  return _o.release();
}

inline void ServerDB::UnPackTo(ServerDBT *_o, const flatbuffers::resolver_function_t *_resolver) const {
  (void)_o;
  (void)_resolver;
  { auto _e = servers(); if (_e) { _o->servers.resize(_e->size()); for (flatbuffers::uoffset_t _i = 0; _i < _e->size(); _i++) { if(_o->servers[_i]) { _e->Get(_i)->UnPackTo(_o->servers[_i].get(), _resolver); } else { _o->servers[_i] = std::unique_ptr<DB::ServerT>(_e->Get(_i)->UnPack(_resolver)); }; } } else { _o->servers.resize(0); } }
}

inline flatbuffers::Offset<ServerDB> ServerDB::Pack(flatbuffers::FlatBufferBuilder &_fbb, const ServerDBT* _o, const flatbuffers::rehasher_function_t *_rehasher) {
  return CreateServerDB(_fbb, _o, _rehasher);
}

inline flatbuffers::Offset<ServerDB> CreateServerDB(flatbuffers::FlatBufferBuilder &_fbb, const ServerDBT *_o, const flatbuffers::rehasher_function_t *_rehasher) {
  (void)_rehasher;
  (void)_o;
  struct _VectorArgs { flatbuffers::FlatBufferBuilder *__fbb; const ServerDBT* __o; const flatbuffers::rehasher_function_t *__rehasher; } _va = { &_fbb, _o, _rehasher}; (void)_va;
  auto _servers = _o->servers.size() ? _fbb.CreateVector<flatbuffers::Offset<DB::Server>> (_o->servers.size(), [](size_t i, _VectorArgs *__va) { return CreateServer(*__va->__fbb, __va->__o->servers[i].get(), __va->__rehasher); }, &_va ) : 0;
  return DB::CreateServerDB(
      _fbb,
      _servers);
}

inline const DB::ServerDB *GetServerDB(const void *buf) {
  return flatbuffers::GetRoot<DB::ServerDB>(buf);
}

inline const DB::ServerDB *GetSizePrefixedServerDB(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<DB::ServerDB>(buf);
}

inline ServerDB *GetMutableServerDB(void *buf) {
  return flatbuffers::GetMutableRoot<ServerDB>(buf);
}

inline DB::ServerDB *GetMutableSizePrefixedServerDB(void *buf) {
  return flatbuffers::GetMutableSizePrefixedRoot<DB::ServerDB>(buf);
}

inline const char *ServerDBIdentifier() {
  return "SERV";
}

inline bool ServerDBBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, ServerDBIdentifier());
}

inline bool SizePrefixedServerDBBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, ServerDBIdentifier(), true);
}

inline bool VerifyServerDBBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<DB::ServerDB>(ServerDBIdentifier());
}

inline bool VerifySizePrefixedServerDBBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<DB::ServerDB>(ServerDBIdentifier());
}

inline const char *ServerDBExtension() {
  return "svr";
}

inline void FinishServerDBBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<DB::ServerDB> root) {
  fbb.Finish(root, ServerDBIdentifier());
}

inline void FinishSizePrefixedServerDBBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<DB::ServerDB> root) {
  fbb.FinishSizePrefixed(root, ServerDBIdentifier());
}

inline std::unique_ptr<DB::ServerDBT> UnPackServerDB(
    const void *buf,
    const flatbuffers::resolver_function_t *res = nullptr) {
  return std::unique_ptr<DB::ServerDBT>(GetServerDB(buf)->UnPack(res));
}

inline std::unique_ptr<DB::ServerDBT> UnPackSizePrefixedServerDB(
    const void *buf,
    const flatbuffers::resolver_function_t *res = nullptr) {
  return std::unique_ptr<DB::ServerDBT>(GetSizePrefixedServerDB(buf)->UnPack(res));
}

}  // namespace DB

#endif  // FLATBUFFERS_GENERATED_SERVERS_DB_H_
