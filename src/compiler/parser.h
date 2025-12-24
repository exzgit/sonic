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

      vector<unique_ptr<Stmt>> parse();

    private:

      unique_ptr<Stmt> parse_stmt();

      unique_ptr<Type> parse_type();
      unique_ptr<Stmt> parse_assignment();
      unique_ptr<Stmt> parse_function_stmt(bool is_public);
      unique_ptr<Expr> parse_expression();
      unique_ptr<Expr> parse_value_literal();
      unique_ptr<Expr> parse_binary_expr(int min_prec);

      int parse_precedence(TokenKind kind);

      bool accept(TokenKind kind);
      bool consume(TokenKind kind);

    private:
      Lexer* lexer;
      string files;

      Token tok;
  };
};
