// ScriptFunction.hpp
#ifndef SCRIPT_FUNCTION_HPP
#define SCRIPT_FUNCTION_HPP

#include <vector>

#include "Token.hpp"
#include "Value.hpp"


class SScriptInterpreter;

class BaseFunction {
  protected:
    std::string name;
  public:
    virtual ~BaseFunction()                                                       = default;
    virtual void  validate(const std::vector<Token> & tokens, size_t & i) const   = 0;
    virtual Value call(const std::vector<Value> & args, bool debug = false) const = 0;

    template <typename FuncClass> void registerFunctionTo(SScriptInterpreter & interp) {
        FuncClass::registerTo(interp);
    }
};

#endif  // SCRIPT_FUNCTION_HPP
