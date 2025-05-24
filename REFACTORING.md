# VoidScript Refactoring

## Symbol Management Simplification

This refactoring simplifies the class, symbol, function, and method storage in VoidScript by:

1. Removing `UnifiedModuleHelper` and the class registry
2. Storing classes, class methods, and functions directly in the `SymbolContainer`
3. Ensuring the parser only uses operations instead of direct calls to methods, functions, or classes

### Key Changes

#### 1. Enhanced SymbolContainer

The `SymbolContainer` class has been enhanced to handle all symbol types:
- Added class registry functionality
- Added function registry functionality
- Added method registry functionality
- Preserved the existing documentation system using macros

#### 2. Registration Macros

The registration macros have been updated to use the enhanced `SymbolContainer`:
- `REGISTER_FUNCTION`
- `REGISTER_CLASS`
- `REGISTER_METHOD`
- `REGISTER_PROPERTY`

#### 3. Parser Updates

The Parser has been updated to:
- Use `SymbolContainer` instead of `ClassRegistry`
- Ensure all method and function calls go through operations

#### 4. Module System

The module system has been simplified:
- Removed `UnifiedModuleManager`
- Each module now registers its functions directly with `SymbolContainer`
- Updated `BaseModule` to include a `registerFunctions` method

### Benefits

1. **Simplified Architecture**: Reduced the number of components involved in symbol management
2. **Improved Maintainability**: Centralized symbol storage in one container
3. **Consistent API**: All symbols are accessed through the same interface
4. **Better Encapsulation**: Parser no longer directly calls methods or functions

### Migration Notes

When updating existing modules:
1. Replace `registerModule()` with `registerFunctions()`
2. Update parameter types from `FunctParameterInfo` to `Symbols::FunctionParameterInfo`
3. Remove references to `UnifiedModuleManager`
4. Use `SymbolContainer` for all symbol operations

## Refactoring Task List

### Core Components
- [x] Create enhanced `SymbolContainer` with class and method registration capabilities
- [x] Create new `RegistrationMacros.hpp` that use the enhanced `SymbolContainer`
- [x] Update `OperationsFactory.hpp` to use `SymbolContainer` for operations
- [x] Remove `UnifiedModuleManager.hpp` and `UnifiedModuleManager.cpp`
- [x] Remove `ClassRegistry.hpp` and `ClassRegistry.cpp`
- [x] Update `VoidScript.hpp` to remove references to `UnifiedModuleManager`
- [x] Update `BaseModule.hpp` to replace `registerModule()` with `registerFunctions()`

### Parser and Interpreter
- [x] Update `Parser.cpp` to use `SymbolContainer` instead of `ClassRegistry`
- [x] Update `Parser.hpp` to remove `ClassRegistry` includes
- [x] Update `Interpreter.cpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [x] Update `Value.cpp` to use `SymbolContainer` instead of `UnifiedModuleManager` and `ClassRegistry`

### Expression Nodes
- [x] Update `CallExpressionNode.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager` and `ClassRegistry`
- [x] Update `MethodCallExpressionNode.hpp` to use `SymbolContainer` instead of `ClassRegistry`
- [x] Update `NewExpressionNode.hpp` to use `SymbolContainer` instead of `ClassRegistry`
- [x] Update `MemberExpressionNode.hpp` to use `SymbolContainer` instead of `ClassRegistry`

### Statement Nodes
- [x] Update `ClassDefinitionStatementNode.hpp` to use `SymbolContainer` instead of `ClassRegistry`
- [x] Update `CallStatementNode.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager` and `ClassRegistry`

### Modules
- [x] Update `PrintModule.hpp` to use the new registration system
- [x] Update `ModuleHelperModule.hpp` and `ModuleHelperModule.cpp` to use `SymbolContainer`
- [ ] Update `JsonModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [ ] Update `StringModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [ ] Update `HeaderModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [ ] Update `ArrayModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [ ] Update `FileModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`
- [ ] Update `VariableHelpersModule.hpp` to use `SymbolContainer` instead of `UnifiedModuleManager`

### Cleanup
- [ ] Remove `ClassMigration.hpp` if no longer needed
- [ ] Remove `ClassContainerAdapter.hpp` and `ClassContainerAdapter.cpp`
- [ ] Update CMakeLists.txt to remove references to removed files

### Testing
- [ ] Run all tests to ensure functionality is preserved
- [ ] Fix any issues that arise during testing