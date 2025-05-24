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