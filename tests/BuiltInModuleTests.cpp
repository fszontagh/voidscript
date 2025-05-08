// BuiltInModuleTests.cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <sstream>

#include "Modules/BuiltIn/ArrayModule.hpp"
#include "Modules/BuiltIn/FileModule.hpp"
#include "Modules/BuiltIn/HeaderModule.hpp"
#include "Modules/BuiltIn/JsonModule.hpp"
#include "Modules/BuiltIn/PrintModule.hpp"
#include "Modules/BuiltIn/StringModule.hpp"
#include "Modules/BuiltIn/VariableHelpersModule.hpp"
#include "Modules/UnifiedModuleManager.hpp"
#include "Symbols/Value.hpp"

using namespace Modules;
using namespace Symbols;

TEST_CASE("BuiltInModules all functions", "[built_in]") {
    auto & mm = UnifiedModuleManager::instance();
    // Register modules
    mm.addModule(std::make_unique<ArrayModule>());
    mm.addModule(std::make_unique<FileModule>());
    mm.addModule(std::make_unique<HeaderModule>());
    mm.addModule(std::make_unique<JsonModule>());
    mm.addModule(std::make_unique<PrintModule>());
    mm.addModule(std::make_unique<StringModule>());
    mm.addModule(std::make_unique<VariableHelpersModule>());
    mm.registerAll();

    SECTION("ArrayModule sizeof") {
        Value::ObjectMap map;
        map["0"]    = Value(1);
        map["1"]    = Value(2);
        auto result = mm.callFunction("sizeof", { Value(map) });
        REQUIRE(result.getType() == Variables::Type::INTEGER);
        REQUIRE(result.get<int>() == 2);
        // Scalar and boolean types yield 1
        REQUIRE(mm.callFunction("sizeof", { Value(123) }).get<int>() == 1);
        REQUIRE(mm.callFunction("sizeof", { Value(3.14) }).get<int>() == 1);
        REQUIRE(mm.callFunction("sizeof", { Value(true) }).get<int>() == 1);
        // String length
        REQUIRE(mm.callFunction("sizeof", { Value("abc") }).get<int>() == 3);
        // Wrong argument count
        REQUIRE_THROWS_AS(mm.callFunction("sizeof", {}), std::runtime_error);
    }

    SECTION("FileModule file_exists and file_get/put_contents") {
        std::string filename = "test_file.txt";
        if (std::filesystem::exists(filename)) {
            std::filesystem::remove(filename);
        }
        auto existsFalse = mm.callFunction("file_exists", { Value(filename) });
        REQUIRE(existsFalse.get<bool>() == false);
        std::string content = "hello world";
        mm.callFunction("file_put_contents", { Value(filename), Value(content), Value(false) });
        REQUIRE(std::filesystem::exists(filename));
        auto readVal = mm.callFunction("file_get_contents", { Value(filename) });
        REQUIRE(readVal.getType() == Variables::Type::STRING);
        REQUIRE(readVal.get<std::string>() == content);
        auto existsTrue = mm.callFunction("file_exists", { Value(filename) });
        REQUIRE(existsTrue.get<bool>() == true);
        REQUIRE_THROWS_AS(mm.callFunction("file_put_contents", { Value(filename), Value("x"), Value(false) }),
                          std::runtime_error);
        mm.callFunction("file_put_contents", { Value(filename), Value("new"), Value(true) });
        auto readVal2 = mm.callFunction("file_get_contents", { Value(filename) });
        REQUIRE(readVal2.get<std::string>() == "new");
        std::filesystem::remove(filename);
        REQUIRE_THROWS_AS(mm.callFunction("file_get_contents", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("file_exists", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("file_put_contents", {}), std::runtime_error);
    }

    SECTION("HeaderModule header management") {
        HeaderModule::clearHeaders();
        REQUIRE(HeaderModule::getHeaders().empty());
        REQUIRE_THROWS(mm.callFunction("header", {}));
        mm.callFunction("header", { Value("Content-Type"), Value("text/plain") });
        auto hdrs = HeaderModule::getHeaders();
        REQUIRE(hdrs.size() == 1);
        REQUIRE(hdrs.at("Content-Type") == "text/plain");
        mm.callFunction("header", { Value("Content-Type"), Value("application/json") });
        hdrs = HeaderModule::getHeaders();
        REQUIRE(hdrs.size() == 1);
        REQUIRE(hdrs.at("Content-Type") == "application/json");
        HeaderModule::clearHeaders();
        REQUIRE(HeaderModule::getHeaders().empty());
    }

    SECTION("JsonModule encode and decode") {
        REQUIRE(mm.callFunction("json_encode", { Value(123) }).get<std::string>() == "123");
        REQUIRE(mm.callFunction("json_encode", { Value(true) }).get<std::string>() == "true");
        REQUIRE(mm.callFunction("json_encode", { Value(false) }).get<std::string>() == "false");
        REQUIRE(mm.callFunction("json_encode", { Value("abc") }).get<std::string>() == "\"abc\"");
        Value::ObjectMap obj;
        obj["a"]    = Value(1);
        obj["b"]    = Value("x");
        auto encObj = mm.callFunction("json_encode", { Value(obj) }).get<std::string>();
        REQUIRE(encObj == "{\"a\":1,\"b\":\"x\"}");
        auto decNum = mm.callFunction("json_decode", { Value("123") });
        REQUIRE(decNum.getType() == Variables::Type::INTEGER);
        REQUIRE(decNum.get<int>() == 123);
        auto decBool = mm.callFunction("json_decode", { Value("false") });
        REQUIRE(decBool.getType() == Variables::Type::BOOLEAN);
        REQUIRE(decBool.get<bool>() == false);
        auto decStr = mm.callFunction("json_decode", { Value("\"hi\"") });
        REQUIRE(decStr.getType() == Variables::Type::STRING);
        REQUIRE(decStr.get<std::string>() == "hi");
        auto decObj = mm.callFunction("json_decode", { Value("{\"k\":10}") });
        REQUIRE(decObj.getType() == Variables::Type::OBJECT);
        auto decMap = std::get<Value::ObjectMap>(decObj.get());
        REQUIRE(decMap["k"].get<int>() == 10);
        REQUIRE_THROWS_AS(mm.callFunction("json_encode", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("json_decode", { Value(1) }), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("json_decode", {}), std::runtime_error);
    }

    SECTION("PrintModule output and errors") {
        std::ostringstream out;
        auto *             oldOut = std::cout.rdbuf(out.rdbuf());
        mm.callFunction("print", { Value("A"), Value("B") });
        REQUIRE(out.str() == "AB");
        out.str("");
        mm.callFunction("printnl", { Value("X"), Value("Y") });
        REQUIRE(out.str() == "XY\n");
        std::cout.rdbuf(oldOut);
        std::ostringstream err;
        auto *             oldErr = std::cerr.rdbuf(err.rdbuf());
        mm.callFunction("error", { Value("E1"), Value("E2") });
        REQUIRE(err.str() == "E1E2\n");
        err.str("");
        REQUIRE_THROWS_AS(mm.callFunction("throw_error", { Value("oops") }), Exception);
        std::cerr.rdbuf(oldErr);
    }

    SECTION("StringModule functions") {
        auto len = mm.callFunction("string_length", { Value("hello") });
        REQUIRE(len.get<int>() == 5);
        REQUIRE(mm.callFunction("string_replace", { Value("foofoo"), Value("foo"), Value("bar"), Value(true) })
                    .get<std::string>() == "barbar");
        REQUIRE(mm.callFunction("string_replace", { Value("foofoo"), Value("foo"), Value("bar"), Value(false) })
                    .get<std::string>() == "barfoo");
        REQUIRE(mm.callFunction("string_substr", { Value("hello"), Value(1), Value(3) }).get<std::string>() == "ell");
        REQUIRE_THROWS_AS(mm.callFunction("string_length", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("string_replace", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("string_substr", { Value("x"), Value(-1), Value(1) }), std::runtime_error);
    }

    SECTION("VariableHelpersModule typeof") {
        auto t1 = mm.callFunction("typeof", { Value(10) });
        REQUIRE(t1.get<std::string>() == "int");
        auto t2 = mm.callFunction("typeof", { Value(10), Value("int") });
        REQUIRE(t2.get<bool>() == true);
        auto t3 = mm.callFunction("typeof", { Value(10), Value("string") });
        REQUIRE(t3.get<bool>() == false);
        REQUIRE_THROWS_AS(mm.callFunction("typeof", {}), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("typeof", { Value(1), Value(2), Value(3) }), std::runtime_error);
        REQUIRE_THROWS_AS(mm.callFunction("typeof", { Value(1), Value(2) }), std::runtime_error);
        // Synonym support for boolean
        REQUIRE(mm.callFunction("typeof", { Value(true), Value("boolean") }).get<bool>() == true);
        // Unrecognized type name yields false
        REQUIRE(mm.callFunction("typeof", { Value(true), Value("unknown") }).get<bool>() == false);
        // Numeric type synonyms
        REQUIRE(mm.callFunction("typeof", { Value(3.14), Value("double") }).get<bool>() == true);
        REQUIRE(mm.callFunction("typeof", { Value(2.71f), Value("float") }).get<bool>() == true);
        // String type
        REQUIRE(mm.callFunction("typeof", { Value("hi"), Value("string") }).get<bool>() == true);
    }
}
