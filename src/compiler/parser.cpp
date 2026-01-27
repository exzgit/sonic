#include "parser.h"
#include "diagnostics.h"
#include "operand.h"
#include "stmt.h"
#include "token.h"
#include "type.h"
#include "../core/io.h"

namespace sonic::frontend {

  Parser::Parser(const std::string& filename, Lexer* lexer) : lexer(lexer), filename(filename) {
    previous_token = nullptr;
    current_token = new Token(lexer->next_token());
  }

  Stmt* Parser::parse() {
    Stmt* program = new Stmt();
    program->kind = StmtKind::PROGRAM;
    program->filename = filename;
    program->locations = current_token->location;

    while (!match(TokenType::ENDOFFILE)) {
      program->children.push_back(parse_stmt());
      if (match(TokenType::ENDOFFILE)) break;
    }

    return program;
  }

  Stmt* Parser::parse_stmt() {
    Stmt* stmt = new Stmt();

    if (match(TokenType::IMPORT)) {
      next();

      stmt->kind = StmtKind::IMPORT;
      stmt->locations = current_token->location;
      std::string path = expect(TokenType::IDENT)->value;
      stmt->importPath.push_back(path);

      auto loc = stmt->locations;

      while (match(TokenType::COLON_COLON)) {
        next();

        std::string pth = expect(TokenType::IDENT)->value;
        path += "/" + pth;
        stmt->importPath.push_back(pth);

        if (!match(TokenType::COLON_COLON)) break;
      }

      loc.start++;
      loc.end = previous_token->location.end + 1;

      if (match(TokenType::USE)) {
        next();

        if (match(TokenType::STAR)) {
          next();
          stmt->isImportAll = true;
        } else if (match(TokenType::LEFTBRACE)) {
          next();

          while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
            if (match(TokenType::STAR)) {
              next();
              stmt->isImportAll = true;
              break;
            }
            ImportItem item = ImportItem();
            item.locations = current_token->location;
            item.name = expect(TokenType::IDENT)->value;

            if (match(TokenType::ALIAS)) {
              next();
              item.useAlias = true;
              item.alias = expect(TokenType::IDENT)->value;
            }

            stmt->importItems.push_back(item);

            if (!match(TokenType::COMMA)) break;
            else next();
          }

          expect(TokenType::RIGHTBRACE);
        }
      } else {
        stmt->isImportAll = true;
      }

      skip_semicolon();

      return stmt;
    }

    if (match(TokenType::EXTERN) && !is_extern) {
      stmt->locations = current_token->location;
      is_extern = true;
      stmt->body = parse_block();
      is_extern = false;
    } else if (match(TokenType::EXTERN)) {
      diag->report({
        ErrorType::SYNTAX,
        Severity::ERROR,
        current_token->location,
        "unexpected syntax '" + current_token->value + "'",
        "do not use block extern inside block extern",
        ""
      });
      next();
    }

    if (match(TokenType::IF)) {
      next();
      stmt->kind = StmtKind::IFELSE;
      stmt->locations = current_token->location;
      stmt->value = parse_expr(0);
      stmt->then_block = parse_block();

      if (match(TokenType::ELSE)) {
        next();
        if (match(TokenType::IF)) {
          stmt->else_block = parse_stmt();
        } else {
          stmt->else_block = parse_block();
        }
      }

      return stmt;
    }

    if (match(TokenType::IDENT)) {
      stmt->kind = StmtKind::EXPRTOSTMT;
      stmt->locations = current_token->location;
      return parse_assignment(stmt);
    }

    stmt->isExtern = is_extern;

    if (match(TokenType::PUBLIC)) {
      stmt->locations = current_token->location;
      stmt->isPublic = true;
      next();

      if (!match(TokenType::STATIC) && !match(TokenType::CONST) && !match(TokenType::FUNCT) && !match(TokenType::STRUCT) && !match(TokenType::ENUM)) {
        diag->report({
          ErrorType::SYNTAX,
          Severity::ERROR,
          current_token->location,
          "unexpected syntax '" + current_token->value + "'",
          "expected 'static', 'const', 'func', 'struct', or 'enum' after 'public'",
          ""
        });
      }
    }

