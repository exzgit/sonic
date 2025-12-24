#include "parser.h"
#include "compiler/ast/stmt.h"
#include "compiler/ast/type.h"
#include "core/token.h"
#include <memory>

namespace frontend {
  Parser::Parser(string filepath, Lexer* lexer)
  : files(filepath), lexer(lexer) {
    tok = lexer->next();
  }

  vector<unique_ptr<Stmt>> Parser::parse() {
    vector<unique_ptr<Stmt>> nodes;

    while (tok.kind != TokenKind::END_OF_FILE) {
      unique_ptr<Stmt> stmt = parse_stmt();
      if (stmt) nodes.push_back(stmt);
    }

    return nodes;
  }

  unique_ptr<Type> Parser::parse_type() {
    unique_ptr<Type> types = make_unique<AutoType>();

    if (accept(TokenKind::U8))
      types = make_unique<StandardType>(StandardTypeKind::U8_Type);
    else if (accept(TokenKind::U16))
      types = make_unique<StandardType>(StandardTypeKind::U16_Type);
    else if (accept(TokenKind::U32))
      types = make_unique<StandardType>(StandardTypeKind::U32_Type);
    else if (accept(TokenKind::U64))
      types = make_unique<StandardType>(StandardTypeKind::U64_Type);
    else if (accept(TokenKind::U128))
      types = make_unique<StandardType>(StandardTypeKind::U128_Type);
    else if (accept(TokenKind::I8))
      types = make_unique<StandardType>(StandardTypeKind::I8_Type);
    else if (accept(TokenKind::I16))
      types = make_unique<StandardType>(StandardTypeKind::I16_Type);
    else if (accept(TokenKind::I32))
      types = make_unique<StandardType>(StandardTypeKind::I32_Type);
    else if (accept(TokenKind::I64))
      types = make_unique<StandardType>(StandardTypeKind::I64_Type);
    else if (accept(TokenKind::I128))
      types = make_unique<StandardType>(StandardTypeKind::I128_Type);
    else if (accept(TokenKind::F32))
      types = make_unique<StandardType>(StandardTypeKind::F32_Type);
    else if (accept(TokenKind::F64))
      types = make_unique<StandardType>(StandardTypeKind::F64_Type);
    else if (accept(TokenKind::BOOL))
      types = make_unique<StandardType>(StandardTypeKind::Boolean_Type);
    else if (accept(TokenKind::CHAR))
      types = make_unique<StandardType>(StandardTypeKind::Char_Type);
    else if (accept(TokenKind::STRING))
      types = make_unique<StandardType>(StandardTypeKind::String_Type);
    else {
      vector<string> segment;
      while (tok.kind != TokenKind::END_OF_FILE) {
        segment.push_back(tok.value);
        consume(TokenKind::IDENTIFIER);

        if (accept(TokenKind::DOUBLE_COLON)) {
          if (tok.kind != TokenKind::IDENTIFIER) consume(TokenKind::IDENTIFIER);
          continue;
        } else break;
      }

      types = make_unique<QualifiedType>(segment);

      if (accept(TokenKind::LESS)) {
        vector<unique_ptr<Type>> genericList;

        while (tok.kind != TokenKind::END_OF_FILE && tok.kind != TokenKind::GREATER) {
          genericList.push_back(parse_type());
          if (tok.kind == TokenKind::COMMA) consume(tok.kind);
          else break;
        }

        consume(TokenKind::GREATER);
        types = make_unique<GenericType>(std::move(types), std::move(genericList));
      }
    }

    return types;
  }

  unique_ptr<Stmt> Parser::parse_stmt() {
    if (accept(TokenKind::LET)) {
      string name = tok.value;
      consume(TokenKind::IDENTIFIER);

      unique_ptr<Type> types = make_unique<AutoType>();
      if (accept(TokenKind::COLON))
        types = parse_type();

      unique_ptr<Expr> value = make_unique<NoneExpr>();
      if (accept(TokenKind::EQUAL)) value = parse_expression();

      return make_unique<LeteralDecl>(
        std::move(name),
        std::move(types),
        std::move(value)
      );
    }

    if (accept(TokenKind::PUBLIC)) {
      if (accept(TokenKind::FUNCTION)) return parse_function_stmt(true);
    }

    if (accept(TokenKind::FUNCTION)) return parse_function_stmt(false);

    if (tok.kind == TokenKind::IDENTIFIER) return parse_assignment();

    return nullptr;
  };

