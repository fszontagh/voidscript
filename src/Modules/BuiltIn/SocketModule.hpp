// SocketModule.hpp
#ifndef MODULES_SOCKETMODULE_HPP
#define MODULES_SOCKETMODULE_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "Modules/BaseModule.hpp"
#include "Symbols/RegistrationMacros.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Modules {

/**
 * @brief Blocking TCP client as the VoidScript class "TcpClient". No external dependency.
 *
 *   TcpClient $c = new TcpClient();
 *   $c->connect("example.com", 80, 5);            // host, port, optional timeout seconds
 *   $c->send("GET / HTTP/1.0\r\nHost: example.com\r\n\r\n");
 *   string $line = $c->recvLine();                // read up to and including one '\n'
 *   string $chunk = $c->recv(4096);               // read up to N bytes ("" at EOF)
 *   $c->close();
 *
 * Talks to protocols curl cannot (SMTP, Redis, custom daemons, health checks). The socket
 * fd is kept per instance, keyed by the framework instance id.
 */
class SocketModule : public BaseModule {
  public:
    SocketModule() {
        setModuleName("Socket");
        setDescription("Blocking TCP client (TcpClient class): connect/send/recv/recvLine/close");
        setBuiltIn(true);
    }

    ~SocketModule() override {
        for (auto & kv : fds_) {
            if (kv.second >= 0) {
                ::close(kv.second);
            }
        }
    }

    void registerFunctions() override {
        REGISTER_CLASS("TcpClient");
        using T = Symbols::Variables::Type;

        REGISTER_METHOD("TcpClient", "__construct", {},
                        [this](Symbols::FunctionArguments & args) { return this->construct(args); },
                        T::CLASS, "Create a TcpClient");

        std::vector<Symbols::FunctionParameterInfo> conn_params = {
            { "host", T::STRING, "Host name or IP" },
            { "port", T::INTEGER, "TCP port" },
            { "timeout", T::INTEGER, "Timeout in seconds for connect and recv (default 10)", true }
        };
        REGISTER_METHOD("TcpClient", "connect", conn_params,
                        [this](Symbols::FunctionArguments & args) { return this->connectFn(args); },
                        T::BOOLEAN, "Connect to host:port");

        std::vector<Symbols::FunctionParameterInfo> send_params = { { "data", T::STRING, "Bytes to send" } };
        REGISTER_METHOD("TcpClient", "send", send_params,
                        [this](Symbols::FunctionArguments & args) { return this->sendFn(args); },
                        T::INTEGER, "Send all the bytes; returns the count");

        std::vector<Symbols::FunctionParameterInfo> recv_params = {
            { "maxBytes", T::INTEGER, "Maximum bytes to read (default 4096)", true }
        };
        REGISTER_METHOD("TcpClient", "recv", recv_params,
                        [this](Symbols::FunctionArguments & args) { return this->recvFn(args); },
                        T::STRING, "Read up to maxBytes; \"\" at end of stream");
        REGISTER_METHOD("TcpClient", "recvLine", {},
                        [this](Symbols::FunctionArguments & args) { return this->recvLineFn(args); },
                        T::STRING, "Read up to and including the next '\\n' (or until EOF)");
        REGISTER_METHOD("TcpClient", "isConnected", {},
                        [this](Symbols::FunctionArguments & args) { return this->isConnectedFn(args); },
                        T::BOOLEAN, "Whether the socket is open");
        REGISTER_METHOD("TcpClient", "close", {},
                        [this](Symbols::FunctionArguments & args) { return this->closeFn(args); },
                        T::NULL_TYPE, "Close the socket");
    }

  private:
    std::unordered_map<long, int> fds_;

    static long idOf(Symbols::FunctionArguments & args, const char * method) {
        if (args.empty() || (args[0] != Symbols::Variables::Type::CLASS && args[0] != Symbols::Variables::Type::OBJECT)) {
            throw std::runtime_error(std::string("TcpClient::") + method + " must be called on a TcpClient instance");
        }
        return Symbols::ValuePtr::instanceId(args[0]);
    }

