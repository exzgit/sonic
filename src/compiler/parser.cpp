// c++ library
#include <memory>

// local header
#include "parser.h"
#include "ast.h"
#include "diagnostics.h"
#include "token.h"

namespace sonic::frontend {

  Parser::Parser(const std::string& filename, Lexer* lexer) : lexer(lexer), filename(filename) {
    previous_token = nullptr;
    current_token = new Token(lexer->next_token());
  }

  std::unique_ptr<SonicStmt> Parser::parse() {
    auto program = std::make_unique<SonicStmt>();
    program->kind = StmtKind::PROGRAM;
    program->name = filename;
    program->location = current_token->location;

    while (!match(TokenType::ENDOFFILE)) {
      program->body.push_back(parse_stmt());
      if (match(TokenType::ENDOFFILE)) break;
    }

    return program;
  }

  std::unique_ptr<SonicStmt> Parser::parse_stmt() {
    auto stmt = std::make_unique<SonicStmt>();
    stmt->location = current_token->location;

    if (match(TokenType::IDENT)) {
      return parse_assignment();
    }

    // parse public syntax
    if (match(TokenType::PUBLIC)) {
      next();
      stmt->isPublic = true;
    }

    // parse extern syntax
    if (match(TokenType::EXTERN)) {
      next();
      stmt->isExtern = true;
    }

    if (match(TokenType::STATIC) || match(TokenType::CONST)) {
      // MARK: Parse Static || Constant Declaration
      next();
      stmt->kind = StmtKind::VAR_DECL;
      stmt->mutability = previous_token->type == TokenType::STATIC ? Mutability::STATIC : Mutability::CONSTANT;
      stmt->location = current_token->location;

      expect(TokenType::IDENT);
      stmt->name = previous_token->value;
      expect(TokenType::COLON);
      stmt->dataType = parse_type();
      expect(TokenType::EQUAL);
      stmt->expr = parse_expr();

      skip_semicolon();
    } else if (match(TokenType::LET)) {
      // MARK: Parse Variable Declaration
      next();

      stmt->kind = StmtKind::VAR_DECL;
      stmt->location = current_token->location;

      expect(TokenType::IDENT);
      stmt->name = previous_token->value;

      if (match(TokenType::COLON)) {
        next();
        stmt->dataType = parse_type();
      } else {
        stmt->dataType = nullptr;
      }

      if (match(TokenType::EQUAL)) {
        next();
        stmt->expr = parse_expr();
      }

      skip_semicolon();
    } else if (match(TokenType::FUNCT)) {
      // MARK: Parse Function Declaration
      next();

      stmt->kind = StmtKind::FUNC_DECL;
      stmt->location = current_token->location;

      expect(TokenType::IDENT);
      stmt->name = previous_token->value;

      if (match(TokenType::LESS)) {
        next();
        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          auto generic = std::make_unique<SonicType>();
          auto name = expect(TokenType::IDENT)->value;

          generic->location = previous_token->location;

          if (match(TokenType::COLON)) {
            next();
            generic->returnType = parse_type();
          }

          stmt->genericParams.push_back(std::move(generic));

          if (!match(TokenType::COMMA)) break;
          next();
        }
        expect(TokenType::GREATER);
      }

      expect(TokenType::LEFTPAREN);

      while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
        auto param = std::make_unique<FunctionParameter>();

        if (match(TokenType::VARIADIC)) {
          next();
          param->variadic = true;
        }

        param->name = expect(TokenType::IDENT)->value;
        param->location = previous_token->location;

        expect(TokenType::COLON);
        param->type = parse_type();

        if (!match(TokenType::COMMA)) break;
        next();
      }

      expect(TokenType::RIGHTPAREN);

      if (match(TokenType::ARROW)) {
        next();
        stmt->returnType = parse_type();
      } else {
        stmt->returnType = std::make_unique<SonicType>();
        stmt->returnType->kind = TypeKind::VOID;
      }

