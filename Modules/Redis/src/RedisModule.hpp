#ifndef REDIS_MODULE_HPP
#define REDIS_MODULE_HPP

#include <string>
#include <unordered_map>

#include "Modules/BaseModule.hpp"
#include "Symbols/Value.hpp"

namespace Modules {

// VoidScript class "Redis": a minimal RESP client over a raw TCP socket (no hiredis).
//   Redis $r = new Redis();  $r->connect("127.0.0.1", 6379);
//   $r->set("k", "v");  string $v = $r->get("k");  $r->del("k");
//   auto $reply = $r->command(["LPUSH", "list", "x"]);   // raw command escape hatch
class RedisModule : public BaseModule {
  public:
    RedisModule() {
        setModuleName("Redis");
        setDescription("Minimal Redis client (RESP over raw sockets): connect/set/get/del/exists/incr/expire/ping/command");
    }
    ~RedisModule() override;
    void registerFunctions() override;

    // Public so the RESP read/write helpers in the .cpp can operate on a connection.
    struct Conn {
        int         fd  = -1;
        std::string buf;
        size_t      pos = 0;
    };

  private:
    std::unordered_map<long, Conn> conns_;

    Conn &            connFor(FunctionArguments & args, const char * method);
    Symbols::ValuePtr construct(FunctionArguments & args);
    Symbols::ValuePtr connectFn(FunctionArguments & args);
    Symbols::ValuePtr closeFn(FunctionArguments & args);
    Symbols::ValuePtr isConnectedFn(FunctionArguments & args);
    Symbols::ValuePtr setFn(FunctionArguments & args);
    Symbols::ValuePtr getFn(FunctionArguments & args);
    Symbols::ValuePtr delFn(FunctionArguments & args);
    Symbols::ValuePtr existsFn(FunctionArguments & args);
    Symbols::ValuePtr incrFn(FunctionArguments & args);
    Symbols::ValuePtr expireFn(FunctionArguments & args);
    Symbols::ValuePtr pingFn(FunctionArguments & args);
    Symbols::ValuePtr commandFn(FunctionArguments & args);
};

}  // namespace Modules
#endif  // REDIS_MODULE_HPP
