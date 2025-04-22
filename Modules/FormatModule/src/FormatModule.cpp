// CurlModule implementation: HTTP GET and POST using libcurl
#include "FormatModule.hpp"

#include <fmt/args.h>

#include <iostream>
#include <string>
#include <vector>

#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

// Register module functions
void Modules::FormatModule::registerModule() {
    auto & mgr = Modules::ModuleManager::instance();
    mgr.registerFunction(
        "format",
        [](FuncionArguments & args) -> Symbols::Value {
            if (args.size() < 2) {
                throw std::runtime_error("2 arguments required");
            }
            if (args.front().getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("First parameter need to be string");
            }

            auto         _args  = args;
            const auto & format = Symbols::Value::to_string(_args.front());
            _args.erase(_args.begin());

            fmt::dynamic_format_arg_store<fmt::format_context> store;

            for (const auto & arg : _args) {
                store.push_back(Symbols::Value::to_string(arg));
            }
            std::cout << fmt::vformat(format, store);
            return Symbols::Value::makeNull(Symbols::Variables::Type::NULL_TYPE);
        },
        Symbols::Variables::Type::NULL_TYPE);

    mgr.registerFunction(
        "sformat",
        [](FuncionArguments & args) -> Symbols::Value {
            if (args.size() < 2) {
                throw std::runtime_error("2 arguments required");
            }
            if (args.front().getType() != Symbols::Variables::Type::STRING) {
                throw std::runtime_error("First parameter need to be string");
            }

            auto         _args  = args;
            const auto & format = Symbols::Value::to_string(_args.front());
            _args.erase(_args.begin());

            fmt::dynamic_format_arg_store<fmt::format_context> store;

            for (const auto & arg : _args) {
                store.push_back(Symbols::Value::to_string(arg));
            }
            return Symbols::Value(fmt::vformat(format, store));
        },
        Symbols::Variables::Type::NULL_TYPE);
}
