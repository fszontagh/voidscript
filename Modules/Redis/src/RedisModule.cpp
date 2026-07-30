#include "RedisModule.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

namespace {

// Read one more chunk from the socket into the connection buffer. Returns false at EOF.
bool fillMore(int fd, std::string & buf) {
    char    tmp[8192];
    ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
    if (n <= 0) {
        return false;
    }
    buf.append(tmp, static_cast<size_t>(n));
    return true;
}

}  // namespace

RedisModule::~RedisModule() {
    for (auto & kv : conns_) {
        if (kv.second.fd >= 0) {
            ::close(kv.second.fd);
        }
    }
}

RedisModule::Conn & RedisModule::connFor(FunctionArguments & args, const char * method) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error(std::string("Redis::") + method + " must be called on a Redis instance");
    }
    const long id = Symbols::ValuePtr::instanceId(args[0]);
    auto       it = conns_.find(id);
    if (it == conns_.end() || it->second.fd < 0) {
        throw std::runtime_error(std::string("Redis::") + method + ": not connected - call connect() first");
    }
    return it->second;
}

// --- RESP read helpers (operate on a Conn's buffered fd) --------------------------------

namespace {

std::string readLine(RedisModule::Conn & c) {
    while (true) {
        size_t nl = c.buf.find("\r\n", c.pos);
        if (nl != std::string::npos) {
            std::string line = c.buf.substr(c.pos, nl - c.pos);
            c.pos            = nl + 2;
            return line;
        }
        if (!fillMore(c.fd, c.buf)) {
            throw std::runtime_error("Redis: connection closed while reading");
        }
    }
}

std::string readN(RedisModule::Conn & c, size_t n) {
    while (c.buf.size() - c.pos < n + 2) {  // n payload + trailing \r\n
        if (!fillMore(c.fd, c.buf)) {
            throw std::runtime_error("Redis: connection closed while reading bulk");
        }
    }
    std::string out = c.buf.substr(c.pos, n);
    c.pos += n + 2;  // skip payload and \r\n
    return out;
}

// Parse a single RESP reply into a ValuePtr. Errors ('-') throw. $-1 / *-1 -> null.
Symbols::ValuePtr readReply(RedisModule::Conn & c) {
    std::string line = readLine(c);
    if (line.empty()) {
        throw std::runtime_error("Redis: empty reply");
    }
    const char type = line[0];
    const std::string rest = line.substr(1);
    switch (type) {
        case '+':
            return Symbols::ValuePtr(rest);
        case '-':
            throw std::runtime_error("Redis error: " + rest);
        case ':':
            return Symbols::ValuePtr(static_cast<int>(std::stoll(rest)));
        case '$': {
            long len = std::stol(rest);
            if (len < 0) {
                return Symbols::ValuePtr::null();
            }
            return Symbols::ValuePtr(readN(c, static_cast<size_t>(len)));
        }
        case '*': {
            long count = std::stol(rest);
            if (count < 0) {
                return Symbols::ValuePtr::null();
            }
            Symbols::ObjectMap arr;
            for (long i = 0; i < count; ++i) {
                arr[std::to_string(i)] = readReply(c);
            }
            return Symbols::ValuePtr(arr);
        }
        default:
            throw std::runtime_error(std::string("Redis: unexpected reply type '") + type + "'");
    }
}

// Encode a command as a RESP array of bulk strings and send it.
void sendCommand(int fd, const std::vector<std::string> & parts) {
    std::string out = "*" + std::to_string(parts.size()) + "\r\n";
    for (const auto & p : parts) {
        out += "$" + std::to_string(p.size()) + "\r\n" + p + "\r\n";
    }
    size_t sent = 0;
    while (sent < out.size()) {
        ssize_t n = ::send(fd, out.data() + sent, out.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            throw std::runtime_error(std::string("Redis: send failed: ") + std::strerror(errno));
        }
        sent += static_cast<size_t>(n);
    }
}

Symbols::ValuePtr call(RedisModule::Conn & c, const std::vector<std::string> & parts) {
    sendCommand(c.fd, parts);
    return readReply(c);
}

std::string argStr(FunctionArguments & args, size_t i) {
    return args[i]->toString();
}

}  // namespace

