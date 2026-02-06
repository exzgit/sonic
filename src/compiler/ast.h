#pragma once

// c++ library
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <charconv>
#include <system_error>
#include <cmath>

// local headers
#include "source.h"

namespace sonic::frontend::ast {

  // declaration
  struct Type;
  struct Expression;
  struct Statement;

  enum class StmtKind {
    NAMESPACE,

    // macro decorator
    MACRO,
    MACRO_ATTR,

    // declaration
    VARIABLE,
    FUNCTION,

    // assignment
    ASSIGNMENT,

    // parameters and generics
    PARAMETER,
    GENERICS,

    // program logic
    IF_ELSE,
    FOR_LOOP,
    WHILE_LOOP,
    TRY_CATCH,

    // instruction
    BREAK,
    DEFAULT,
    CONTINUE,
    RETURN,

    // module importer
    IMPORT,
    IMPORT_FIELD,
    IMPORT_ITEM,

    // data structure
    STRUCT,
    STRUCT_FIELD,

    // enums
    ENUM,
    ENUM_VARIANT,

    // expression
    EXPR,
  };

  enum class Mutability {
    STATIC,
    CONSTANT,
    VARIABLE,
  };


  enum class TypeKind {
    LITERAL,
    VOID,
    PTR,
    REF,
    OBJECT,
    SCOPE,
    FUNCTION,
  };

  enum class LiteralKind {
    STRING,
    CHAR,
    I32,
    I64,
    I128,
    F32,
    F64,
    BOOL,

    // if unresolved literal number
    UNK_INT,
    UNK_FLOAT,
  };

  // MARK: TYPE
  struct Type {
    TypeKind kind_;
    LiteralKind literal_;

    SourceLocation loc_;

    // named of type
    std::string name_;

    // type nested
    std::unique_ptr<Type> nested_;

    // generics
    std::vector<std::unique_ptr<Type>> generics_;

    // decoration
    bool nullable_  = false;

    // semantic info
    void* symbols_ = nullptr;

    Type() = default;
    ~Type() = default;

    std::unique_ptr<Type> clone() {
      auto type = std::make_unique<Type>();
      type->kind_ = kind_;
      type->literal_ = literal_;
      type->loc_ = loc_;
      type->name_ = name_;

      if (nested_) type->nested_ = nested_->clone();

      for (auto& ch : generics_) type->generics_.push_back(ch->clone());

      type->nullable_ = nullable_;

      return type;
    }

    int bitWidth() {
      switch (literal_) {
        case LiteralKind::F32:
        case LiteralKind::I32: return 32;
        case LiteralKind::F64:
        case LiteralKind::I64: return 64;
        case LiteralKind::I128: return 128;
        default: return 0;
      }
    }

    bool isIntegerType() {
      switch (literal_) {
        case LiteralKind::I32:
        case LiteralKind::I64:
        case LiteralKind::I128:
        case LiteralKind::UNK_INT:
          return true;
        default:
          return false;
      }
    }

    bool isFloatType() {
      switch (literal_) {
        case LiteralKind::F32:
        case LiteralKind::F64:
          return true;
        default:
          return false;
      }
    }
  };

  enum class ExprKind {
    LITERAL,
    VARIABLE,
    SCOPE,
    MEMBER,
    REF,
    DEREF,
    INDEX,
    BINARY,
    UNARY,
    CALL,
    RANGE,
    NONE,
  };

  // MARK: EXPRESSION
  struct Expression {
    ExprKind kind_;
    LiteralKind literal_;
    SourceLocation loc_;

    std::string name_;
    std::string value_;
    std::string raw_;

    std::vector<std::unique_ptr<Type>> generics_;
    std::vector<std::unique_ptr<Expression>> args_;

    std::unique_ptr<Expression> nested_;
    std::unique_ptr<Expression> index_;
    std::unique_ptr<Expression> callee_;

    // for binary expression / range iterator
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;


    // semantic info
    Type* type_;
    void* symbols_ = nullptr;

    Expression() = default;
    ~Expression() = default;

    std::unique_ptr<Expression> clone() {
      auto expr = std::make_unique<Expression>();
      expr->kind_ = kind_;
      expr->loc_ = loc_;
      expr->name_ = name_;

      for (auto& ch : generics_) expr->generics_.push_back(ch->clone());
      for (auto& ch : args_) expr->args_.push_back(ch->clone());

      if (nested_) expr->nested_ = nested_->clone();
      if (index_) expr->index_ = index_->clone();
      if (callee_) expr->callee_ = callee_->clone();

      if (lhs_) expr->lhs_ = lhs_->clone();
      if (rhs_) expr->rhs_ = rhs_->clone();

      if (type_) expr->type_ = type_;

      return expr;
    }

