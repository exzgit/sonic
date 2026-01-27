#pragma once

#include <string>
#include "lexer.h"
#include "token.h"
#include "diagnostics.h"
#include "ast/stmt.h"
#include "ast/expr.h"
#include "ast/type.h"

namespace sonic::frontend {
  class Parser {
  public:
    Parser(const std::string& filename, Lexer* lexer);
    ~Parser() = default;

    Stmt* parse();

    DiagnosticEngine* diag = nullptr;
  private:
    Lexer* lexer;

    Token* current_token = nullptr;
    Token* previous_token = nullptr;

    std::string filename;

    bool is_extern = false;

  private:
    Stmt* parse_stmt();
    Type* parse_type();
    Stmt* parse_block();
    Expr* parse_expr(int prec);
    Expr* parse_value();
    Expr* parse_identifiers(Expr* e);
    Stmt* parse_assignment(Stmt* stmt);

    void skip_semicolon();

    int parse_precedence(TokenType t);


    bool match(TokenType type);

    Token* expect(TokenType type);

    void next();
  };
}
