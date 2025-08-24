// JsonModule.hpp
#ifndef MODULES_JSONMODULE_HPP
#define MODULES_JSONMODULE_HPP

#include <cctype>
#include <stdexcept>
#include <string>
#include <variant>

#include "../../json.hpp"
#include "Modules/BaseModule.hpp"
#include "Modules/BuiltIn/JsonConverters.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Symbols/RegistrationMacros.hpp"

namespace Modules {

/**
 * @brief Module providing JSON encode/decode functions.
 *   json_encode(value) -> string
 *   json_decode(string) -> object/value
 */
class JsonModule : public BaseModule {
  public:
    JsonModule() {
        setModuleName("Json");
        setDescription("Provides JSON serialization and deserialization functions for converting between VoidScript objects and JSON strings");
    }

    void registerFunctions() override {
        std::vector<Symbols::FunctionParameterInfo> params = {
            { "object", Symbols::Variables::Type::OBJECT, "The object / array to serialize", false, false },
        };

        REGISTER_FUNCTION("json_encode", Symbols::Variables::Type::STRING, params, "Serialize a value to JSON string",
                          [](const Symbols::FunctionArguments & args) -> Symbols::ValuePtr {
                              if (args.size() != 1) {
                                  throw std::runtime_error("json_encode expects 1 argument");
                              }

                              try {
                                  // Use the JsonConverters library to convert ValuePtr to nlohmann::json
                                  nlohmann::json jsonData = Modules::JsonConverters::valueToJson(args.at(0));

                                  // Serialize to string using nlohmann's robust serialization
                                  return Symbols::ValuePtr(jsonData.dump());
                              } catch (const std::exception& e) {
                                  throw std::runtime_error("JSON encoding failed: " + std::string(e.what()));
                              }
                          });

        params = {
            { "object", Symbols::Variables::Type::STRING, "The string to parse into object", false, false },
        };
        REGISTER_FUNCTION("json_decode", Symbols::Variables::Type::OBJECT, params, "Parse JSON string into object",
                          &Modules::JsonModule::JsonDecode);
    }

    static Symbols::ValuePtr JsonDecode(const Symbols::FunctionArguments & args);
};
};  // namespace Modules
#endif  // MODULES_JSONMODULE_HPP
