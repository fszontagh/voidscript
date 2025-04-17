#ifndef STATEMENT_NODE_HPP
#define STATEMENT_NODE_HPP

#include <string>

namespace Interpreter {

class StatementNode {
  public:
    std::string filename_;
    int         line_;
    size_t      column_;

    StatementNode(const std::string & file_name, int file_line, size_t line_column) :
        filename_(file_name),
        line_(file_line),
        column_(line_column) {}

    virtual ~StatementNode()                                      = default;
    virtual void interpret(class Interpreter & interpreter) const = 0;
    virtual std::string toString() const = 0;
};

};  // namespace Interpreter

#endif  // STATEMENT_NODE_HPP
