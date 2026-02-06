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
  using namespace ast;

  class Parser {
  public:
    Parser(const std::string& filepath, Lexer* lexer);
    ~Parser() = default;

    std::unique_ptr<Program> parse();

    DiagnosticEngine* diag = nullptr;
  private:
    Lexer* lexer;

    Token* current_token = nullptr;
    Token* previous_token = nullptr;

    std::string filepath;

    bool is_extern = false;

  private:
    std::unique_ptr<Statement> parse_stmt();
    std::unique_ptr<Statement> parse_assignment();

    std::unique_ptr<Expression> parse_expr();
    std::unique_ptr<Expression> parse_binop(int prec);
    std::unique_ptr<Expression> parse_value();
    std::unique_ptr<Expression> parse_identifiers();

    std::unique_ptr<Type> parse_type();

    void skip_semicolon();

    int parse_precedence(TokenType t);

    bool match(TokenType type);

    Token* expect(TokenType type);

    void next();
  };
}