    if (match(TokenType::STRUCT)) {
      next();
      stmt->kind = StmtKind::STRUCT;
      stmt->locations = current_token->location;
      if (match(TokenType::IDENT)) {
        stmt->name = expect(TokenType::IDENT)->value;
      } else {
        expect(TokenType::IDENT);
        if (match(TokenType::LEFTBRACE) || match(TokenType::LESS)) {}
        else next();
      }

      if (match(TokenType::LESS)) {
        next();

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          auto* generic = new Generic();
          generic->name = expect(TokenType::IDENT)->value;
          if (match(TokenType::COLON)) {
            next();
            generic->type = parse_type();
          }
          stmt->genericType.push_back(generic);
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::GREATER);

        if (stmt->genericType.empty())
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            current_token->location,
            "expected generic type",
            "generic type cannot be empty"
          });
      }

      expect(TokenType::LEFTBRACE);
      while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
        StructField field;
        field.locations = current_token->location;
        field.name = expect(TokenType::IDENT)->value;
        expect(TokenType::COLON);
        field.type = parse_type();
        stmt->structFields.push_back(field);
        skip_semicolon();
      }
      expect(TokenType::RIGHTBRACE);

      return stmt;
    }

    if (match(TokenType::STATIC) || match(TokenType::CONST)) {
      stmt->kind = StmtKind::VARDECL;

      stmt->locations = current_token->location;
      stmt->isStatic = match(TokenType::STATIC);
      stmt->isConstant = match(TokenType::CONST);

      next();

      if (match(TokenType::IDENT)) {
        stmt->locations = current_token->location;
        stmt->name = expect(TokenType::IDENT)->value;

        expect(TokenType::COLON);
        stmt->dataType = parse_type();

        expect(TokenType::EQUAL);
        stmt->value = parse_expr(0);
        stmt->isDefine = true;

        skip_semicolon();
        return stmt;
      } else if (stmt->isConstant) {
        diag->report({
          ErrorType::SYNTAX,
          Severity::ERROR,
          current_token->location,
          "expected identifier after 'const'",
          "constant declaration must have an identifier",
          ""
        });
        return stmt;
      }
    }


    if (match(TokenType::FUNCT)) {
      stmt->kind = StmtKind::FUNCTION;
      next();
      stmt->locations = current_token->location;

      if (match(TokenType::IDENT)) {
        stmt->name = expect(TokenType::IDENT)->value;
      } else {
        expect(TokenType::IDENT);
        if (match(TokenType::LEFTPAREN) || match(TokenType::LESS)) {}
        else next();

      }

      if (match(TokenType::LESS)) {
        next();

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          auto* generic = new Generic();
          generic->name = expect(TokenType::IDENT)->value;
          if (match(TokenType::COLON)) {
            next();
            generic->type = parse_type();
          }
          stmt->genericType.push_back(generic);
          if (!match(TokenType::COMMA)) break;
          next();
        }

        expect(TokenType::GREATER);

        if (stmt->genericType.empty())
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            current_token->location,
            "expected generic type",
            "generic type cannot be empty"
          });
      }

      expect(TokenType::LEFTPAREN);
      while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
        bool isVariadic = false;
        if (match(TokenType::VARIADIC)) {
          next();
          isVariadic = true;
        }
        auto location = current_token->location;
        std::string name = expect(TokenType::IDENT)->value;
        expect(TokenType::COLON);
        stmt->params.push_back(Params(
          name,
          parse_type(),
          isVariadic,
          location
        ));

        if (isVariadic) break;
        if (!match(TokenType::COMMA)) break;
        next();
      }
      expect(TokenType::RIGHTPAREN);

      if (match(TokenType::ARROW)) {
        next();
        stmt->returnType = parse_type();
      } else {
        stmt->returnType = new Type();
        stmt->returnType->kind = TypeKind::VOID;
      }

      if (match(TokenType::LEFTBRACE)) {
        stmt->body = parse_block();
        stmt->isDefine = true;
      } else {
        stmt->isDeclare = true;
        skip_semicolon();
      }

      return stmt;
    } else if (match(TokenType::LET)) {
      next();
      stmt->kind = StmtKind::VARDECL;
      stmt->locations = current_token->location;
      auto* identifier = expect(TokenType::IDENT);
      if (!identifier) return stmt;
      stmt->name = identifier->value;

      if (match(TokenType::COLON)) {
        next();
        stmt->dataType = parse_type();
      } else {
        stmt->dataType = new Type();
        stmt->dataType->kind = TypeKind::AUTO;
      }

      if (match(TokenType::EQUAL)) {
        expect(TokenType::EQUAL);
        stmt->value = parse_expr(0);
        stmt->isDefine = true;
      } else {
        stmt->isDeclare = true;
      }

      skip_semicolon();
      return stmt;
    } else if (match(TokenType::IDENT)) {
      stmt->locations = current_token->location;
      stmt->name = expect(TokenType::IDENT)->value;
      skip_semicolon();
      return stmt;
    } else if (match(TokenType::RETURN)) {
      stmt->locations = current_token->location;
      stmt->kind = StmtKind::RETURN;
      next();
      auto* value = parse_expr(0);

      if (value) {
        stmt->value = value;
      } else {
        stmt->value = nullptr;
      }

      skip_semicolon();
      return stmt;
    } else {
      diag->report({
        ErrorType::SYNTAX,
        Severity::ERROR,
        current_token->location,
        "Unexpected syntax '" + current_token->value + "'"
      });
      next();
    }

    return stmt;
  }

  Stmt* Parser::parse_block() {
    Stmt* block = new Stmt();
    block->kind = StmtKind::BLOCK;
    block->locations = current_token->location;
    expect(TokenType::LEFTBRACE);

    while (!match(TokenType::RIGHTBRACE) && !match(TokenType::ENDOFFILE)) {
      block->children.push_back(parse_stmt());
    }

    if (match(TokenType::ENDOFFILE)) {
      diag->report({
        ErrorType::SYNTAX,
        Severity::ERROR,
        current_token->location,
        "unexpected end of file, expected '}' to close block"
      });
      return block;
    }

    expect(TokenType::RIGHTBRACE);
    return block;
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

  Expr* Parser::parse_expr(int prec) {
    Expr* left = parse_value();

    for (;;) {
      int precedence = parse_precedence(current_token->type);

      if (precedence < prec || precedence == 0) break;

      Token* op = current_token;

      next();

      Expr* bin = new Expr();
      bin->kind = ExprKind::BINARY;
      bin->left = left;
      bin->right = parse_expr(precedence + 1);
      bin->binOp = stringToBinaryOp(op->value);
      left = bin;
    }

    return left;
  }

  Expr* Parser::parse_value() {
    Expr* expr = new Expr();
    expr->locations = current_token->location;

    if (match(TokenType::LEFTPAREN)) {
      next();
      expr->exprValue = parse_expr(0);
      expect(TokenType::RIGHTPAREN);
      return expr;
    }

    if (match(TokenType::NONE)) {
      expr->kind = ExprKind::NONE;
      expr->value = expect(TokenType::NONE)->value;
      return expr;
    }

    if (match(TokenType::MINUS) || match(TokenType::PLUS) || match(TokenType::STAR) || match(TokenType::AMPERSAND)) {
      expr->kind = ExprKind::UNARY;
      expr->value = current_token->value;
      expr->unOp = stringToUnaryOp(current_token->value);
      next();
      expr->exprValue = parse_expr(0);
      return expr;
    }

    if (match(TokenType::NUMBER)) {
      expr->kind = ExprKind::PRIMITIVE;
      expr->value = expect(TokenType::NUMBER)->value;
      expr->isSignedInteger = true;
      expr->primitive = Primitive::I64;
      return expr;
    } else if (match(TokenType::STRLIT)) {
      expr->kind = ExprKind::PRIMITIVE;
      expr->value = expect(TokenType::STRLIT)->value;
      expr->primitive = Primitive::STR;
      return expr;
    } else if (match(TokenType::CHAR)) {
      expr->kind = ExprKind::PRIMITIVE;
      expr->value = expect(TokenType::CHARLIT)->value;
      expr->primitive = Primitive::CHAR;
      return expr;
    } else if (match(TokenType::IDENT)) {
      expr->kind = ExprKind::IDENT;
      expr->value = expect(TokenType::IDENT)->value;
      expr = parse_identifiers(expr);
      return expr;
    } else if (match(TokenType::LEFTBRACE)) {
      expr->kind = ExprKind::BLOCK;
      expr->block = parse_block();
      return expr;
    } else if (match(TokenType::LEFTBRACKET)) {
      return expr;
    } else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
      expr->kind = ExprKind::PRIMITIVE;
      expr->value = expect(current_token->type)->value;
      expr->primitive = Primitive::BOOL;
      return expr;
    }

    return nullptr;
  }

  Stmt* Parser::parse_assignment(Stmt* stmt) {
    Expr* expr = new Expr();
    expr->locations = current_token->location;
    expr->name = expect(TokenType::IDENT)->value;
    expr->kind = ExprKind::IDENT;

    bool is_call = false;

    while (true) {
      if (match(TokenType::LEFTBRACKET)) {
        next();
        auto* index = new Expr();
        index->kind = ExprKind::INDEX;
        index->locations = current_token->location;
        index->object = expr;
        index->value = parse_expr(0)->value;
        expect(TokenType::RIGHTBRACKET);

        expr = index;

        if (match(TokenType::EQUAL)) {
          next();
          stmt->kind = StmtKind::ASSIGN;
          stmt->object = expr;
          stmt->value = parse_expr(0);
          stmt->locations = previous_token->location;
          stmt->name = expr->name;
          skip_semicolon();
          return stmt;
        } else if (match(TokenType::IS_EQUAL) || match(TokenType::NOT_EQUAL) ||
                  match(TokenType::PLUS_EQUAL) || match(TokenType::MINUS_EQUAL) ||
                  match(TokenType::STAR_EQUAL) || match(TokenType::DIV_EQUAL) ||
                  match(TokenType::PERCENT_EQUAL) || match(TokenType::POWER_EQUAL)) {
          next();
          stmt->kind = StmtKind::ASSIGN;
          stmt->object = expr;
          stmt->value = new Expr();
          stmt->value->kind = ExprKind::BINARY;
          stmt->value->left = expr;
          stmt->value->right = parse_expr(0);
          stmt->value->binOp = stringToBinaryOp(previous_token->value);
          skip_semicolon();
          return stmt;
        } else if (match(TokenType::DOT)) {
          continue;
        } else {
          diag->report({
            ErrorType::SYNTAX,
            Severity::ERROR,
            current_token->location,
            "expected assignment operator after index expression",
            "",
            ""
          });
          break;
        }
      }

      if (match(TokenType::EQUAL)) {
        next();
        stmt->kind = StmtKind::ASSIGN;
        stmt->object = expr;
        stmt->value = parse_expr(0);
        stmt->locations = previous_token->location;
        stmt->name = expr->name;
        skip_semicolon();
        return stmt;
      } else if (match(TokenType::IS_EQUAL) || match(TokenType::NOT_EQUAL) ||
                 match(TokenType::PLUS_EQUAL) || match(TokenType::MINUS_EQUAL) ||
                 match(TokenType::STAR_EQUAL) || match(TokenType::DIV_EQUAL) ||
                 match(TokenType::PERCENT_EQUAL) || match(TokenType::POWER_EQUAL)) {
        next();
        stmt->kind = StmtKind::ASSIGN;
        stmt->object = expr;
        stmt->value = new Expr();
        stmt->value->kind = ExprKind::BINARY;
        stmt->value->left = expr;
        stmt->value->right = parse_expr(0);
        stmt->value->binOp = stringToBinaryOp(previous_token->value);
        skip_semicolon();
        return stmt;
      }

      if (match(TokenType::LESS)) {
        next();
        auto* generic = new Expr();
        generic->locations = previous_token->location;
        generic->kind = ExprKind::CALL;
        generic->callee = expr;

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->genericType.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->args.push_back(parse_expr(0));
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = generic;
        continue;
      }

      if (match(TokenType::LEFTPAREN)) {
        next();
        auto* call = new Expr();
        call->locations = previous_token->location;
        call->kind = ExprKind::CALL;
        call->callee = expr;
        call->kind = ExprKind::CALL;
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->args.push_back(parse_expr(0));
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = call;
      }

      if (match(TokenType::DOT)) {
        next();
        auto* lookup = new Expr();
        lookup->kind = ExprKind::LOOKUP;
        lookup->locations = current_token->location;
        lookup->value = expect(TokenType::IDENT)->value;
        lookup->object = expr;

        expr = lookup;
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto* lookup_module = new Expr();
        lookup_module->kind = ExprKind::LOOKUP_MODULE;
        lookup_module->locations = current_token->location;
        lookup_module->value = expect(TokenType::IDENT)->value;
        lookup_module->object = expr;

        expr = lookup_module;

        continue;
      }

      break;
    }

    skip_semicolon();
    stmt->value = expr;
    return stmt;
  }

  Expr* Parser::parse_identifiers(Expr* e) {
    Expr* expr = e;

    bool is_call = false;

    while (true) {
      if (match(TokenType::LESS)) {
        next();
        auto* generic = new Expr();
        generic->locations = previous_token->location;
        generic->kind = ExprKind::CALL;
        generic->callee = expr;

        while (!match(TokenType::GREATER) && !match(TokenType::ENDOFFILE)) {
          generic->genericType.push_back(parse_type());
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }
        expect(TokenType::GREATER);

        expect(TokenType::LEFTPAREN);
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          generic->args.push_back(parse_expr(0));
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = generic;
        continue;
      }

      if (match(TokenType::LEFTPAREN)) {
        next();
        auto* call = new Expr();
        call->locations = previous_token->location;
        call->kind = ExprKind::CALL;
        call->callee = expr;
        call->kind = ExprKind::CALL;
        while (!match(TokenType::RIGHTPAREN) && !match(TokenType::ENDOFFILE)) {
          call->args.push_back(parse_expr(0));
          if (match(TokenType::COMMA)) {
            next();
          } else break;
        }

        expect(TokenType::RIGHTPAREN);
        expr = call;
      }

      if (match(TokenType::DOT)) {
        next();
        auto* lookup = new Expr();
        lookup->kind = ExprKind::LOOKUP;
        lookup->locations = current_token->location;
        lookup->value = expect(TokenType::IDENT)->value;
        lookup->object = expr;

        expr = lookup;
        continue;
      }
      else if (match(TokenType::COLON_COLON) && !is_call) {
        next();
        auto* lookup_module = new Expr();
        lookup_module->kind = ExprKind::LOOKUP_MODULE;
        lookup_module->locations = current_token->location;
        lookup_module->value = expect(TokenType::IDENT)->value;
        lookup_module->object = expr;

        expr = lookup_module;

        continue;
      }

      break;
    }

    return expr;
  }

  Primitive tokenTypeToPrimitive(TokenType t) {
    switch (t) {
      case TokenType::U32:
      case TokenType::I32:
        return Primitive::I32;
      case TokenType::U64:
      case TokenType::I64:
        return Primitive::I64;
      case TokenType::U128:
      case TokenType::I128:
        return Primitive::I128;
      case TokenType::F32:
        return Primitive::F32;
      case TokenType::F64:
        return Primitive::F64;
      case TokenType::CHAR:
        return Primitive::CHAR;
      case TokenType::STR:
        return Primitive::STR;
      case TokenType::BOOL:
        return Primitive::BOOL;
      default:
        return Primitive::UNKNOWN;
    }
  }

  Type* Parser::parse_type() {
    Type* type = new Type();
    type->locations = current_token->location;
    type->isNullable = false;

    if (match(TokenType::I32) || match(TokenType::I64)
      || match(TokenType::I128) || match(TokenType::U32)
      || match(TokenType::U64) || match(TokenType::U128)
      || match(TokenType::F32) || match(TokenType::F64)
      || match(TokenType::CHAR) || match(TokenType::STR)
      || match(TokenType::BOOL)) {
      type->kind = TypeKind::PRIMITIVE;
      type->primitive = tokenTypeToPrimitive(current_token->type);
      type->isSignedInteger = type->primitive == Primitive::I32 || type->primitive == Primitive::I64 || type->primitive == Primitive::I128;
      next();

      if (match(TokenType::QUESTION)) {
        type->isNullable = true;
        next();
      }

      return type;
    } else if (match(TokenType::VOID)) {
      type->kind = TypeKind::VOID;
      next();

      if (match(TokenType::QUESTION)) {
        type->isNullable = true;
        next();
      }

      return type;
    } else if (match(TokenType::ANY)) {
      type->kind = TypeKind::ANY;
      next();

      if (match(TokenType::QUESTION)) {
        type->isNullable = true;
        next();
      }

      return type;
    } else if (match(TokenType::FUNCT)) {
      type->kind = TypeKind::FUNC;

      if (match(TokenType::QUESTION)) {
        type->isNullable = true;
        next();
      }

      return type;
    } else if (match(TokenType::IDENT)) {
      type->kind = TypeKind::IDENT;
      type->name = current_token->value;
      next();

      while (true) {

        if (match(TokenType::LESS)) {
          next();
          while (!match(TokenType::GREATER)) {
            type->genericType.push_back(parse_type());
            if (match(TokenType::COMMA)) {
              next();
            } else break;
          }

          expect(TokenType::GREATER);

          if (type->genericType.empty()) {
            diag->report({
              ErrorType::SYNTAX,
              Severity::ERROR,
              current_token->location,
              "expected type after '<'",
              "",
              ""
            });
          }
          break;
        }

        if (match(TokenType::COLON_COLON)) {
          next();

          if (match(TokenType::IDENT)) {
            auto* lookup_type = new Type();
            lookup_type->kind = TypeKind::LOOKUP;
            lookup_type->name = current_token->value;
            lookup_type->locations = current_token->location;
            lookup_type->object = type;
            next();
            type = lookup_type;
          } else {
            diag->report({
              ErrorType::SYNTAX,
              Severity::ERROR,
              current_token->location,
              "expected identifier after '::'",
              "",
              ""
            });
          }
        }


        if (match(TokenType::QUESTION)) {
          type->isNullable = true;
          next();
        }

        return type;
      }

      return nullptr;
    }

    diag->report({
      ErrorType::SYNTAX,
      Severity::ERROR,
      current_token->location,
      "expected type, but got '" + tokenTypeToValue(current_token->type) + "'",
      "",
      ""
    });

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