  unique_ptr<Stmt> Parser::parse_function_stmt(bool is_public) {
    string name = tok.value;
    consume(TokenKind::IDENTIFIER);

    vector<unique_ptr<Type>> typeParams;
    bool hasTypeParams = false;
    if (accept(TokenKind::LESS)) {
      hasTypeParams = true;
      while (tok.kind != TokenKind::GREATER && tok.kind != TokenKind::END_OF_FILE) {
        typeParams.push_back(parse_type());
        if (!accept(TokenKind::COMMA)) break;
      }
      consume(TokenKind::GREATER);
    }

    consume(TokenKind::LPARENT);

    vector<unique_ptr<Stmt>> parameter;
    while (tok.kind != TokenKind::RPARENT && tok.kind != TokenKind::END_OF_FILE) {
      string name = tok.value;
      consume(TokenKind::IDENTIFIER);
      consume(TokenKind::COLON);
      unique_ptr<Type> types = parse_type();

      parameter.push_back(make_unique<Parameter>(name, std::move(types)));

      if (!accept(TokenKind::COMMA)) break;
    }

    consume(TokenKind::RPARENT);

    unique_ptr<Type> return_types = make_unique<VoidType>();

    if (accept(TokenKind::ARROW)) {
      return_types = parse_type();
    }

    vector<unique_ptr<Stmt>> body;
    consume(TokenKind::LBRACE);

    while (tok.kind != TokenKind::RBRACE && tok.kind != TokenKind::END_OF_FILE) {
      unique_ptr<Stmt> stmt = parse_stmt();
      if (stmt) body.push_back(stmt);
    }

    consume(TokenKind::RBRACE);

    return make_unique<FunctionDecl>(
        std::move(name),
        std::move(return_types),
        std::move(parameter),
        std::move(body),
        is_public,
        hasTypeParams,
        std::move(typeParams)
      );
  }

  unique_ptr<Stmt> Parser::parse_assignment() {
      unique_ptr<Expr> expr = make_unique<IdentifierExpr>(tok.value);
      consume(TokenKind::IDENTIFIER);

      while (tok.kind != TokenKind::END_OF_FILE) {

        // member access: a.b
        if (tok.kind == TokenKind::DOT) {
          consume(TokenKind::DOT);

          if (tok.kind != TokenKind::IDENTIFIER) {
            consume(TokenKind::IDENTIFIER);
            break;
          }

          string member = tok.value;
          consume(TokenKind::IDENTIFIER);

          expr = make_unique<LookupExpr>(
            std::move(expr),
            make_unique<IdentifierExpr>(member)
          );

          continue;
        }

        if (tok.kind == TokenKind::EQUAL) {
          consume(TokenKind::EQUAL);
          unique_ptr<Expr> values = parse_expression();
          return make_unique<AssignDecl>("=", std::move(expr), std::move(values));
        } else if (tok.kind == TokenKind::IS_EQUAL ||
            tok.kind == TokenKind::NOT_EQUAL ||
            tok.kind == TokenKind::PLUS_EQUAL ||
            tok.kind == TokenKind::MINUS_EQUAL ||
            tok.kind == TokenKind::STAR_EQUAL ||
            tok.kind == TokenKind::SLASH_EQUAL ||
            tok.kind == TokenKind::POWER_EQUAL ||
            tok.kind == TokenKind::PERCENT_EQUAL) {
          string op = tok.value;
          consume(tok.kind);
          unique_ptr<Expr> values = parse_expression();
          return make_unique<AssignDecl>(op, std::move(expr), std::move(values));
        }

        // call<type>: x<type>(...)
        if (tok.kind == TokenKind::LESS) {
          consume(TokenKind::LESS);
          vector<unique_ptr<Type>> typeArgs;
          while (tok.kind != TokenKind::GREATER &&
                tok.kind != TokenKind::END_OF_FILE) {

            typeArgs.push_back(parse_type());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }
          consume(TokenKind::GREATER);
          expr = make_unique<CallExpr>(
            std::move(expr),
            vector<unique_ptr<Expr>>(),
            true,
            std::move(typeArgs)
          );;

          consume(TokenKind::LPARENT);

          vector<unique_ptr<Expr>> args;
          while (tok.kind != TokenKind::RPARENT &&
                tok.kind != TokenKind::END_OF_FILE) {

            args.push_back(parse_expression());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }

          consume(TokenKind::RPARENT);

          expr = make_unique<CallExpr>(
            std::move(expr),
            std::move(args),
            true,
            std::move(typeArgs)
          );
          continue;
        }

        // call: x(...)
        if (tok.kind == TokenKind::LPARENT) {
          consume(TokenKind::LPARENT);

          vector<unique_ptr<Expr>> args;
          while (tok.kind != TokenKind::RPARENT &&
                tok.kind != TokenKind::END_OF_FILE) {

            args.push_back(parse_expression());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }

          consume(TokenKind::RPARENT);

          expr = make_unique<CallExpr>(
            std::move(expr),
            std::move(args),
            false,
            vector<unique_ptr<Type>>()
          );

          continue;
        }

        break;
      }

      return make_unique<ExprStmt>(std::move(expr));
  }

