#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sstream>

#include "ast/span.h"
#include "ast/stmt.h"
#include "ast/value.h"
#include "ast/type.h"

#include "lexer.h"

#include "../core/token.h"
#include "../core/error.h"

using namespace std;
using namespace errors;

namespace frontend {
  class Parser {
    public:
      Parser(string filepath, Lexer* lexer);
      ~Parser() = default;

      unique_ptr<Stmt> parse();

    private:

      unique_ptr<Stmt> parse_block();
      unique_ptr<Stmt> parse_stmt(bool at_local_scope);

      unique_ptr<Type> parse_type();
      unique_ptr<Expr> parse_expression(int min_prec);
      unique_ptr<Expr> parse_value_literal();
      unique_ptr<Expr> parse_postfix_from_expr(unique_ptr<Expr> expr);

      void report_error(const std::string& message);
      void synchronize();

      int parse_precedence(TokenKind kind);

      bool accept(TokenKind kind);
      bool consume(TokenKind kind);

    private:
      string files;

      Lexer* lexer;
      Token tok;
  };
};
