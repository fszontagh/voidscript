#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "Interpreter/Interpreter.hpp"
#include "Interpreter/Lexer.hpp"
#include "Interpreter/Parser.hpp"
#include "Symbols/SymbolContainer.hpp"
#include "Symbols/SymbolFactory.hpp"
#include "Symbols/Value.hpp"
#include "Symbols/Variable.hpp"
#include "Symbols/FunctionSymbol.hpp"
#include "Interpreter/Nodes/Statement/DeclareVariableStatementNode.hpp"
#include "Interpreter/Nodes/Expression/LiteralNode.hpp"
#include "Modules/UnifiedModuleManager.hpp" // For resetting module states if necessary
#include "Interpreter/OperationContainer.hpp" // For resetting operations

// Helper function to reset global states for test isolation
void reset_global_state() {
    Symbols::SymbolContainer::instance()->reset(); // Resets all scopes and symbols
    Interpreter::Interpreter::reset_unique_call_id(); // Reset unique call ID for functions
    Modules::UnifiedModuleManager::instance().reset(); // Reset modules
    Operations::Container::instance()->reset(); // Reset operations
    // Re-register core types/modules if necessary, though SymbolContainer::reset should be comprehensive
    // For example, if there's a core module that needs to be re-initialized:
    // Modules::Builtins::registerModule(Modules::UnifiedModuleManager::instance());
}

// Helper function to interpret a script
void interpret_script(const std::string& script_content, const std::string& script_name = "test_script") {
    Interpreter::Lexer  lexer(script_content, script_name);
    Interpreter::Parser parser(lexer);
    auto                program = parser.parse(); // Assuming parse() returns ProgramNode
    Interpreter::Interpreter interpreter;
    program->interpret(interpreter);
}

// Helper function to get a symbol's value from the global scope
Symbols::ValuePtr get_global_value(const std::string& symbol_name) {
    auto sc = Symbols::SymbolContainer::instance();
    auto global_scope = sc->getGlobalScopeTable();
    if (!global_scope) {
        FAIL("Global scope table not found.");
        return Symbols::ValuePtr::null(); // Should not reach here if FAIL works
    }
    auto symbol = global_scope->get(Symbols::SymbolContainer::DEFAULT_VARIABLES_SCOPE, symbol_name);
    if (!symbol) {
        // Try constants scope if not found in variables
        symbol = global_scope->get(Symbols::SymbolContainer::DEFAULT_CONSTANTS_SCOPE, symbol_name);
    }
    if (!symbol) {
        FAIL("Symbol '" + symbol_name + "' not found in global scope.");
        return Symbols::ValuePtr::null(); // Should not reach here
    }
    return symbol->getValue();
}

// Helper to get a specific property from an ObjectMap ValuePtr
Symbols::ValuePtr get_object_property(Symbols::ValuePtr object_val, const std::string& prop_name) {
    REQUIRE(object_val.getType() == Symbols::Variables::Type::OBJECT);
    const auto& map = object_val->get<Symbols::ObjectMap>();
    auto it = map.find(prop_name);
    REQUIRE(it != map.end());
    return it->second;
}

// Helper to check if a key exists in an ObjectMap ValuePtr
bool object_has_property(Symbols::ValuePtr object_val, const std::string& prop_name) {
    if (object_val.getType() != Symbols::Variables::Type::OBJECT && object_val.getType() != Symbols::Variables::Type::CLASS) {
        return false;
    }
    const auto& map = object_val->get<Symbols::ObjectMap>();
    return map.count(prop_name) > 0;
}

// Helper function for the implicit conversion test case
static void check_type_passed(Symbols::Variables::Type actual_type, Symbols::Variables::Type expected_type) {
    REQUIRE(actual_type == expected_type);
}