  int Parser::parse_precedence(TokenKind kind) {
    switch (kind) {
      case TokenKind::OR:
        return 1;

      case TokenKind::AND:
        return 2;

      case TokenKind::IS_EQUAL:
      case TokenKind::NOT_EQUAL:
        return 3;

      case TokenKind::LESS:
      case TokenKind::GREATER:
      case TokenKind::LESS_EQUAL:
      case TokenKind::GREATER_EQUAL:
        return 4;

      case TokenKind::PLUS:
      case TokenKind::MINUS:
        return 5;

      case TokenKind::STAR:
      case TokenKind::SLASH:
      case TokenKind::PERCENT:
        return 6;

      case TokenKind::POWER:
        return 7;

      default:
        return 0; // is not binary operand
    }
  }

  unique_ptr<Expr> Parser::parse_expression() {
    return parse_binary_expr(0);
  }

  unique_ptr<Expr> Parser::parse_binary_expr(int min_prec) {
    unique_ptr<Expr> lhs = parse_value_literal();

    while (true) {
      if (tok.kind == TokenKind::END_OF_FILE)
        break;

      int prec = parse_precedence(tok.kind);

      if (prec < min_prec || prec == 0)
        break;

      string op = tok.value;
      TokenKind op_kind = tok.kind;
      consume(tok.kind);

      // associativity
      int next_min_prec = prec + 1;

      if (op_kind == TokenKind::POWER)
        next_min_prec = prec;

      unique_ptr<Expr> rhs = parse_binary_expr(next_min_prec);

      lhs = make_unique<BinOp>(
        std::move(op),
        std::move(lhs),
        std::move(rhs)
      );
    }

    return lhs;
  }