    int bitWidth() {
      if (value_.find('.') != std::string::npos) {
        float f;
        auto [ptr, ec] = std::from_chars(value_.data(), value_.data() + value_.size(), f);
        if (ec == std::errc()) return 32;
        double d;
        auto [ptr2, ec2] = std::from_chars(value_.data(), value_.data() + value_.size(), d);
        if (ec2 == std::errc()) return 64;
        return 0;
      } else {
        int i;
        auto [ptr, ec] = std::from_chars(value_.data(), value_.data() + value_.size(), i);
        if (ec == std::errc()) return 32;
        long l;
        auto [ptr2, ec2] = std::from_chars(value_.data(), value_.data() + value_.size(), l);
        if (ec2 == std::errc()) return 64;
        long long ll;
        auto [ptr3, ec3] = std::from_chars(value_.data(), value_.data() + value_.size(), ll);
        if (ec3 == std::errc()) return 128;
        return 0;
      }
    }

    bool isIntegerVal() {
      int i;
      auto [ptr, ec] = std::from_chars(value_.data(), value_.data() + value_.size(), i);
      if (ec == std::errc()) return true;
      long l;
      auto [ptr2, ec2] = std::from_chars(value_.data(), value_.data() + value_.size(), l);
      if (ec2 == std::errc()) return true;
      long long ll;
      auto [ptr3, ec3] = std::from_chars(value_.data(), value_.data() + value_.size(), ll);
      if (ec3 == std::errc()) return true;
      return false;
    }

    bool isFloatVal() {
      float f;
      auto [ptr, ec] = std::from_chars(value_.data(), value_.data() + value_.size(), f);
      if (ec == std::errc()) return true;
      double d;
      auto [ptr2, ec2] = std::from_chars(value_.data(), value_.data() + value_.size(), d);
      if (ec2 == std::errc()) return true;
      return false;
    }
  };

  // MARK: STATEMENT
  struct Statement {
    StmtKind kind_;
    SourceLocation loc_;

    // variable | function | struct | enum
    std::string name_;

    // assignment
    std::unique_ptr<Expression> assign_;

    // value | condition | iterator | catch error
    std::unique_ptr<Expression> value_;

    // import module
    std::vector<std::unique_ptr<Statement>> import_qualified_;
    std::vector<std::unique_ptr<Statement>> import_items_;
    std::string import_alias_;

    // function | variable
    std::unique_ptr<Type> type_;

    // struct | enum | function
    std::vector<std::unique_ptr<Statement>> generics_;

    // function | macro attr
    std::vector<std::unique_ptr<Statement>> params_;

    // function | while | for
    std::vector<std::unique_ptr<Statement>> body_;

    // branch statement
    std::vector<std::unique_ptr<Statement>> then_;
    std::vector<std::unique_ptr<Statement>> else_;

    // try | catch |finally
    std::vector<std::unique_ptr<Statement>> try_;
    std::vector<std::unique_ptr<Statement>> catch_;
    std::vector<std::unique_ptr<Statement>> finally_;


    // decorations
    bool public_ = false;
    bool extern_  = false;
    bool async_ = false;
    bool import_all_ = false;
    Mutability mutability = Mutability::VARIABLE;
    bool declare_  = false;
    bool variadic_ = false;

    // semantic info
    void* symbols_ = nullptr;

    Statement() = default;
    ~Statement() = default;

    std::unique_ptr<Statement> clone() {
      auto stmt = std::make_unique<Statement>();
      stmt->kind_ = kind_;
      stmt->loc_ = loc_;
      stmt->name_ = name_;
      stmt->import_alias_ = import_alias_;
      stmt->public_ = public_;
      stmt->extern_ = extern_;
      stmt->async_ = async_;
      stmt->import_all_ = import_all_;
      stmt->mutability = mutability;
      stmt->declare_ = declare_;
      stmt->variadic_ = variadic_;

      if (assign_) stmt->assign_ = assign_->clone();
      if (value_) stmt->value_ = value_->clone();
      if (type_) stmt->type_ = type_->clone();

      for (auto& ch : import_qualified_) stmt->import_qualified_.push_back(ch->clone());
      for (auto& ch : import_items_) stmt->import_items_.push_back(ch->clone());
      for (auto& ch : generics_) stmt->generics_.push_back(ch->clone());
      for (auto& ch : params_) stmt->params_.push_back(ch->clone());
      for (auto& ch : body_) stmt->body_.push_back(ch->clone());

      for (auto& ch : then_) stmt->then_.push_back(ch->clone());
      for (auto& ch : else_) stmt->else_.push_back(ch->clone());

      return stmt;
    }
  };

  struct Program {
    std::string name_;

    std::vector<std::unique_ptr<Statement>> statements_;

    Program() = default;
    ~Program() = default;

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    Program(Program&&) = default;
    Program& operator=(Program&&) = default;

    std::unique_ptr<Program> clone() {
      auto program = std::make_unique<Program>();
      program->name_ = name_;

      for (auto& ch : statements_) program->statements_.push_back(ch->clone());

      return program;
    }
  };

};