void RedisModule::registerFunctions() {
    REGISTER_CLASS(this->name());
    using T = Symbols::Variables::Type;

    REGISTER_METHOD(this->name(), "__construct", {},
                    [this](FunctionArguments & args) { return this->construct(args); },
                    T::CLASS, "Create a Redis client");

    std::vector<Symbols::FunctionParameterInfo> conn_params = {
        { "host", T::STRING, "Host (default 127.0.0.1)", true },
        { "port", T::INTEGER, "Port (default 6379)", true },
        { "timeout", T::INTEGER, "Timeout seconds (default 10)", true }
    };
    REGISTER_METHOD(this->name(), "connect", conn_params,
                    [this](FunctionArguments & args) { return this->connectFn(args); },
                    T::BOOLEAN, "Connect to a Redis server");
    REGISTER_METHOD(this->name(), "isConnected", {},
                    [this](FunctionArguments & args) { return this->isConnectedFn(args); },
                    T::BOOLEAN, "Whether connected");
    REGISTER_METHOD(this->name(), "close", {},
                    [this](FunctionArguments & args) { return this->closeFn(args); },
                    T::NULL_TYPE, "Close the connection");

    std::vector<Symbols::FunctionParameterInfo> kv = { { "key", T::STRING, "Key" }, { "value", T::STRING, "Value" } };
    std::vector<Symbols::FunctionParameterInfo> k  = { { "key", T::STRING, "Key" } };
    std::vector<Symbols::FunctionParameterInfo> ke = { { "key", T::STRING, "Key" }, { "seconds", T::INTEGER, "TTL seconds" } };
    std::vector<Symbols::FunctionParameterInfo> cmd = { { "args", T::OBJECT, "Array of command words" } };

    REGISTER_METHOD(this->name(), "set", kv, [this](FunctionArguments & args) { return this->setFn(args); },
                    T::BOOLEAN, "SET key value");
    REGISTER_METHOD(this->name(), "get", k, [this](FunctionArguments & args) { return this->getFn(args); },
                    T::STRING, "GET key (null if missing)");
    REGISTER_METHOD(this->name(), "del", k, [this](FunctionArguments & args) { return this->delFn(args); },
                    T::INTEGER, "DEL key; returns number removed");
    REGISTER_METHOD(this->name(), "exists", k, [this](FunctionArguments & args) { return this->existsFn(args); },
                    T::BOOLEAN, "EXISTS key");
    REGISTER_METHOD(this->name(), "incr", k, [this](FunctionArguments & args) { return this->incrFn(args); },
                    T::INTEGER, "INCR key; returns the new value");
    REGISTER_METHOD(this->name(), "expire", ke, [this](FunctionArguments & args) { return this->expireFn(args); },
                    T::BOOLEAN, "EXPIRE key seconds");
    REGISTER_METHOD(this->name(), "ping", {}, [this](FunctionArguments & args) { return this->pingFn(args); },
                    T::STRING, "PING (returns PONG)");
    REGISTER_METHOD(this->name(), "command", cmd, [this](FunctionArguments & args) { return this->commandFn(args); },
                    T::OBJECT, "Run an arbitrary command from an array of words; returns the raw reply");
}

Symbols::ValuePtr RedisModule::construct(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("Redis::__construct must be called on a Redis instance");
    }
    return args[0];
}

