#include <catch2/catch_test_macros.hpp>
#include "Symbols/ClassRegistry.hpp"
#include "Symbols/ClassMigration.hpp"
#include "Symbols/Value.hpp"

using namespace Symbols;

TEST_CASE("UnifiedClassContainer basic operations", "[class][unified]") {
    UnifiedClassContainer container;

    SECTION("Register and retrieve a class") {
        auto& classInfo = container.registerClass("TestClass");
        REQUIRE(classInfo.name == "TestClass");
        REQUIRE(classInfo.parentClass.empty());
        REQUIRE(container.hasClass("TestClass"));
        
        const auto& retrievedInfo = container.getClassInfo("TestClass");
        REQUIRE(retrievedInfo.name == "TestClass");
    }

    SECTION("Register class with inheritance") {
        container.registerClass("ParentClass");
        auto& childInfo = container.registerClass("ChildClass", "ParentClass");
        REQUIRE(childInfo.name == "ChildClass");
        REQUIRE(childInfo.parentClass == "ParentClass");
    }

    SECTION("Add and retrieve properties") {
        auto& classInfo = container.registerClass("PropertyTest");
        container.addProperty("PropertyTest", "testProp", Variables::Type::STRING, false);
        REQUIRE(container.hasProperty("PropertyTest", "testProp"));
        REQUIRE(container.getPropertyType("PropertyTest", "testProp") == Variables::Type::STRING);
    }

    SECTION("Add and retrieve methods") {
        auto& classInfo = container.registerClass("MethodTest");
        std::vector<UnifiedClassContainer::ParameterInfo> params = {
            {"param1", Variables::Type::INT},
            {"param2", Variables::Type::STRING}
        };
        container.addMethod("MethodTest", "testMethod", Variables::Type::BOOL, params);
        REQUIRE(container.hasMethod("MethodTest", "testMethod"));
        REQUIRE(container.getMethodReturnType("MethodTest", "testMethod") == Variables::Type::BOOL);
        auto& methodParams = container.getMethodParameters("MethodTest", "testMethod");
        REQUIRE(methodParams.size() == 2);
        REQUIRE(methodParams[0].name == "param1");
        REQUIRE(methodParams[0].type == Variables::Type::INT);
    }

    SECTION("Static properties") {
        auto& classInfo = container.registerClass("StaticTest");
        auto value = ValuePtr("test value");
        container.setStaticProperty("StaticTest", "staticProp", value);
        REQUIRE(container.hasStaticProperty("StaticTest", "staticProp"));
        auto retrieved = container.getStaticProperty("StaticTest", "staticProp");
        REQUIRE(retrieved->get<std::string>() == "test value");
    }
}

TEST_CASE("ClassFactory operations", "[class][factory]") {
    UnifiedClassContainer container;
    ClassFactory factory(container);

    SECTION("Create a basic instance") {
        container.registerClass("SimpleClass");
        container.addProperty("SimpleClass", "name", Variables::Type::STRING);
        container.addProperty("SimpleClass", "age", Variables::Type::INT);
        
        auto instance = factory.createInstance("SimpleClass");
        REQUIRE(instance->getType() == Variables::Type::OBJECT);
        REQUIRE(factory.getClassName(instance) == "SimpleClass");
        REQUIRE(factory.hasProperty(instance, "name"));
        REQUIRE(factory.hasProperty(instance, "age"));
    }

    SECTION("Property access") {
        container.registerClass("PropClass");
        container.addProperty("PropClass", "value", Variables::Type::STRING);
        
        auto instance = factory.createInstance("PropClass");
        auto value = ValuePtr("test value");
        factory.setProperty(instance, "value", value);
        
        auto retrieved = factory.getProperty(instance, "value");
        REQUIRE(retrieved->get<std::string>() == "test value");
    }

    SECTION("Instance of checking") {
        container.registerClass("BaseClass");
        container.registerClass("ChildClass", "BaseClass");
        
        auto baseInstance = factory.createInstance("BaseClass");
        auto childInstance = factory.createInstance("ChildClass");
        
        REQUIRE(factory.isInstanceOf(baseInstance, "BaseClass"));
        REQUIRE_FALSE(factory.isInstanceOf(baseInstance, "ChildClass"));
        
        REQUIRE(factory.isInstanceOf(childInstance, "ChildClass"));
        REQUIRE(factory.isInstanceOf(childInstance, "BaseClass"));
    }
}

