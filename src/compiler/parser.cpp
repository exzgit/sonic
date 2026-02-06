// c++ library
#include <memory>

// local header
#include "parser.h"
#include "ast.h"
#include "diagnostics.h"
#include "io.h"
#include "token.h"

namespace sonic::frontend {

  Parser::Parser(const std::string& filepath, Lexer* lexer) : lexer(lexer), filepath(filepath) {
    previous_token = nullptr;
    current_token = new Token(lexer->next_token());
  }

  std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    program->name_ = io::cutPath(filepath, "src");

    while (!match(TokenType::ENDOFFILE)) {
      program->statements_.push_back(parse_stmt());
    }

    return program;
  }

  std::unique_ptr<Statement> Parser::parse_stmt() {
    auto stmt = std::make_unique<Statement>();
    stmt->loc_ = current_token->location;

    if (match(TokenType::IDENT)) {
      return parse_assignment();
    }
    else if (match(TokenType::IF)) {
      next();
      stmt->kind_ = StmtKind::IF_ELSE;
      stmt->loc_ = previous_token->location;

      stmt->value_ = parse_expr();

      expect(TokenType::LEFTBRACE);

      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->then_.push_back(parse_stmt());

      expect(TokenType::RIGHTBRACE);
      if (match(TokenType::ELSE)) {
        next();
        if (match(TokenType::IF)) {
          stmt->else_.push_back(parse_stmt());
        } else {
          expect(TokenType::LEFTBRACE);
          while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->else_.push_back(parse_stmt());
          expect(TokenType::RIGHTBRACE);
        }
      }
      return stmt;
    }
    else if (match(TokenType::WHILE)) {
      next();
      stmt->kind_ = StmtKind::WHILE_LOOP;
      stmt->loc_ = previous_token->location;

      stmt->value_ = parse_expr();

      expect(TokenType::LEFTBRACE);

      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->body_.push_back(parse_stmt());

      expect(TokenType::RIGHTBRACE);
      return stmt;
    }
    else if (match(TokenType::FOR)) {
      next();
      stmt->kind_ = StmtKind::FOR_LOOP;
      stmt->loc_ = previous_token->location;

      stmt->assign_ = parse_expr();

      expect(TokenType::IN);

      stmt->value_ = parse_value();
      if (match(TokenType::RANGE)) {
        next();
        auto iterator = std::make_unique<Expression>();
        iterator->kind_ = ExprKind::RANGE;
        iterator->loc_ = stmt->value_->loc_;
        iterator->lhs_ = std::move(stmt->value_);
        iterator->rhs_ = parse_expr();
        stmt->value_ = std::move(iterator);
      }

      expect(TokenType::LEFTBRACE);

      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->body_.push_back(parse_stmt());

      expect(TokenType::RIGHTBRACE);
      return stmt;
    }
    else if (match(TokenType::TRY)) {
      next();
      stmt->kind_ = StmtKind::TRY_CATCH;
      stmt->loc_ = previous_token->location;

      expect(TokenType::LEFTBRACE);

      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->body_.push_back(parse_stmt());

      expect(TokenType::RIGHTBRACE);
      expect(TokenType::CATCH);

      stmt->value_ = std::make_unique<Expression>();
      stmt->value_->kind_ = ExprKind::VARIABLE;
      stmt->value_->name_ = expect(TokenType::IDENT)->value;

      expect(TokenType::LEFTBRACE);
      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->body_.push_back(parse_stmt());
      expect(TokenType::RIGHTBRACE);

      if (match(TokenType::FINALLY)) {
        expect(TokenType::LEFTBRACE);
        while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) stmt->body_.push_back(parse_stmt());
        expect(TokenType::RIGHTBRACE);
      }

      return stmt;
    }
    else if (match(TokenType::IMPORT)) {
      stmt->kind_ = StmtKind::IMPORT;
      stmt->loc_ = current_token->location;
      next();

      auto importPath = std::make_unique<Statement>();
      importPath->kind_ = StmtKind::IMPORT_FIELD;
      importPath->name_ = expect(TokenType::IDENT)->value;
      importPath->loc_ = previous_token->location;
      stmt->import_qualified_.push_back(std::move(importPath));

      while (match(TokenType::COLON_COLON)) {
        next();

        importPath = std::make_unique<Statement>();
        importPath->kind_ = StmtKind::IMPORT_FIELD;
        importPath->name_ = expect(TokenType::IDENT)->value;
        importPath->loc_ = previous_token->location;
        stmt->import_qualified_.push_back(std::move(importPath));
      }

      expect(TokenType::USE);
      expect(TokenType::LEFTBRACE);

      while(!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
        if (match(TokenType::STAR)) {
          next();
          stmt->import_all_ = true;
          break;
        }

        auto importItem = std::make_unique<Statement>();
        importItem->import_all_ = true;
        importItem->kind_ = StmtKind::IMPORT_ITEM;
        importItem->name_ = expect(TokenType::IDENT)->value;
        importItem->loc_ = previous_token->location;
        if (match(TokenType::ALIAS)) {
          next();
          importItem->import_alias_ = expect(TokenType::IDENT)->value;
        }

        stmt->import_items_.push_back(std::move(importItem));
        if (!match(TokenType::COMMA)) break;
        next();
      }

      expect(TokenType::RIGHTBRACE);

      skip_semicolon();
      return stmt;
    } else if (match(TokenType::PUBLIC)) {
      next();
      stmt->public_ = true;
    }

    if (match(TokenType::EXTERN)) {
      next();
      stmt->extern_ = true;
    }

    if (match(TokenType::STATIC) || match(TokenType::CONST)) {
      // MARK: Parse Static || Constant Declaration
      stmt->kind_ = StmtKind::VARIABLE;
      stmt->mutability = match(TokenType::STATIC) ? Mutability::STATIC : Mutability::CONSTANT;
      stmt->loc_ = current_token->location;
      next();

      stmt->name_ = expect(TokenType::IDENT)->value;
      expect(TokenType::COLON);
      stmt->type_ = parse_type();
      expect(TokenType::EQUAL);
      stmt->value_ = parse_expr();

      skip_semicolon();
    } else if (match(TokenType::LET)) {
      // MARK: Parse Variable Declaration
      next();

      stmt->kind_ = StmtKind::VARIABLE;
      stmt->loc_ = current_token->location;

      stmt->name_ = expect(TokenType::IDENT)->value;

      if (match(TokenType::COLON)) {
        next();
        stmt->type_ = parse_type();
      }

      if (match(TokenType::EQUAL)) {
        next();
        stmt->value_ = parse_expr();
        if (!stmt->value_) {
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            previous_token->location,
            "expected value after '='"
          });
        }
      } else stmt->declare_ = true;

      skip_semicolon();
    } else if (match(TokenType::FUNCT)) {
      // MARK: Parse Function Declaration
      next();

      stmt->kind_ = StmtKind::FUNCTION;
      stmt->loc_ = current_token->location;

      stmt->name_ = expect(TokenType::IDENT)->value;

      if (match(TokenType::LESS)) {
        next();
        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          auto generic = std::make_unique<Statement>();
          generic->kind_ = StmtKind::GENERICS;
          generic->name_ = expect(TokenType::IDENT)->value;

          generic->loc_ = previous_token->location;

          if (match(TokenType::COLON)) {
            next();
            generic->type_ = parse_type();
          }

          stmt->generics_.push_back(std::move(generic));

          if (!match(TokenType::COMMA)) break;
          next();
        }
        expect(TokenType::GREATER);
      }

      expect(TokenType::LEFTPAREN);

      while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
        auto param = std::make_unique<Statement>();
        param->kind_ = StmtKind::PARAMETER;

        param->name_ = expect(TokenType::IDENT)->value;
        param->loc_ = previous_token->location;

        expect(TokenType::COLON);
        param->type_ = parse_type();
        stmt->params_.push_back(std::move(param));

        if (!match(TokenType::COMMA)) break;
        next();

        if (match(TokenType::VARIADIC)) {
          next();
          stmt->variadic_ = true;
          break;
        }
      }

      expect(TokenType::RIGHTPAREN);

      if (match(TokenType::ARROW)) {
        next();
        stmt->type_ = parse_type();
      }

      if (match(TokenType::LEFTBRACE)) {
        next();

        while(!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
          stmt->body_.push_back(parse_stmt());
        }

        expect(TokenType::RIGHTBRACE);
        stmt->declare_ = false;
      } else {
        stmt->declare_ = true;
        skip_semicolon();
      }
    } else if (match(TokenType::RETURN)) {
      stmt->kind_ = StmtKind::RETURN;
      stmt->loc_ = current_token->location;
      next();
      stmt->value_ = parse_expr();
      skip_semicolon();
    }
    else {
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


  std::unique_ptr<Statement> Parser::parse_assignment() {
    auto expr = std::make_unique<Expression>();
    expr->loc_ = current_token->location;
    expr->name_ = expect(TokenType::IDENT)->value;
    expr->kind_ = ExprKind::VARIABLE;

    auto stmt = std::make_unique<Statement>();
    stmt->kind_ = StmtKind::EXPR;

    bool is_call = false;

    while (true) {
      if (match(TokenType::LEFTBRACKET)) {
        next();
        auto index = std::make_unique<Expression>();
        index->kind_ = ExprKind::INDEX;
        index->loc_ = current_token->location;
        index->nested_ = expr->clone();
        index->index_ = parse_expr();
        expect(TokenType::RIGHTBRACKET);

        expr = std::move(index);
      }

      if (match(TokenType::EQUAL)) {
        next();
        stmt->kind_ = StmtKind::ASSIGNMENT;
        stmt->name_ = expr->name_;
        stmt->assign_ = std::move(expr);
        stmt->value_ = parse_expr();
        if (!stmt->value_) {
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            previous_token->location,
            "expected value after '='"
          });
        }
        stmt->loc_ = previous_token->location;
        skip_semicolon();
        return stmt;
      } else if (match(TokenType::IS_EQUAL) || match(TokenType::NOT_EQUAL) ||
                 match(TokenType::PLUS_EQUAL) || match(TokenType::MINUS_EQUAL) ||
                 match(TokenType::STAR_EQUAL) || match(TokenType::DIV_EQUAL) ||
                 match(TokenType::PERCENT_EQUAL) || match(TokenType::POWER_EQUAL)) {
        std::string op = current_token->value;
        next();
        stmt->kind_ = StmtKind::ASSIGNMENT;
        stmt->assign_ = expr->clone();
        stmt->value_ = std::make_unique<Expression>();
        stmt->value_->kind_ = ExprKind::BINARY;
        stmt->value_->value_ = op;
        stmt->value_->lhs_ = std::move(expr);
        stmt->value_->rhs_ = parse_expr();
        if (!stmt->value_->rhs_) {
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            previous_token->location,
            "expected value after '" + op + "'"
          });
        }
        skip_semicolon();
        return stmt;
      }

      if (match(TokenType::LESS)) {
        next();
        auto generic = std::make_unique<Expression>();
        generic->kind_ = ExprKind::CALL;
        generic->loc_ = previous_token->location;
        generic->callee_ = std::move(expr);

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->generics_.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->args_.push_back(parse_expr());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(generic);
        continue;
      }

      if (match(TokenType::LEFTPAREN)) {
        auto call = std::make_unique<Expression>();
        call->loc_ = previous_token->location;
        call->kind_ = ExprKind::CALL;
        call->callee_ = std::move(expr);
        next();
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->args_.push_back(parse_expr());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(call);
      }

      if (match(TokenType::DOT)) {
        next();
        auto lookup = std::make_unique<Expression>();
        lookup->kind_ = ExprKind::MEMBER;
        lookup->loc_ = current_token->location;
        lookup->name_ = expect(TokenType::IDENT)->value;
        lookup->nested_ = std::move(expr);

        expr = std::move(lookup);
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto lookup_module = std::make_unique<Expression>();
        lookup_module->kind_ = ExprKind::SCOPE;
        lookup_module->loc_ = current_token->location;
        lookup_module->name_ = expect(TokenType::IDENT)->value;
        lookup_module->nested_ = std::move(expr);

        expr = std::move(lookup_module);
        continue;
      }

      break;
    }

    skip_semicolon();
    stmt->value_ = std::move(expr);
    return stmt;
  }

  std::unique_ptr<Expression> Parser::parse_expr() {
    return parse_binop(0);
  }

  std::unique_ptr<Expression> Parser::parse_binop(int prec) {
    auto left = parse_value();

    for (;;) {
      int precedence = parse_precedence(current_token->type);

      if (precedence < prec || precedence == 0) break;

      Token* op = current_token;

      next();

      auto bin = std::unique_ptr<Expression>();
      bin->kind_ = ExprKind::BINARY;
      bin->value_ = op->value;
      bin->lhs_ = left->clone();
      bin->rhs_ = parse_binop(precedence + 1);
      left = std::move(bin);
    }

    return left;
  }

  std::unique_ptr<Expression> Parser::parse_value() {
    auto expr = std::make_unique<Expression>();

    if (match(TokenType::MINUS)) {
      next();
      expr->kind_ = ExprKind::UNARY;
      expr->value_ = previous_token->value;
      expr->nested_ = parse_expr();
      return expr;
    }

    if (match(TokenType::NONE)) {
      next();

      expr->value_ = std::move(previous_token->value);
      expr->loc_ = previous_token->location;
      expr->kind_ = ExprKind::NONE;

      return expr;
    }
    else if (match(TokenType::NUMBER)) {
      next();

      expr->value_ = std::move(previous_token->value);
      expr->raw_ = expr->value_;
      expr->loc_ = previous_token->location;
      bool isFloat = expr->value_.find('.') != std::string::npos;
      expr->kind_ = ExprKind::LITERAL;
      expr->literal_ = isFloat ? LiteralKind::UNK_FLOAT : LiteralKind::UNK_INT;

      return expr;
    }
    else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
      next();

      expr->value_ = std::move(previous_token->value);
      expr->loc_ = previous_token->location;
      expr->kind_ = ExprKind::LITERAL;
      expr->literal_ = LiteralKind::BOOL;

      return expr;
    }
    else if (match(TokenType::CHARLIT)) {
      next();

      expr->value_ = std::move(previous_token->value);
      expr->loc_ = previous_token->location;
      expr->kind_ = ExprKind::LITERAL;
      expr->literal_ = LiteralKind::CHAR;

      return expr;
    }
    else if (match(TokenType::STRLIT)) {
      next();

      expr->value_ = std::move(previous_token->value);
      expr->loc_ = previous_token->location;
      expr->kind_ = ExprKind::LITERAL;
      expr->literal_ = LiteralKind::STRING;

      return expr;
    }

    if (match(TokenType::STAR)) {
      next();
      expr->kind_ = ExprKind::DEREF;
      expr->value_ = previous_token->value;
      expr->loc_ = previous_token->location;
      expr->nested_ = parse_expr();
      return expr;
    } else if (match(TokenType::AMPERSAND)) {
      next();
      expr->kind_ = ExprKind::REF;
      expr->value_ = previous_token->value;
      expr->loc_ = previous_token->location;
      expr->nested_ = parse_expr();
      return expr;
    }

    if (match(TokenType::IDENT)) {
      return parse_identifiers();
    }

    return nullptr;
  }

  std::unique_ptr<Expression> Parser::parse_identifiers() {
    auto expr = std::make_unique<Expression>();
    expr->kind_ = ExprKind::VARIABLE;
    expr->name_ = expect(TokenType::IDENT)->value;
    expr->loc_ = previous_token->location;

    bool is_call = false;

    for (;;) {
      if (match(TokenType::ENDOFFILE)) break;

      if (match(TokenType::LESS)) {
        next();
        auto generic = std::make_unique<Expression>();
        generic->kind_ = ExprKind::CALL;
        generic->loc_ = expr->loc_;
        generic->callee_ = std::move(expr);

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->generics_.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->args_.push_back(parse_expr());
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(generic);
        continue;
      } else if (match(TokenType::LEFTPAREN)) {
        auto call = std::make_unique<Expression>();
        call->loc_ = expr->loc_;
        call->kind_ = ExprKind::CALL;
        call->callee_ = std::move(expr);
        next();
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->args_.push_back(parse_expr());
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::RIGHTPAREN);
        expr = std::move(call);
      }

      if (match(TokenType::LEFTBRACKET)) {
        next();

        auto index = std::make_unique<Expression>();
        index->kind_ = ExprKind::INDEX;
        index->nested_ = std::move(expr);
        index->index_ = parse_expr();
        expect(TokenType::RIGHTBRACKET);
        expr = std::move(index);
      }

      if (match(TokenType::DOT)) {
        next();
        auto lookup = std::make_unique<Expression>();
        lookup->kind_ = ExprKind::MEMBER;
        lookup->loc_ = current_token->location;
        lookup->name_ = expect(TokenType::IDENT)->value;
        lookup->nested_ = std::move(expr);

        expr = std::move(lookup);
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto lookup_module = std::make_unique<Expression>();
        lookup_module->kind_ = ExprKind::SCOPE;
        lookup_module->loc_ = current_token->location;
        lookup_module->name_ = expect(TokenType::IDENT)->value;
        lookup_module->nested_ = std::move(expr);

        expr = std::move(lookup_module);
        continue;
      }

      break;
    }

    return expr;
  }

  std::unique_ptr<Type> Parser::parse_type() {
    auto type = std::make_unique<Type>();
    type->loc_ = current_token->location;
    type->kind_ = TypeKind::LITERAL;

    if (match(TokenType::I32)) {
      next();
      type->literal_ = LiteralKind::I32;
    } else if (match(TokenType::I64)) {
      next();
      type->literal_ = LiteralKind::I64;
    } else if (match(TokenType::I128)) {
      next();
      type->literal_ = LiteralKind::I128;
    } else if (match(TokenType::F32)) {
      next();
      type->literal_ = LiteralKind::F32;
    } else if (match(TokenType::F64)) {
      next();
      type->literal_ = LiteralKind::F64;
    } else if (match(TokenType::CHAR)) {
      next();
      type->literal_ = LiteralKind::CHAR;
    } else if (match(TokenType::STR)) {
      next();
      type->literal_ = LiteralKind::STRING;
    } else if (match(TokenType::BOOL)) {
      next();
      type->literal_ = LiteralKind::BOOL;
    } else if (match(TokenType::IDENT)) {
      next();
      type->kind_ = TypeKind::OBJECT;
      type->name_ = previous_token->value;

      for (;;) {
        if (match(TokenType::LESS)) {
          next();
          while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
            type->generics_.push_back(parse_type());
            if (!match(TokenType::COMMA)) break;
            next();
          }

          expect(TokenType::GREATER);
          break;
        }

        if (match(TokenType::COLON_COLON)) {
          next();
          expect(TokenType::IDENT);

          auto nested = std::make_unique<Type>();
          nested->kind_ = TypeKind::SCOPE;
          nested->name_ = previous_token->value;
          nested->loc_ = previous_token->location;
          nested->nested_ = std::move(type);

          type = std::move(nested);
          continue;
        }

        break;
      }
    }

    if (match(TokenType::QUESTION)) {
      type->nullable_ = true;
      next();
    } else if (match(TokenType::STAR)) {
      auto ptrType = std::make_unique<Type>();
      ptrType->kind_ = TypeKind::PTR;
      ptrType->nested_ = std::move(type);
      ptrType->loc_ = current_token->location;
      type = std::move(ptrType);
      next();
    } else if (match(TokenType::AMPERSAND)) {
      auto refType = std::make_unique<Type>();
      refType->kind_ = TypeKind::REF;
      refType->nested_ = std::move(type);
      refType->loc_ = current_token->location;
      type = std::move(refType);
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