  unique_ptr<Expr> Parser::parse_value_literal() {
    Token current = tok;

    if (tok.kind == TokenKind::LPARENT) {
      consume(tok.kind);
      unique_ptr<Expr> expr = parse_expression();
      consume(TokenKind::RPARENT);
      return expr;
    }

    if (tok.kind == TokenKind::STAR) {
      consume(TokenKind::STAR);
      unique_ptr<Expr> addr = parse_value_literal();
      return make_unique<GetValuePtr>(std::move(addr));
    }

    if (tok.kind == TokenKind::AMPER) {
      consume(TokenKind::AMPER);
      unique_ptr<Expr> target = parse_value_literal();
      return make_unique<AddressOf>(std::move(target));
    }

    if (tok.kind == TokenKind::STRING_LITERAL) {
      consume(TokenKind::STRING_LITERAL);
      return make_unique<LiteralExpr>(
        LiteralKind::String,
        current.value
      );
    }

    if (tok.kind == TokenKind::NUMBER) {
      consume(TokenKind::NUMBER);
      return make_unique<LiteralExpr>(
        LiteralKind::Digits,
        current.value
      );
    }

    if (tok.kind == TokenKind::SCNOTATION) {
      consume(TokenKind::SCNOTATION);
      return make_unique<LiteralExpr>(
        LiteralKind::ScientificNotation,
        current.value
      );
    }

    if (tok.kind == TokenKind::HEX) {
      consume(TokenKind::HEX);
      return make_unique<LiteralExpr>(
        LiteralKind::Hex,
        current.value
      );
    }

    if (tok.kind == TokenKind::CHAR_LITERAL) {
      consume(TokenKind::CHAR_LITERAL);
      return make_unique<LiteralExpr>(
        LiteralKind::Char,
        current.value
      );
    }

    if (tok.kind == TokenKind::TRUE || tok.kind == TokenKind::FALSE) {
      consume(tok.kind);
      return make_unique<LiteralExpr>(
        LiteralKind::Boolean,
        current.value
      );
    }

    if (tok.kind == TokenKind::IDENTIFIER) {
      unique_ptr<Expr> expr = make_unique<IdentifierExpr>(tok.value);
      consume(TokenKind::IDENTIFIER);

      while (tok.kind != TokenKind::END_OF_FILE) {

        // member access: a.b
        if (tok.kind == TokenKind::DOT) {
          consume(TokenKind::DOT);

          if (tok.kind != TokenKind::IDENTIFIER) {
            consume(TokenKind::IDENTIFIER);
            break;
          }

          string member = tok.value;
          consume(TokenKind::IDENTIFIER);

          expr = make_unique<LookupExpr>(
            std::move(expr),
            make_unique<IdentifierExpr>(member)
          );
          continue;
        }

        // call<type>: x<type>(...)
        if (tok.kind == TokenKind::LESS) {
          consume(TokenKind::LESS);
          vector<unique_ptr<Type>> typeArgs;
          while (tok.kind != TokenKind::GREATER &&
                tok.kind != TokenKind::END_OF_FILE) {

            typeArgs.push_back(parse_type());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }
          consume(TokenKind::GREATER);
          expr = make_unique<CallExpr>(
            std::move(expr),
            vector<unique_ptr<Expr>>(),
            true,
            std::move(typeArgs)
          );;

          consume(TokenKind::LPARENT);

          vector<unique_ptr<Expr>> args;
          while (tok.kind != TokenKind::RPARENT &&
                tok.kind != TokenKind::END_OF_FILE) {

            args.push_back(parse_expression());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }

          consume(TokenKind::RPARENT);

          expr = make_unique<CallExpr>(
            std::move(expr),
            std::move(args),
            true,
            std::move(typeArgs)
          );

          continue;
        }

        // call: x(...)
        if (tok.kind == TokenKind::LPARENT) {
          consume(TokenKind::LPARENT);

          vector<unique_ptr<Expr>> args;
          while (tok.kind != TokenKind::RPARENT &&
                tok.kind != TokenKind::END_OF_FILE) {

            args.push_back(parse_expression());
            if (tok.kind == TokenKind::COMMA) consume(TokenKind::COMMA);
            else break;
          }

          consume(TokenKind::RPARENT);

          expr = make_unique<CallExpr>(
            std::move(expr),
            std::move(args),
            false,
            vector<unique_ptr<Type>>()
          );

          continue;
        }

        break;
      }

      return expr;

    }

    if (tok.kind == TokenKind::NONE) {
      consume(TokenKind::NONE);
      return make_unique<NoneExpr>();
    }

    return make_unique<NoneExpr>();
  }

  bool Parser::accept(TokenKind kind) {
    if (tok.kind != kind) {
      return false;
    }

    tok = lexer->next();
    return true;
  }

  bool Parser::consume(TokenKind kind) {
    if (tok.kind != kind) {
      stringstream error;
      error << "\033[31merrors:\033[0m expected `" << TokenKindToValue(kind) <<"`, but got `" << TokenKindToValue(tok.kind) << "`\n";
      error << "  at " << tok.files << ":" << tok.line << ":" << tok.column << "\n";
      error << "  --> " << tok.source << "\n";
      ErrorHandler::create(error.str());
      return false;
    }

    tok = lexer->next();
    return true;
  }
};