TEST_CASE("Value Semantics Tests", "[ValueSemantics]") {

    SECTION("Test 1: Simple Assignment & Independence") {
        reset_global_state();
        std::string script = R"(
            $a = 10;
            $b = $a;
            $b = 20;
        )";
        interpret_script(script);
        auto val_a = get_global_value("a");
        REQUIRE(val_a.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(val_a->get<int>() == 10);
    }

    SECTION("Test 2: Object Assignment & Independence") {
        reset_global_state();
        std::string script = R"(
            $obj1 = {"key": "val", "num": 1};
            $obj2 = $obj1;
            $obj2["key"] = "new_val";
            $obj2["num"] = 2;
        )";
        interpret_script(script);
        auto val_obj1 = get_global_value("obj1");
        REQUIRE(val_obj1.getType() == Symbols::Variables::Type::OBJECT);
        
        auto obj1_key = get_object_property(val_obj1, "key");
        REQUIRE(obj1_key.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(obj1_key->get<std::string>() == "val");

        auto obj1_num = get_object_property(val_obj1, "num");
        REQUIRE(obj1_num.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(obj1_num->get<int>() == 1);
    }

    SECTION("Test 3: Nested Object Assignment & Independence") {
        reset_global_state();
        std::string script = R"(
            $obj1 = {"nested": {"key": "val"}, "top": "original"};
            $obj2 = $obj1;
            $obj2["nested"]["key"] = "new_val";
            $obj2["top"] = "changed";
        )";
        interpret_script(script);
        auto val_obj1 = get_global_value("obj1");
        REQUIRE(val_obj1.getType() == Symbols::Variables::Type::OBJECT);

        auto val_obj1_top = get_object_property(val_obj1, "top");
        REQUIRE(val_obj1_top.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(val_obj1_top->get<std::string>() == "original");

        auto val_obj1_nested = get_object_property(val_obj1, "nested");
        REQUIRE(val_obj1_nested.getType() == Symbols::Variables::Type::OBJECT);
        
        auto val_obj1_nested_key = get_object_property(val_obj1_nested, "key");
        REQUIRE(val_obj1_nested_key.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(val_obj1_nested_key->get<std::string>() == "val");
    }

    SECTION("Test 4: Function Call Semantics (Primitive Argument)") {
        reset_global_state();
        std::string script = R"(
            function modify(param) {
                param = 100;
                return param;
            }
            $x = 10;
            $y = modify($x);
        )";
        interpret_script(script);
        auto val_x = get_global_value("x");
        REQUIRE(val_x.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(val_x->get<int>() == 10);

        auto val_y = get_global_value("y");
        REQUIRE(val_y.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(val_y->get<int>() == 100);
    }

    SECTION("Test 5: Function Call Semantics (Object Argument)") {
        reset_global_state();
        std::string script = R"(
            function modify_obj(obj_param) {
                obj_param["key"] = "modified_in_func";
                obj_param["new_key"] = "added_in_func";
                return obj_param;
            }
            $my_obj = {"key": "original_val"};
            $modified_obj = modify_obj($my_obj);
        )";
        interpret_script(script);
        auto val_my_obj = get_global_value("my_obj");
        REQUIRE(val_my_obj.getType() == Symbols::Variables::Type::OBJECT);
        
        auto my_obj_key = get_object_property(val_my_obj, "key");
        REQUIRE(my_obj_key.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(my_obj_key->get<std::string>() == "original_val");
        
        REQUIRE_FALSE(object_has_property(val_my_obj, "new_key"));

        auto val_modified_obj = get_global_value("modified_obj");
        REQUIRE(val_modified_obj.getType() == Symbols::Variables::Type::OBJECT);
        
        auto modified_obj_key = get_object_property(val_modified_obj, "key");
        REQUIRE(modified_obj_key.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(modified_obj_key->get<std::string>() == "modified_in_func");
        
        REQUIRE(object_has_property(val_modified_obj, "new_key"));
    }

    SECTION("Test 6: ValuePtr::clone() of Null Value (C++ API)") {
        reset_global_state(); // Reset for safety, though this test is pure C++
        
        Symbols::ValuePtr vp1; // Default constructed, ptr_ is new Value which is_null
        REQUIRE(vp1->isNULL());
        REQUIRE(vp1.getType() == Symbols::Variables::Type::NULL_TYPE);

        auto vp2 = vp1.clone();
        REQUIRE(vp2->isNULL());
        REQUIRE(vp2.getType() == Symbols::Variables::Type::NULL_TYPE);
        REQUIRE(vp1.operator->() != vp2.operator->()); // Should be different Value objects

        Symbols::ValuePtr vp3 = Symbols::ValuePtr::null(Symbols::Variables::Type::STRING);
        // Based on Value.cpp: ValuePtr::null(STRING) creates an EMPTY NON-NULL string.
        REQUIRE_FALSE(vp3->isNULL()); 
        REQUIRE(vp3.getType() == Symbols::Variables::Type::STRING); 

        auto vp4 = vp3.clone();
        REQUIRE_FALSE(vp4->isNULL());
        REQUIRE(vp4.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(vp3.operator->() != vp4.operator->());
        REQUIRE(vp4->get<std::string>() == ""); 
    }

    SECTION("Test 7: Cloning of All Supported Symbols::Variables::Type (C++ API)") {
        reset_global_state();

        // INTEGER
        Symbols::ValuePtr source_int_vp(123);
        auto cloned_int_vp = source_int_vp.clone();
        REQUIRE(source_int_vp.operator->() != cloned_int_vp.operator->());
        REQUIRE(cloned_int_vp.getType() == Symbols::Variables::Type::INTEGER);
        REQUIRE(cloned_int_vp->get<int>() == 123);
        cloned_int_vp->get<int>() = 456;
        REQUIRE(source_int_vp->get<int>() == 123);
        REQUIRE(cloned_int_vp->get<int>() == 456);

        // STRING
        Symbols::ValuePtr source_str_vp(std::string("hello"));
        auto cloned_str_vp = source_str_vp.clone();
        REQUIRE(source_str_vp.operator->() != cloned_str_vp.operator->());
        REQUIRE(cloned_str_vp.getType() == Symbols::Variables::Type::STRING);
        REQUIRE(cloned_str_vp->get<std::string>() == "hello");
        cloned_str_vp->get<std::string>() = "world";
        REQUIRE(source_str_vp->get<std::string>() == "hello");
        REQUIRE(cloned_str_vp->get<std::string>() == "world");

        // BOOLEAN
        Symbols::ValuePtr source_bool_vp(true);
        auto cloned_bool_vp = source_bool_vp.clone();
        REQUIRE(source_bool_vp.operator->() != cloned_bool_vp.operator->());
        REQUIRE(cloned_bool_vp.getType() == Symbols::Variables::Type::BOOLEAN);
        REQUIRE(cloned_bool_vp->get<bool>() == true);
        cloned_bool_vp->get<bool>() = false;
        REQUIRE(source_bool_vp->get<bool>() == true);
        REQUIRE(cloned_bool_vp->get<bool>() == false);

        // DOUBLE
        Symbols::ValuePtr source_double_vp(123.456);
        auto cloned_double_vp = source_double_vp.clone();
        REQUIRE(source_double_vp.operator->() != cloned_double_vp.operator->());
        REQUIRE(cloned_double_vp.getType() == Symbols::Variables::Type::DOUBLE);
        REQUIRE(cloned_double_vp->get<double>() == Approx(123.456));
        cloned_double_vp->get<double>() = 789.012;
        REQUIRE(source_double_vp->get<double>() == Approx(123.456));
        REQUIRE(cloned_double_vp->get<double>() == Approx(789.012));
        
        // FLOAT (similar to double)
        Symbols::ValuePtr source_float_vp(12.34f);
        auto cloned_float_vp = source_float_vp.clone();
        REQUIRE(source_float_vp.operator->() != cloned_float_vp.operator->());
        REQUIRE(cloned_float_vp.getType() == Symbols::Variables::Type::FLOAT);
        REQUIRE(cloned_float_vp->get<float>() == Approx(12.34f));
        cloned_float_vp->get<float>() = 56.78f;
        REQUIRE(source_float_vp->get<float>() == Approx(12.34f));
        REQUIRE(cloned_float_vp->get<float>() == Approx(56.78f));

        // OBJECT
        Symbols::ObjectMap inner_map;
        inner_map["inner_key"] = Symbols::ValuePtr(std::string("inner_value"));
        
        Symbols::ObjectMap outer_map;
        outer_map["top_key"] = Symbols::ValuePtr(std::string("top_value"));
        outer_map["nested_obj"] = Symbols::ValuePtr(inner_map);
        outer_map["primitive"] = Symbols::ValuePtr(777);

        Symbols::ValuePtr source_obj_vp(outer_map);
        auto cloned_obj_vp = source_obj_vp.clone();

        REQUIRE(source_obj_vp.operator->() != cloned_obj_vp.operator->()); // Different Value objects
        REQUIRE(cloned_obj_vp.getType() == Symbols::Variables::Type::OBJECT);
        
        // Check deep copy quality
        const auto& cloned_map_ref = cloned_obj_vp->get<Symbols::ObjectMap>();
        const auto& source_map_ref = source_obj_vp->get<Symbols::ObjectMap>();

        REQUIRE(cloned_map_ref.at("top_key")->get<std::string>() == "top_value");
        REQUIRE(cloned_map_ref.at("primitive")->get<int>() == 777);
        
        auto cloned_nested_obj = cloned_map_ref.at("nested_obj");
        REQUIRE(cloned_nested_obj.getType() == Symbols::Variables::Type::OBJECT);
        REQUIRE(cloned_nested_obj->get<Symbols::ObjectMap>().at("inner_key")->get<std::string>() == "inner_value");

        // Ensure internal ValuePtrs are also clones, not same shared_ptr instances
        REQUIRE(source_map_ref.at("top_key").operator->() != cloned_map_ref.at("top_key").operator->());
        REQUIRE(source_map_ref.at("primitive").operator->() != cloned_map_ref.at("primitive").operator->());
        REQUIRE(source_map_ref.at("nested_obj").operator->() != cloned_nested_obj.operator->());
        
        auto source_nested_obj_valptr = source_map_ref.at("nested_obj");
        auto cloned_nested_obj_valptr = cloned_map_ref.at("nested_obj");
        REQUIRE(source_nested_obj_valptr->get<Symbols::ObjectMap>().at("inner_key").operator->() != cloned_nested_obj_valptr->get<Symbols::ObjectMap>().at("inner_key").operator->());


        // Modify cloned object
        cloned_obj_vp->get<Symbols::ObjectMap>()["top_key"] = Symbols::ValuePtr(std::string("new_top_value"));
        cloned_obj_vp->get<Symbols::ObjectMap>()["primitive"]->get<int>() = 888;
        cloned_nested_obj->get<Symbols::ObjectMap>()["inner_key"]->get<std::string>() = "new_inner_value";
        cloned_obj_vp->get<Symbols::ObjectMap>()["added_key"] = Symbols::ValuePtr(std::string("just_for_cloned"));


        // Verify source object is unchanged
        REQUIRE(source_obj_vp->get<Symbols::ObjectMap>().at("top_key")->get<std::string>() == "top_value");
        REQUIRE(source_obj_vp->get<Symbols::ObjectMap>().at("primitive")->get<int>() == 777);
        REQUIRE(source_obj_vp->get<Symbols::ObjectMap>().at("nested_obj")->get<Symbols::ObjectMap>().at("inner_key")->get<std::string>() == "inner_value");
        REQUIRE_FALSE(object_has_property(source_obj_vp, "added_key"));

        // Verify cloned changes
        REQUIRE(cloned_obj_vp->get<Symbols::ObjectMap>().at("top_key")->get<std::string>() == "new_top_value");
        REQUIRE(cloned_obj_vp->get<Symbols::ObjectMap>().at("primitive")->get<int>() == 888);
        REQUIRE(cloned_obj_vp->get<Symbols::ObjectMap>().at("nested_obj")->get<Symbols::ObjectMap>().at("inner_key")->get<std::string>() == "new_inner_value");
        REQUIRE(object_has_property(cloned_obj_vp, "added_key"));
        REQUIRE(cloned_obj_vp->get<Symbols::ObjectMap>().at("added_key")->get<std::string>() == "just_for_cloned");

        // NULL_TYPE is tested in Test 6
    }
}


TEST_CASE("ValuePtr implicit conversion to Variables::Type", "[ValueSemantics][Conversion]") {
    reset_global_state(); 

    Symbols::ValuePtr int_ptr(123);
    Symbols::ValuePtr string_ptr("test_string");
    Symbols::ValuePtr bool_ptr(true);
    Symbols::ValuePtr double_ptr(123.456);
    Symbols::ValuePtr object_ptr(Symbols::ObjectMap{});
    Symbols::ValuePtr default_null_ptr; // Default constructor makes it NULL_TYPE
    Symbols::ValuePtr string_null_ptr = Symbols::ValuePtr::null(Symbols::Variables::Type::STRING);

    SECTION("Integer type") {
        check_type_passed(int_ptr, Symbols::Variables::Type::INTEGER);
    }
    SECTION("String type") {
        check_type_passed(string_ptr, Symbols::Variables::Type::STRING);
    }
    SECTION("Boolean type") {
        check_type_passed(bool_ptr, Symbols::Variables::Type::BOOLEAN);
    }
    SECTION("Double type") {
        check_type_passed(double_ptr, Symbols::Variables::Type::DOUBLE);
    }
    SECTION("Object type") {
        check_type_passed(object_ptr, Symbols::Variables::Type::OBJECT);
    }
    SECTION("Default Null type") {
        check_type_passed(default_null_ptr, Symbols::Variables::Type::NULL_TYPE);
    }
    SECTION("String Null type (actually empty non-null string)") {
        // ValuePtr::null(STRING) creates an EMPTY NON-NULL string.
        check_type_passed(string_null_ptr, Symbols::Variables::Type::STRING); 
        REQUIRE_FALSE(string_null_ptr->isNULL()); // Based on current ValuePtr::null impl.
                                                 // which calls set<std::string>("") and is_null = false.
    }
}
