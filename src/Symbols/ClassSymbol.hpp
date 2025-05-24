// ClassSymbol.hpp
#ifndef CLASS_SYMBOL_HPP
#define CLASS_SYMBOL_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "BaseSymbol.hpp"
#include "Symbols/VariableTypes.hpp"

namespace Symbols {

class ClassSymbol : public Symbol {
private:
    std::string parentClass_; // Name of the parent class if any
    bool isAbstract_;         // Whether the class is abstract

public:
    ClassSymbol(const std::string & name, const std::string & context, 
                const std::string & parentClass = "", bool isAbstract = false) :
        Symbol(name, "", context, Symbols::Kind::Class),
        parentClass_(parentClass),
        isAbstract_(isAbstract) {}

    Symbols::Kind kind() const override { return Symbols::Kind::Class; }
    
    const std::string & parentClass() const { return parentClass_; }
    
    bool isAbstract() const { return isAbstract_; }
    
    // Dump class symbol: name, context, parent class, etc.
    std::string dump() const override {
        std::string r = "\t\t  " + kindToString(this->kind_) + " name: '" + name_ + "' \n\t\t\tContext: " + context_;
        if (!parentClass_.empty()) {
            r += "\n\t\t\tParent Class: " + parentClass_;
        }
        if (isAbstract_) {
            r += "\n\t\t\tAbstract: true";
        }
        return r;
    }
};

} // namespace Symbols

#endif
