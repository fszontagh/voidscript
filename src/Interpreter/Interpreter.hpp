#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP
#include <memory>
#include <utility>
#include <vector>

#include "Interpreter/Operation.hpp"
#include "Interpreter/OperationContainer.hpp"
#include "Symbols/SymbolContainer.hpp"

namespace Interpreter {

class Interpreter {
  private:
    std::shared_ptr<Symbols::SymbolContainer> symbol_container_;
    const OperationContainer &                operations_conatiner_;

    void setVariable(const std::string & name, int value) {
        //symbol_container_->setVariable(name, value);
    }
  public:
    // Constructor takes the populated symbol container from the parser
    Interpreter(std::shared_ptr<Symbols::SymbolContainer> symbols, const OperationContainer & operations) :
        symbol_container_(std::move(symbols)),
        operations_conatiner_(operations) {}

    void run(const std::vector<Operation> & ops) {
        for (const auto & op : ops) {
            switch (op.type) {
                case OperationType::Assignment:
                    {
                        int value = op.expression->evaluate(*this);
                        setVariable(op.targetVariable, value);
                        break;
                    }

                case OperationType::Expression:
                    {
                        op.expression->evaluate(*this);  // csak side effect miatt
                        break;
                    }
            }
        }
    }

};  // class Interpreter

}  // namespace Interpreter
#endif  // INTERPRETER_HPP