TEST_CASE("ClassRegistry operations", "[class][registry]") {
    ClassRegistry registry;

    SECTION("Register and create instances") {
        registry.registerClass("TestClass");
        REQUIRE(registry.hasClass("TestClass"));
        
        auto instance = registry.createInstance("TestClass");
        REQUIRE(instance->getType() == Variables::Type::OBJECT);
        REQUIRE(registry.getClassFactory().getClassName(instance) == "TestClass");
    }

    SECTION("Static and instance properties") {
        registry.registerClass("PropClass");
        registry.getClassContainer().addProperty("PropClass", "instProp", Variables::Type::STRING);
        
        auto staticVal = ValuePtr("static value");
        registry.setStaticProperty("PropClass", "staticProp", staticVal);
        
        auto instance = registry.createInstance("PropClass");
        auto instVal = ValuePtr("instance value");
        registry.setInstanceProperty(instance, "instProp", instVal);
        
        REQUIRE(registry.getStaticProperty("PropClass", "staticProp")->get<std::string>() == "static value");
        REQUIRE(registry.getInstanceProperty(instance, "instProp")->get<std::string>() == "instance value");
    }
}

TEST_CASE("ClassContainerAdapter backward compatibility", "[class][adapter]") {
    // Get the adapter instance
    auto adapter = ClassContainerAdapter::instance();
    
    SECTION("Register and retrieve classes") {
        adapter->registerClass("AdapterTest");
        REQUIRE(adapter->hasClass("AdapterTest"));
        REQUIRE(ClassRegistry::instance().hasClass("AdapterTest"));
        
        const auto& info = adapter->getClassInfo("AdapterTest");
        REQUIRE(info.name == "AdapterTest");
    }
    
    SECTION("Properties and methods") {
        adapter->registerClass("PropMethodTest");
        
        adapter->addProperty("PropMethodTest", "testProp", Variables::Type::STRING);
        REQUIRE(adapter->hasProperty("PropMethodTest", "testProp"));
        REQUIRE(ClassRegistry::instance().getClassContainer().hasProperty("PropMethodTest", "testProp"));
        
        std::vector<functionParameterType> params = {
            {"param1", Variables::Type::INT}
        };
        adapter->addMethod("PropMethodTest", "testMethod", Variables::Type::BOOL, params);
        REQUIRE(adapter->hasMethod("PropMethodTest", "testMethod"));
        REQUIRE(ClassRegistry::instance().getClassContainer().hasMethod("PropMethodTest", "testMethod"));
    }
    
    SECTION("Static properties") {
        adapter->registerClass("StaticTest");
        auto value = ValuePtr("test value");
        adapter->setObjectProperty("StaticTest", "staticProp", value);
        REQUIRE(adapter->hasObjectProperty("StaticTest", "staticProp"));
        REQUIRE(ClassRegistry::instance().hasStaticProperty("StaticTest", "staticProp"));
        
        auto retrieved = adapter->getObjectProperty("StaticTest", "staticProp");
        REQUIRE(retrieved->get<std::string>() == "test value");
    }
}

TEST_CASE("ClassMigration", "[class][migration]") {
    // First, set up some classes in the old system
    auto oldContainer = ClassContainer::instance();
    oldContainer->registerClass("MigrationTest");
    oldContainer->addProperty("MigrationTest", "name", Variables::Type::STRING);
    oldContainer->addMethod("MigrationTest", "sayHello", Variables::Type::STRING);
    oldContainer->setObjectProperty("MigrationTest", "VERSION", ValuePtr("1.0"));
    
    // Create a new registry
    ClassRegistry registry;
    
    SECTION("Migrate single class") {
        bool success = ClassMigration::migrateClass("MigrationTest", registry);
        REQUIRE(success);
        REQUIRE(registry.hasClass("MigrationTest"));
        REQUIRE(registry.getClassContainer().hasProperty("MigrationTest", "name"));
        REQUIRE(registry.getClassContainer().hasMethod("MigrationTest", "sayHello"));
        REQUIRE(registry.hasStaticProperty("MigrationTest", "VERSION"));
        REQUIRE(registry.getStaticProperty("MigrationTest", "VERSION")->get<std::string>() == "1.0");
    }
    
    SECTION("Migrate all classes") {
        oldContainer->registerClass("AnotherClass");
        oldContainer->addProperty("AnotherClass", "count", Variables::Type::INT);
        
        int count = ClassMigration::migrateAllClasses(registry);
        REQUIRE(count >= 2);  // At least MigrationTest and AnotherClass
        REQUIRE(registry.hasClass("MigrationTest"));
        REQUIRE(registry.hasClass("AnotherClass"));
    }
}
