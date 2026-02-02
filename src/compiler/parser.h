#pragma once

// c++ library
#include <string>
#include <memory>

// local headers
#include "lexer.h"
#include "token.h"
#include "diagnostics.h"
#include "ast.h"


namespace sonic::frontend {
  class Parser {
  public:
    Parser(const std::string& filename, Lexer* lexer);
    ~Parser() = default;

    std::unique_ptr<SonicStmt> parse();

    DiagnosticEngine* diag = nullptr;
  private:
    Lexer* lexer;

    Token* current_token = nullptr;
    Token* previous_token = nullptr;

    std::string filename;

    bool is_extern = false;

  private:
    std::unique_ptr<SonicStmt> parse_stmt();
    std::unique_ptr<SonicStmt> parse_assignment();

    std::unique_ptr<SonicExpr> parse_expr();
    std::unique_ptr<SonicExpr> parse_binop(int prec);
    std::unique_ptr<SonicExpr> parse_value();
    std::unique_ptr<SonicExpr> parse_identifiers();

    std::unique_ptr<SonicType> parse_type();

    void skip_semicolon();

    int parse_precedence(TokenType t);

    bool match(TokenType type);

    Token* expect(TokenType type);

    void next();
  };
}
