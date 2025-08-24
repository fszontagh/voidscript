// JsonModule.cpp
#include "JsonModule.hpp"

#include <stdexcept>
#include <string>

#include "../../Symbols/FunctionParameterInfo.hpp"
#include "../../Symbols/RegistrationMacros.hpp"
#include "../../Symbols/SymbolContainer.hpp"
#include "../../Symbols/Value.hpp"
#include "../../Symbols/VariableTypes.hpp"
#include "../../json.hpp"
#include "JsonConverters.hpp"

namespace Modules {

Symbols::ValuePtr JsonModule::JsonDecode(const Symbols::FunctionArguments & args) {
    if (args.size() != 1) {
        throw std::runtime_error("json_decode expects 1 argument");
    }
    if (args[0]->getType() != Symbols::Variables::Type::STRING) {
        throw std::runtime_error("json_decode expects a JSON string");
    }
    const std::string s = args[0]->get<std::string>();

    try {
        // Use nlohmann's robust JSON parser to parse the string
        nlohmann::json jsonData = nlohmann::json::parse(s);

        // Convert the parsed JSON to a VoidScript ValuePtr using the existing converter
        return Modules::JsonConverters::jsonToValueWithContext(jsonData, "json_decode");
    } catch (const nlohmann::detail::parse_error& e) {
        // Provide detailed error information for parsing failures
        std::string errorMsg = "JSON parsing error at position " + std::to_string(e.byte) +
                             ": " + std::string(e.what());
        throw std::runtime_error(errorMsg);
    } catch (const nlohmann::detail::exception& e) {
        // Handle other nlohmann JSON exceptions
        std::string errorMsg = "JSON error: " + std::string(e.what());
        throw std::runtime_error(errorMsg);
    } catch (const std::exception& e) {
        // Handle standard exceptions
        std::string errorMsg = "JSON decoding failed: " + std::string(e.what());
        throw std::runtime_error(errorMsg);
    } catch (...) {
        // Handle any other unexpected exceptions
        throw std::runtime_error("JSON decoding failed with unknown error");
    }
}

}  // namespace Modules