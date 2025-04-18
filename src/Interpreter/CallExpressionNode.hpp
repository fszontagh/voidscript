 #ifndef INTERPRETER_CALL_EXPRESSION_NODE_HPP
 #define INTERPRETER_CALL_EXPRESSION_NODE_HPP

 #include <string>
 #include <vector>
 #include <memory>

 #include "Interpreter/ExpressionNode.hpp"
 #include "Interpreter/Interpreter.hpp"
 #include "Interpreter/ReturnException.hpp"
 #include "Interpreter/OperationContainer.hpp"
 #include "Interpreter/Operation.hpp"
 #include "Symbols/SymbolContainer.hpp"
 #include "Symbols/SymbolFactory.hpp"
 #include "Symbols/FunctionSymbol.hpp"
 #include "Symbols/Value.hpp"
 #include "Modules/ModuleManager.hpp"

 namespace Interpreter {

 /**
  * @brief Expression node representing a function call returning a value.
  */
 class CallExpressionNode : public ExpressionNode {
     std::string functionName_;
     std::vector<std::unique_ptr<ExpressionNode>> args_;

   public:
     CallExpressionNode(std::string functionName,
                        std::vector<std::unique_ptr<ExpressionNode>> args) :
         functionName_(std::move(functionName)), args_(std::move(args)) {}

     Symbols::Value evaluate(Interpreter &interpreter) const override {
         using namespace Symbols;
         // Evaluate argument expressions
         std::vector<Value> argValues;
         argValues.reserve(args_.size());
         for (const auto &expr : args_) {
             argValues.push_back(expr->evaluate(interpreter));
         }

         // Built-in function
         auto &mgr = Modules::ModuleManager::instance();
         if (mgr.hasFunction(functionName_)) {
             return mgr.callFunction(functionName_, argValues);
         }

         // User-defined function
         SymbolContainer *sc = Symbols::SymbolContainer::instance();
         const std::string currentNs = sc->currentScopeName();
         const std::string fnSymNs = currentNs + ".functions";
         auto sym = sc->get(fnSymNs, functionName_);
         if (!sym || sym->getKind() != Kind::Function) {
             throw std::runtime_error("Function not found: " + functionName_);
         }
         auto funcSym = std::static_pointer_cast<FunctionSymbol>(sym);
         const auto &params = funcSym->parameters();
         if (params.size() != argValues.size()) {
             throw std::runtime_error(
                 "Function '" + functionName_ + "' expects " + std::to_string(params.size()) +
                 " args, got " + std::to_string(argValues.size()));
         }

         // Enter function scope and bind parameters
         const std::string fnOpNs = currentNs + "." + functionName_;
         sc->enter(fnOpNs);
         for (size_t i = 0; i < params.size(); ++i) {
             const auto &p = params[i];
             const Value &v = argValues[i];
             auto varSym = SymbolFactory::createVariable(p.name, v, fnOpNs);
             sc->add(varSym);
         }

         // Execute function body operations and capture return
         Symbols::Value returnValue;
         auto ops = Operations::Container::instance()->getAll(fnOpNs);
         for (const auto &op : ops) {
             try {
                 interpreter.runOperation(*op);
             } catch (const ReturnException &ret) {
                 returnValue = ret.value();
                 break;
             }
         }
         sc->enterPreviousScope();
         return returnValue;
     }

     std::string toString() const override {
         return "CallExpressionNode{ function='" + functionName_ + "', args=" + std::to_string(args_.size()) + " }";
     }
 };

} // namespace Interpreter

 #endif // INTERPRETER_CALL_EXPRESSION_NODE_HPP