    int fdFor(Symbols::FunctionArguments & args, const char * method) {
        auto it = fds_.find(idOf(args, method));
        if (it == fds_.end() || it->second < 0) {
            throw std::runtime_error(std::string("TcpClient::") + method + ": not connected - call connect() first");
        }
        return it->second;
    }

    Symbols::ValuePtr construct(Symbols::FunctionArguments & args) {
        (void) idOf(args, "__construct");
        return args[0];
    }

    Symbols::ValuePtr connectFn(Symbols::FunctionArguments & args) {
        if (args.size() < 3 || args[1] != Symbols::Variables::Type::STRING ||
            args[2] != Symbols::Variables::Type::INTEGER) {
            throw std::runtime_error("TcpClient::connect expects (string host, int port [, int timeout])");
        }
        const long        id      = idOf(args, "connect");
        const std::string host    = args[1]->get<std::string>();
        const int         port    = args[2]->get<int>();
        const int         timeout = (args.size() >= 4 && args[3] == Symbols::Variables::Type::INTEGER)
                                        ? args[3]->get<int>()
                                        : 10;

        // Close any previous socket on this instance.
        auto prev = fds_.find(id);
        if (prev != fds_.end() && prev->second >= 0) {
            ::close(prev->second);
            prev->second = -1;
        }

        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo * res = nullptr;
        const std::string portStr = std::to_string(port);
        if (::getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0 || res == nullptr) {
            throw std::runtime_error("TcpClient::connect: cannot resolve '" + host + "'");
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
            throw std::runtime_error("TcpClient::connect: could not connect to " + host + ":" + portStr);
        }
        fds_[id] = fd;
        return Symbols::ValuePtr(true);
    }

    Symbols::ValuePtr sendFn(Symbols::FunctionArguments & args) {
        if (args.size() != 2 || args[1] != Symbols::Variables::Type::STRING) {
            throw std::runtime_error("TcpClient::send expects (string data)");
        }
        const int         fd   = fdFor(args, "send");
        const std::string data = args[1]->get<std::string>();
        size_t            sent = 0;
        while (sent < data.size()) {
            ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
            if (n <= 0) {
                throw std::runtime_error(std::string("TcpClient::send failed: ") + std::strerror(errno));
            }
            sent += static_cast<size_t>(n);
        }
        return Symbols::ValuePtr(static_cast<int>(sent));
    }

    Symbols::ValuePtr recvFn(Symbols::FunctionArguments & args) {
        const int fd  = fdFor(args, "recv");
        int       max = 4096;
        if (args.size() >= 2 && args[1] == Symbols::Variables::Type::INTEGER) {
            max = args[1]->get<int>();
            if (max <= 0) {
                throw std::runtime_error("TcpClient::recv: maxBytes must be positive");
            }
        }
        std::string buf(static_cast<size_t>(max), '\0');
        ssize_t     n = ::recv(fd, &buf[0], static_cast<size_t>(max), 0);
        if (n < 0) {
            throw std::runtime_error(std::string("TcpClient::recv failed: ") + std::strerror(errno));
        }
        buf.resize(static_cast<size_t>(n));
        return Symbols::ValuePtr(buf);
    }

    Symbols::ValuePtr recvLineFn(Symbols::FunctionArguments & args) {
        const int   fd = fdFor(args, "recvLine");
        std::string line;
        char        c;
        while (true) {
            ssize_t n = ::recv(fd, &c, 1, 0);
            if (n <= 0) {
                break;  // EOF or timeout: return what we have
            }
            line.push_back(c);
            if (c == '\n') {
                break;
            }
        }
        return Symbols::ValuePtr(line);
    }

    Symbols::ValuePtr isConnectedFn(Symbols::FunctionArguments & args) {
        auto it = fds_.find(idOf(args, "isConnected"));
        return Symbols::ValuePtr(it != fds_.end() && it->second >= 0);
    }

    Symbols::ValuePtr closeFn(Symbols::FunctionArguments & args) {
        auto it = fds_.find(idOf(args, "close"));
        if (it != fds_.end() && it->second >= 0) {
            ::close(it->second);
            it->second = -1;
        }
        return Symbols::ValuePtr::null();
    }
};

}  // namespace Modules

#endif  // MODULES_SOCKETMODULE_HPP
