#pragma once

#include "stmt.h"
#include "expr.h"
#include "type.h"

namespace sonic::frontend {
  using namespace std;

  class ASTPrinter {
  public:
    int indent_level;
    ASTPrinter() : indent_level(0) {}
    ~ASTPrinter() = default;

    void print_indent();
    void print(std::string v);
    void print_stmt(const Stmt* stmt);
    void print_expr(const Expr* expr);
    void print_type(const Type* type);

    void debug_location(SourceLocation location);
  };
}
