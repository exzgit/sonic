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

      unique_ptr<Stmt> parse_stmt();

      unique_ptr<Type> parse_type();
      unique_ptr<Expr> parse_expression(int min_prec);
      unique_ptr<Expr> parse_value_literal();

      int parse_precedence(TokenKind kind);

      bool accept(TokenKind kind);
      bool consume(TokenKind kind);

    private:
      Lexer* lexer;
      string files;

      Token tok;
  };
};