      if (match(TokenType::LEFTBRACE)) {
        next();

        while(!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
          stmt->body.push_back(parse_stmt());
        }

        expect(TokenType::RIGHTBRACE);
      } else {
        if (!stmt->isExtern) {
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            current_token->location,
            "expected function body after function declaration",
            "",
            "example: {\n  // function body\n}"
          });
        }
        skip_semicolon();
      }
    } else {
      diag->report({
        ErrorType::SYNTAX,
        Severity::ERROR,
        current_token->location,
        "unexpected syntax '" + current_token->value + "'"
      });
      next();
    }

    return stmt;
  }

  int Parser::parse_precedence(TokenType t) {
    switch (t) {
      // paling tinggi
      case TokenType::POWER:          // **
        return 7;

      case TokenType::STAR:           // *
      case TokenType::DIV:            // /
      case TokenType::PERCENT:        // %
        return 6;

      case TokenType::PLUS:           // +
      case TokenType::MINUS:          // -
        return 5;

      case TokenType::LESS:           // <
      case TokenType::LESS_EQUAL:     // <=
      case TokenType::GREATER:        // >
      case TokenType::GREATER_EQUAL:  // >=
        return 4;

      case TokenType::IS_EQUAL:       // ==
      case TokenType::NOT_EQUAL:      // !=
        return 3;

      case TokenType::AND:            // &&
        return 2;

      case TokenType::OR:             // ||
        return 1;

      default:
        return 0;
    }
  }


  std::unique_ptr<SonicStmt> Parser::parse_assignment() {

    auto expr = std::make_unique<SonicExpr>();
    expr->location = current_token->location;
    expr->name = expect(TokenType::IDENT)->value;
    expr->kind = ExprKind::IDENT;

    auto stmt = std::make_unique<SonicStmt>();
    stmt->kind = StmtKind::EXPR;

    bool is_call = false;

    while (true) {
      if (match(TokenType::LEFTBRACKET)) {
        next();
        auto index = std::make_unique<SonicExpr>();
        index->kind = ExprKind::INDEX;
        index->location = current_token->location;
        index->nested = expr->clone();
        index->value = parse_expr()->value;
        expect(TokenType::RIGHTBRACKET);

        expr = std::move(index);
      }

      if (match(TokenType::EQUAL)) {
        next();
        stmt->kind = StmtKind::ASSIGN;
        stmt->name = expr->name;
        stmt->target = std::move(expr);
        stmt->expr = parse_expr();
        stmt->location = previous_token->location;
        skip_semicolon();
        return stmt;
      } else if (match(TokenType::IS_EQUAL) || match(TokenType::NOT_EQUAL) ||
                 match(TokenType::PLUS_EQUAL) || match(TokenType::MINUS_EQUAL) ||
                 match(TokenType::STAR_EQUAL) || match(TokenType::DIV_EQUAL) ||
                 match(TokenType::PERCENT_EQUAL) || match(TokenType::POWER_EQUAL)) {
        next();
        stmt->kind = StmtKind::ASSIGN;
        stmt->target = expr->clone();
        stmt->expr = std::make_unique<SonicExpr>();
        stmt->expr->kind = ExprKind::BINARY;
        stmt->expr->lhs = std::move(expr);
        stmt->expr->rhs = parse_expr();
        stmt->expr->op = previous_token->value;
        skip_semicolon();
        return stmt;
      }

      if (match(TokenType::LESS)) {
        next();
        auto generic = std::make_unique<SonicExpr>();
        generic->location = previous_token->location;
        generic->kind = ExprKind::CALL;
        generic->callee = std::move(expr);

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->genericTypes.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->arguments.push_back(parse_expr());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(generic);
        continue;
      }

      if (match(TokenType::LEFTPAREN)) {
        next();
        auto call = std::make_unique<SonicExpr>();
        call->location = previous_token->location;
        call->kind = ExprKind::CALL;
        call->callee = std::move(expr);
        call->kind = ExprKind::CALL;
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->arguments.push_back(parse_expr());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(call);
      }

      if (match(TokenType::DOT)) {
        next();
        auto lookup = std::make_unique<SonicExpr>();
        lookup->kind = ExprKind::MEMBER_ACCESS;
        lookup->location = current_token->location;
        lookup->value = expect(TokenType::IDENT)->value;
        lookup->nested = std::move(expr);

        expr = std::move(lookup);
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto lookup_module = std::make_unique<SonicExpr>();
        lookup_module->kind = ExprKind::SCOPE_ACCESS;
        lookup_module->location = current_token->location;
        lookup_module->value = expect(TokenType::IDENT)->value;
        lookup_module->nested = std::move(expr);

        expr = std::move(lookup_module);
        continue;
      }

      break;
    }

    skip_semicolon();
    stmt->expr = std::move(expr);
    return stmt;
  }

  std::unique_ptr<SonicExpr> Parser::parse_expr() {
    return parse_binop(0);
  }

  std::unique_ptr<SonicExpr> Parser::parse_binop(int prec) {
    auto left = parse_value();

    for (;;) {
      int precedence = parse_precedence(current_token->type);

      if (precedence < prec || precedence == 0) break;

      Token* op = current_token;

      next();

      auto bin = std::unique_ptr<SonicExpr>();
      bin->kind = ExprKind::BINARY;
      bin->lhs = left->clone();
      bin->rhs = parse_binop(precedence + 1);
      bin->op = op->value;
      left = std::move(bin);
    }

    return left;
  }

  std::unique_ptr<SonicExpr> Parser::parse_value() {
    auto expr = std::make_unique<SonicExpr>();

    if (match(TokenType::MINUS)) {
      next();
      expr->kind = ExprKind::UNARY;
      expr->op = previous_token->value;
      expr->nested = parse_expr();
      return expr;
    }

    if (match(TokenType::NONE)) {
      next();

      expr->value = std::move(previous_token->value);
      expr->location = previous_token->location;
      expr->kind = ExprKind::NONE;

      return expr;
    }
    else if (match(TokenType::NUMBER)) {
      next();

      expr->value = std::move(previous_token->value);
      expr->rawValue = expr->value;
      expr->location = previous_token->location;
      expr->kind = ExprKind::UNTYPED_LITERAL;

      return expr;
    }
    else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
      next();

      expr->value = std::move(previous_token->value);
      expr->location = previous_token->location;
      expr->kind = ExprKind::BOOL;

      return expr;
    }
    else if (match(TokenType::CHARLIT)) {
      next();

      expr->value = std::move(previous_token->value);
      expr->location = previous_token->location;
      expr->kind = ExprKind::CHAR;

      return expr;
    }
    else if (match(TokenType::STRLIT)) {
      next();

      expr->value = std::move(previous_token->value);
      expr->location = previous_token->location;
      expr->kind = ExprKind::STRING;

      return expr;
    }

    if (match(TokenType::AMPERSAND) || match(TokenType::STAR)) {
      next();
      expr->kind = ExprKind::UNARY;
      expr->op = previous_token->value;
      if (match(TokenType::IDENT)) {
        expr->nested = parse_identifiers();
        return expr;
      }

      diag->report({
        ErrorType::SYNTAX,
        Severity::ERROR,
        current_token->location,
        "expected variable after '" + previous_token->value + "'"
        "",
        "example '" + previous_token->value + "x'"
      });
    }

    if (match(TokenType::IDENT)) {
      return parse_identifiers();
    }

    return nullptr;
  }

  std::unique_ptr<SonicExpr> Parser::parse_identifiers() {
    auto expr = std::make_unique<SonicExpr>();
    expr->kind = ExprKind::IDENT;
    expr->name = expect(TokenType::IDENT)->value;
    expr->location = previous_token->location;

    bool is_call = false;

    for (;;) {
      if (match(TokenType::ENDOFFILE)) break;

      if (match(TokenType::LESS)) {
        next();
        auto generic = std::make_unique<SonicExpr>();
        generic->location = previous_token->location;
        generic->kind = ExprKind::CALL;
        generic->callee = std::move(expr);

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->genericTypes.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->arguments.push_back(parse_expr());
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(generic);
        continue;
      } else if (match(TokenType::LEFTPAREN)) {
        next();
        auto call = std::make_unique<SonicExpr>();
        call->location = previous_token->location;
        call->kind = ExprKind::CALL;
        call->callee = std::move(expr);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->arguments.push_back(parse_expr());
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(call);
      }

      if (match(TokenType::LEFTBRACKET)) {
        next();

        auto index = std::make_unique<SonicExpr>();
        index->kind = ExprKind::INDEX;
        index->nested = std::move(expr);
        index->index = parse_expr();
        expect(TokenType::RIGHTBRACKET);
        expr = std::move(index);
      }

      if (match(TokenType::DOT)) {
        next();
        auto lookup = std::make_unique<SonicExpr>();
        lookup->kind = ExprKind::MEMBER_ACCESS;
        lookup->location = current_token->location;
        lookup->value = expect(TokenType::IDENT)->value;
        lookup->nested = std::move(expr);

        expr = std::move(lookup);
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto lookup_module = std::make_unique<SonicExpr>();
        lookup_module->kind = ExprKind::SCOPE_ACCESS;
        lookup_module->location = current_token->location;
        lookup_module->value = expect(TokenType::IDENT)->value;
        lookup_module->nested = std::move(expr);

        expr = std::move(lookup_module);
        continue;
      }

      break;
    }

    return expr;
  }

  std::unique_ptr<SonicType> Parser::parse_type() {
    auto type = std::make_unique<SonicType>();
    type->location = current_token->location;

    if (match(TokenType::I32)) {
      next();
      type->kind = TypeKind::I32;
    } else if (match(TokenType::I64)) {
      next();
      type->kind = TypeKind::I64;
    } else if (match(TokenType::I128)) {
      next();
      type->kind = TypeKind::I128;
    } else if (match(TokenType::F32)) {
      next();
      type->kind = TypeKind::F32;
    } else if (match(TokenType::F64)) {
      next();
      type->kind = TypeKind::F64;
    } else if (match(TokenType::CHAR)) {
      next();
      type->kind = TypeKind::CHAR;
    } else if (match(TokenType::STR)) {
      next();
      type->kind = TypeKind::STRING;
    } else if (match(TokenType::BOOL)) {
      next();
      type->kind = TypeKind::BOOL;
    } else if (match(TokenType::IDENT)) {
      next();
      type->kind = TypeKind::IDENT;
      type->name = previous_token->value;

      for (;;) {
        if (match(TokenType::LESS)) {
          next();
          while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
            type->params.push_back(parse_type());
            if (!match(TokenType::COMMA)) break;
            next();
          }

          expect(TokenType::GREATER);
          break;
        }

        if (match(TokenType::COLON_COLON)) {
          next();
          expect(TokenType::IDENT);

          auto nested = std::make_unique<SonicType>();
          nested->kind = TypeKind::SCOPE_ACCESS;
          nested->name = previous_token->value;
          nested->location = previous_token->location;
          nested->scope = std::move(type);

          type = std::move(nested);
          continue;
        }

        break;
      }
    }

    if (match(TokenType::QUESTION)) {
      next();
      type->isNullable = true;
    } else if (match(TokenType::STAR) || match(TokenType::AMPERSAND)) {
      type->isReference = match(TokenType::AMPERSAND);
      type->isPointer = match(TokenType::STAR);
      if (type->isPointer)
        type->isNullable = true;
      next();
    }

    return type;
  }

  void Parser::skip_semicolon() {
    if (match(TokenType::SEMICOLON)) {
      next();
    }
  }

  bool Parser::match(TokenType type) {
    if (current_token->type == type) {
      return true;
    }
    return false;
  }

  Token* Parser::expect(TokenType type) {
    if (match(type)) {
      next();
      return previous_token;
    }
    diag->report({
      ErrorType::SYNTAX,
      Severity::ERROR,
      current_token->location,
      "expected '" + tokenTypeToValue(type) + "', but got '" + tokenTypeToValue(current_token->type) + "'",
      "",
      ""
    });
    return current_token;
  }

  void Parser::next() {
    previous_token = current_token;
    current_token = new Token(lexer->next_token());
  }
}