Symbols::ValuePtr RedisModule::connectFn(FunctionArguments & args) {
    if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
        throw std::runtime_error("Redis::connect must be called on a Redis instance");
    }
    const long        id      = Symbols::ValuePtr::instanceId(args[0]);
    const std::string host    = (args.size() >= 2 && args[1] == Symbols::Variables::Type::STRING)
                                    ? args[1]->get<std::string>()
                                    : "127.0.0.1";
    const int         port    = (args.size() >= 3 && args[2] == Symbols::Variables::Type::INTEGER)
                                    ? args[2]->get<int>()
                                    : 6379;
    const int         timeout = (args.size() >= 4 && args[3] == Symbols::Variables::Type::INTEGER)
                                    ? args[3]->get<int>()
                                    : 10;

    auto prev = conns_.find(id);
    if (prev != conns_.end() && prev->second.fd >= 0) {
        ::close(prev->second.fd);
    }

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo * res     = nullptr;
    const std::string portStr = std::to_string(port);
    if (::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0 || res == nullptr) {
        throw std::runtime_error("Redis::connect: cannot resolve '" + host + "'");
    }
    int fd = -1;
    for (struct addrinfo * p = res; p != nullptr; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) {
            continue;
        }
        struct timeval tv;
        tv.tv_sec  = timeout;
        tv.tv_usec = 0;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(res);
    if (fd < 0) {
        throw std::runtime_error("Redis::connect: could not connect to " + host + ":" + portStr);
    }
    Conn c;
    c.fd        = fd;
    conns_[id]  = std::move(c);
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr RedisModule::isConnectedFn(FunctionArguments & args) {
    if (args.empty()) {
        return Symbols::ValuePtr(false);
    }
    auto it = conns_.find(Symbols::ValuePtr::instanceId(args[0]));
    return Symbols::ValuePtr(it != conns_.end() && it->second.fd >= 0);
}

Symbols::ValuePtr RedisModule::closeFn(FunctionArguments & args) {
    auto it = conns_.find(Symbols::ValuePtr::instanceId(args[0]));
    if (it != conns_.end() && it->second.fd >= 0) {
        ::close(it->second.fd);
        it->second.fd = -1;
    }
    return Symbols::ValuePtr::null();
}

Symbols::ValuePtr RedisModule::setFn(FunctionArguments & args) {
    if (args.size() != 3) {
        throw std::runtime_error("Redis::set expects (string key, string value)");
    }
    Conn & c = connFor(args, "set");
    call(c, { "SET", argStr(args, 1), argStr(args, 2) });
    return Symbols::ValuePtr(true);
}

Symbols::ValuePtr RedisModule::getFn(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Redis::get expects (string key)");
    }
    Conn & c = connFor(args, "get");
    return call(c, { "GET", argStr(args, 1) });
}

Symbols::ValuePtr RedisModule::delFn(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Redis::del expects (string key)");
    }
    Conn & c = connFor(args, "del");
    return call(c, { "DEL", argStr(args, 1) });
}

Symbols::ValuePtr RedisModule::existsFn(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Redis::exists expects (string key)");
    }
    Conn &            c = connFor(args, "exists");
    Symbols::ValuePtr r = call(c, { "EXISTS", argStr(args, 1) });
    return Symbols::ValuePtr(r->get<int>() > 0);
}

Symbols::ValuePtr RedisModule::incrFn(FunctionArguments & args) {
    if (args.size() != 2) {
        throw std::runtime_error("Redis::incr expects (string key)");
    }
    Conn & c = connFor(args, "incr");
    return call(c, { "INCR", argStr(args, 1) });
}

Symbols::ValuePtr RedisModule::expireFn(FunctionArguments & args) {
    if (args.size() != 3 || args[2] != Symbols::Variables::Type::INTEGER) {
        throw std::runtime_error("Redis::expire expects (string key, int seconds)");
    }
    Conn &            c = connFor(args, "expire");
    Symbols::ValuePtr r = call(c, { "EXPIRE", argStr(args, 1), std::to_string(args[2]->get<int>()) });
    return Symbols::ValuePtr(r->get<int>() > 0);
}

Symbols::ValuePtr RedisModule::pingFn(FunctionArguments & args) {
    Conn & c = connFor(args, "ping");
    return call(c, { "PING" });
}

Symbols::ValuePtr RedisModule::commandFn(FunctionArguments & args) {
    if (args.size() != 2 || (args[1] != Symbols::Variables::Type::OBJECT && args[1] != Symbols::Variables::Type::CLASS)) {
        throw std::runtime_error("Redis::command expects (array words)");
    }
    Conn &                   c   = connFor(args, "command");
    const Symbols::ObjectMap & m = args[1]->get<Symbols::ObjectMap>();
    std::vector<std::string> parts;
    for (size_t i = 0;; ++i) {
        auto it = m.find(std::to_string(i));
        if (it == m.end()) {
            break;
        }
        parts.push_back(it->second->toString());
    }
    if (parts.empty()) {
        throw std::runtime_error("Redis::command: empty command");
    }
    return call(c, parts);
}

}  // namespace Modules
