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

  unique_ptr<Stmt> Parser::parse() {
    vector<unique_ptr<Stmt>> nodes;

    // start parsing statements at the global scope level
    while (tok.kind != TokenKind::END_OF_FILE)
      nodes.push_back(parse_stmt(false));

    return make_unique<BlockStmt>(std::move(nodes));
  }

  // responsible for parsing types in functions,
  // variables, etc.
  //
  // arguments: []
  // return: unique_ptr<Type>
  //
  unique_ptr<Type> Parser::parse_type() {
    unique_ptr<Type> types = make_unique<AutoType>();

    if (accept(TokenKind::AMPER)) {
      types = parse_type();
      return make_unique<ReferenceType>(std::move(types));
    } else if (accept(TokenKind::STAR)) {
      types = parse_type();
      return make_unique<PointerType>(std::move(types));
    }

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

  // this function is used to obtain all implementations
  // of statements such as functions, literals, if-else, for loops, etc.
  //
  // arguments: []
  // returns: unique_ptr<Stmt>
  //
  unique_ptr<Stmt> Parser::parse_stmt(bool at_local_scope) {
    bool is_public = false;

    if (!at_local_scope && accept(TokenKind::PUBLIC)) {
      is_public = true;
    }
    else if (at_local_scope && tok.kind == TokenKind::PUBLIC) {
      report_error("unexpected `public` statement at local scope.");
      accept(TokenKind::PUBLIC); // skip 'public' keyword
      synchronize();
      return make_unique<BlockStmt>(vector<unique_ptr<Stmt>>());
    }

    if (tok.kind == TokenKind::LET ||
      tok.kind == TokenKind::CONST ||
      tok.kind == TokenKind::STATIC) {

      Modifiers mod = Modifiers::LETERAL;
      switch (tok.kind) {
        case TokenKind::CONST: mod = Modifiers::CONST; break;
        case TokenKind::STATIC: mod = Modifiers::STATIC; break;
        default: break;
      }

      accept(tok.kind);

      string name = tok.value;
      if (!accept(TokenKind::IDENTIFIER)) {
        stringstream error;
        error << "expected variable name, but found `" << TokenKindToValue(tok.kind) <<"";
        if (tok.kind == TokenKind::NUMBER) error << " number.";
        else if (tok.kind == TokenKind::IDENTIFIER) error << " identifier.";
        else error << " keyword / punctuation.";
        report_error(error.str());
        synchronize();
        return make_unique<BlockStmt>(vector<unique_ptr<Stmt>>());
      }

      // default type is automatically typed
      unique_ptr<Type> types = make_unique<AutoType>();

      // default value is none
      unique_ptr<Expr> value = make_unique<NoneExpr>();

      if (mod == Modifiers::LETERAL) {
        if (accept(TokenKind::COLON))
        {
          types = parse_type();
          if (accept(TokenKind::EQUAL)) {
            value = parse_expression(0);
          }
        }
      } else {
        consume(TokenKind::COLON);
        types = parse_type();
        consume(TokenKind::EQUAL);
        value = parse_expression(0);
      }

      return make_unique<LetDecl>(
        std::move(name),
        std::move(types),
        std::move(value),
        mod,
        is_public
      );
    }

    // parse function declaration
    if (accept(TokenKind::FUNCTION)) {
      // get identifier
      string name = tok.value;
      consume(TokenKind::IDENTIFIER);

      // Take a generic param if the function supports it
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

      consume(TokenKind::LPARENT); // consume '('

      // get the function parameters
      vector<unique_ptr<Stmt>> parameter;
      while (tok.kind != TokenKind::RPARENT && tok.kind != TokenKind::END_OF_FILE) {

        string name = tok.value;
        consume(TokenKind::IDENTIFIER);
        consume(TokenKind::COLON);
        unique_ptr<Type> types = parse_type();

        parameter.push_back(make_unique<Parameter>(name, std::move(types)));

        if (!accept(TokenKind::COMMA)) break;
      }

      consume(TokenKind::RPARENT); // consume ')'

      // default return type is void
      unique_ptr<Type> return_types = make_unique<VoidType>();

      // if there in an arrow, it means the return type of this function is explicitly defined
      if (accept(TokenKind::ARROW))
        return_types = parse_type();

      // parsing the function body
      unique_ptr<Stmt> body = parse_block();

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

    if (accept(TokenKind::IF)) {
      unique_ptr<Expr> conditions = parse_expression(0);

      unique_ptr<Stmt> body_if = parse_block();

      unique_ptr<Stmt> body_else = make_unique<BlockStmt>(vector<unique_ptr<Stmt>>());

      if (accept(TokenKind::ELSE)) {
        if (accept(TokenKind::IF)) {
          body_else = parse_block();
        } else {
          body_else = parse_block();
        }
      }

      return make_unique<BranchStmt>(std::move(conditions), std::move(body_if), std::move(body_else));
    }

    if (accept(TokenKind::FOR)) {
      unique_ptr<Expr> initializer = make_unique<IdentifierExpr>(tok.value);
      consume(TokenKind::IDENTIFIER);

      consume(TokenKind::IN);

      unique_ptr<Expr> iterator;
      unique_ptr<Expr> starts = parse_expression(0);
      if (accept(TokenKind::DOT_DOT)) {
        unique_ptr<Expr> ends = parse_expression(0);
        iterator = make_unique<Range>(std::move(starts), std::move(ends));
      } else iterator = std::move(starts);

      unique_ptr<Stmt> body = parse_block();

      return make_unique<ForLoopStmt>(std::move(initializer), std::move(iterator), std::move(body));
    }

    // parse assignment or function calling
    if (tok.kind == TokenKind::IDENTIFIER) {
      unique_ptr<Expr> expr = make_unique<IdentifierExpr>(tok.value);
      consume(TokenKind::IDENTIFIER);

      // postfix parsing (member access, calls, generic calls)
      expr = parse_postfix_from_expr(std::move(expr));

      // assignment handling: only treat real assignment operators here
      bool is_call = dynamic_cast<CallExpr*>(expr.get()) != nullptr;

      bool is_assignment_op = (
        tok.kind == TokenKind::EQUAL ||
        tok.kind == TokenKind::PLUS_EQUAL ||
        tok.kind == TokenKind::MINUS_EQUAL ||
        tok.kind == TokenKind::STAR_EQUAL ||
        tok.kind == TokenKind::SLASH_EQUAL ||
        tok.kind == TokenKind::POWER_EQUAL ||
        tok.kind == TokenKind::PERCENT_EQUAL
      );

      if (is_assignment_op) {
        if (is_call) {
          // report error and skip the rest of the current source line
          report_error("cannot assign to the result of a function call");
          size_t currentLine = tok.line;
          // advance until we move to the next line or EOF
          while (tok.kind != TokenKind::END_OF_FILE && tok.line == currentLine) {
            tok = lexer->next();
          }
          return make_unique<ExprStmt>(std::move(expr));
        }

        string op = tok.value;
        consume(tok.kind);
        unique_ptr<Expr> values = parse_expression(0);
        return make_unique<AssignDecl>(op, std::move(expr), std::move(values));
      }

      // otherwise it's an expression statement
      return make_unique<ExprStmt>(std::move(expr));
    }

    report_error("unexpected token: `" + TokenKindToValue(tok.kind) + "`");
    synchronize();

    accept(tok.kind);

    return make_unique<BlockStmt>(vector<unique_ptr<Stmt>>());
  }

  unique_ptr<Stmt> Parser::parse_block() {
    vector<unique_ptr<Stmt>> children;

    consume(TokenKind::LBRACE); // consume '{'

    while (
      tok.kind != TokenKind::RBRACE && 
      tok.kind != TokenKind::END_OF_FILE
    ) children.push_back(parse_stmt(true));

    consume(TokenKind::RBRACE); // consume '}'

    return make_unique<BlockStmt>(std::move(children));
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

  unique_ptr<Expr> Parser::parse_expression(int min_prec) {
    unique_ptr<Expr> lhs = parse_value_literal();

    if (!lhs) {
      report_error("expected expression");
      synchronize();
      return make_unique<NoneExpr>();
    }

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

      unique_ptr<Expr> rhs = parse_expression(next_min_prec);

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
      unique_ptr<Expr> expr = parse_expression(0);
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
        std::move(current.value)
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
        std::move(current.value)
      );
    }

    if (tok.kind == TokenKind::IDENTIFIER) {
      unique_ptr<Expr> expr = make_unique<IdentifierExpr>(tok.value);
      consume(TokenKind::IDENTIFIER);
      return parse_postfix_from_expr(std::move(expr));
    }

    if (accept(TokenKind::NONE))
      return make_unique<NoneExpr>();

    // If no literal/primary found, return a NoneExpr so callers don't get nullptr.
    return make_unique<NoneExpr>();
  }

  bool Parser::accept(TokenKind kind) {
    if (tok.kind != kind)
      return false;

    tok = lexer->next();
    return true;
  }

  unique_ptr<Expr> Parser::parse_postfix_from_expr(unique_ptr<Expr> expr) {
    while (tok.kind != TokenKind::END_OF_FILE) {
      // member access: a.b
      if (tok.kind == TokenKind::DOT) {
        consume(TokenKind::DOT);

        if (tok.kind != TokenKind::IDENTIFIER) {
          report_error("expected identifier after '.'");
          synchronize();
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
        while (tok.kind != TokenKind::GREATER && tok.kind != TokenKind::END_OF_FILE) {
          typeArgs.push_back(parse_type());
          if (!accept(TokenKind::COMMA)) break;
        }

        consume(TokenKind::GREATER);

        consume(TokenKind::LPARENT);

        vector<unique_ptr<Expr>> args;
        while (tok.kind != TokenKind::RPARENT && tok.kind != TokenKind::END_OF_FILE) {
          args.push_back(parse_expression(0));
          if (!accept(TokenKind::COMMA)) break;
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
        while (tok.kind != TokenKind::RPARENT && tok.kind != TokenKind::END_OF_FILE) {
          args.push_back(parse_expression(0));
          if (!accept(TokenKind::COMMA)) break;
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

  void Parser::report_error(const std::string& message) {
    stringstream error;
    error << "\033[31merrors:\033[0m " << message << "\n";
    error << "  at " << tok.files << ":" << tok.line << ":" << tok.column << "\n";
    error << "  --> " << tok.source << "\n";
    ErrorHandler::create(error.str());
  }

  void Parser::synchronize() {
    // advance until we find a good recovery point (next source line or block end)
    size_t startLine = tok.line;

    while (tok.kind != TokenKind::END_OF_FILE && tok.kind != TokenKind::RBRACE) {
      if (tok.line > startLine) {
        // moved to next source line: good recovery point
        break;
      }

      if (tok.kind == TokenKind::SEMICOLON) {
        tok = lexer->next();
        break;
      }

      tok = lexer->next();
    }
  }

  bool Parser::consume(TokenKind kind) {
    if (tok.kind != kind) {
      stringstream error;
      error << "expected `" << TokenKindToValue(kind) <<"`, but got `" << TokenKindToValue(tok.kind) << "`";
      report_error(error.str());
      // try to resynchronize so parser can continue
      synchronize();
      return false;
    }

    tok = lexer->next();
    return true;
  }
};
