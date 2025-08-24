// JsonConvertersTests.cpp
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include "Symbols/Value.hpp"
#include "Symbols/VariableTypes.hpp"
#include "Modules/BuiltIn/JsonConverters.hpp"

using namespace Modules::JsonConverters;

TEST_CASE("JsonConverters basic functionality", "[JsonConverters]") {
    SECTION("NULL_TYPE conversion") {
        Symbols::ValuePtr nullValue = Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(nullValue);
        REQUIRE(jsonValue.is_null());

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::NULL_TYPE);
    }

    SECTION("BOOLEAN conversion") {
        Symbols::ValuePtr boolValue = Symbols::ValuePtr(true);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(boolValue);
        REQUIRE(jsonValue.is_boolean());
        REQUIRE(jsonValue.get<bool>() == true);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::BOOLEAN);
        REQUIRE(convertedValue.get<bool>() == true);
    }

    SECTION("INTEGER conversion") {
        Symbols::ValuePtr intValue = Symbols::ValuePtr(42);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(intValue);
        REQUIRE(jsonValue.is_number_integer());
        REQUIRE(jsonValue.get<int>() == 42);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(convertedValue.get<int>() == 42);
    }

    SECTION("FLOAT conversion") {
        Symbols::ValuePtr floatValue = Symbols::ValuePtr(3.14f);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(floatValue);
        REQUIRE(jsonValue.is_number());
        REQUIRE(jsonValue.get<float>() == 3.14f);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::DOUBLE); // JSON numbers become doubles
        REQUIRE(convertedValue.get<double>() == 3.14f);
    }

    SECTION("DOUBLE conversion") {
        Symbols::ValuePtr doubleValue = Symbols::ValuePtr(2.71828);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(doubleValue);
        REQUIRE(jsonValue.is_number());
        REQUIRE(jsonValue.get<double>() == 2.71828);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::DOUBLE);
        REQUIRE(convertedValue.get<double>() == 2.71828);
    }

    SECTION("STRING conversion") {
        Symbols::ValuePtr stringValue = Symbols::ValuePtr("Hello, World!");

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(stringValue);
        REQUIRE(jsonValue.is_string());
        REQUIRE(jsonValue.get<std::string>() == "Hello, World!");

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(convertedValue.get<std::string>() == "Hello, World!");
    }

    SECTION("OBJECT conversion") {
        Symbols::ObjectMap objMap;
        objMap["name"] = Symbols::ValuePtr("Test Object");
        objMap["value"] = Symbols::ValuePtr(123);
        objMap["active"] = Symbols::ValuePtr(true);

        Symbols::ValuePtr objValue = Symbols::ValuePtr(objMap);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(objValue);
        REQUIRE(jsonValue.is_object());
        REQUIRE(jsonValue["name"].get<std::string>() == "Test Object");
        REQUIRE(jsonValue["value"].get<int>() == 123);
        REQUIRE(jsonValue["active"].get<bool>() == true);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::OBJECT);

        Symbols::ObjectMap convertedMap = convertedValue.get<Symbols::ObjectMap>();
        REQUIRE(convertedMap["name"].get<std::string>() == "Test Object");
        REQUIRE(convertedMap["value"].get<int>() == 123);
        REQUIRE(convertedMap["active"].get<bool>() == true);
    }

    SECTION("ARRAY conversion (JSON array to ObjectMap)") {
        nlohmann::json jsonArray = nlohmann::json::array();
        jsonArray.push_back("first");
        jsonArray.push_back(2);
        jsonArray.push_back(3.14);
        jsonArray.push_back(true);

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonArray);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::OBJECT);

        Symbols::ObjectMap convertedMap = convertedValue.get<Symbols::ObjectMap>();
        REQUIRE(convertedMap["0"].get<std::string>() == "first");
        REQUIRE(convertedMap["1"].get<int>() == 2);
        REQUIRE(convertedMap["2"].get<double>() == 3.14);
        REQUIRE(convertedMap["3"].get<bool>() == true);
    }

    SECTION("Nested object conversion") {
        Symbols::ObjectMap innerMap;
        innerMap["id"] = Symbols::ValuePtr(1);
        innerMap["name"] = Symbols::ValuePtr("Inner");

        Symbols::ObjectMap outerMap;
        outerMap["title"] = Symbols::ValuePtr("Outer");
        outerMap["inner"] = Symbols::ValuePtr(innerMap);
        outerMap["numbers"] = Symbols::ValuePtr(std::vector<int>{1, 2, 3});

        Symbols::ValuePtr outerValue = Symbols::ValuePtr(outerMap);

        // Test ValuePtr to JSON conversion
        nlohmann::json jsonValue = valueToJson(outerValue);
        REQUIRE(jsonValue.is_object());
        REQUIRE(jsonValue["title"].get<std::string>() == "Outer");
        REQUIRE(jsonValue["inner"]["id"].get<int>() == 1);
        REQUIRE(jsonValue["inner"]["name"].get<std::string>() == "Inner");

        // Test JSON to ValuePtr conversion
        Symbols::ValuePtr convertedValue = jsonToValue(jsonValue);
        REQUIRE(convertedValue.getType() == Symbols::Variables::Type::OBJECT);

        Symbols::ObjectMap convertedMap = convertedValue.get<Symbols::ObjectMap>();
        REQUIRE(convertedMap["title"].get<std::string>() == "Outer");

        Symbols::ObjectMap convertedInner = convertedMap["inner"].get<Symbols::ObjectMap>();
        REQUIRE(convertedInner["id"].get<int>() == 1);
        REQUIRE(convertedInner["name"].get<std::string>() == "Inner");
    }

    SECTION("Validation functions") {
        Symbols::ValuePtr nullValue = Symbols::ValuePtr::null(Symbols::Variables::Type::NULL_TYPE);
        Symbols::ValuePtr intValue = Symbols::ValuePtr(42);
        Symbols::ValuePtr stringValue = Symbols::ValuePtr("test");

        nlohmann::json jsonNull = nullptr;
        nlohmann::json jsonBool = true;
        nlohmann::json jsonString = "test";
        nlohmann::json jsonObj = {{"key", "value"}};

        // Test canConvertToJson
        REQUIRE(canConvertToJson(nullValue) == true);
        REQUIRE(canConvertToJson(intValue) == true);
        REQUIRE(canConvertToJson(stringValue) == true);

        // Test canConvertToValue
        REQUIRE(canConvertToValue(jsonNull) == true);
        REQUIRE(canConvertToValue(jsonBool) == true);
        REQUIRE(canConvertToValue(jsonString) == true);
        REQUIRE(canConvertToValue(jsonObj) == true);
    }

    SECTION("Error handling - invalid ValuePtr") {
        // Test with null ValuePtr
        REQUIRE_THROWS_AS(valueToJson(Symbols::ValuePtr()), std::runtime_error);

        // Test with invalid JSON type (if any)
        // Note: nlohmann::json should handle all basic types correctly
    }
}