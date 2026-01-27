#pragma once

#include <string>
#include <vector>
#include "../source.h"
#include "../operand.h"
#include "type.h"

namespace sonic::frontend {
  struct Symbol;
  struct Stmt;

  enum class ExprKind {
    PRIMITIVE,
    NONE,
    ARRAY,
    MAP,
    IDENT,
    LOOKUP,
    BLOCK,
    LOOKUP_MODULE,
    CALL,
    STRUCT,
    UNARY,
    BINARY,
    INDEX,
  };

  inline std::string exprKindToString(ExprKind k) {
    switch (k) {
      case ExprKind::PRIMITIVE: return "primitive";
      case ExprKind::NONE: return "none";
      case ExprKind::ARRAY: return "array";
      case ExprKind::MAP: return "map";
      case ExprKind::IDENT: return "ident";
      case ExprKind::LOOKUP: return "lookup";
      case ExprKind::BLOCK: return "block";
      case ExprKind::LOOKUP_MODULE: return "lookup_module";
      case ExprKind::CALL: return "call";
      case ExprKind::STRUCT: return "struct";
      case ExprKind::UNARY: return "unary";
      case ExprKind::BINARY: return "binary";
      case ExprKind::INDEX: return "index";
    }

    return "<unknown-expr>";
  }

  struct Expr {
    ExprKind kind = ExprKind::PRIMITIVE;
    Primitive primitive = Primitive::UNKNOWN;

    SourceLocation locations;

    std::string value;
    std::string name;

    Expr* left = nullptr;
    Expr* right = nullptr;
    BinaryOp binOp = BinaryOp::UNK;
    UnaryOp unOp = UnaryOp::UNK;

    Expr* object = nullptr;
    Expr* callee = nullptr;
    Expr* exprValue = nullptr;

    std::vector<Expr*> args;
    std::vector<Type*> genericType;
    std::vector<Expr*> elements;
    std::vector<Expr*> keys;
    std::vector<Expr*> values;

    Stmt* block = nullptr;

    bool isSignedInteger = false;

    // semantic resolved
    Type* type = nullptr;
    std::string mangledName;
    Symbol* symbol = nullptr;

    Expr() = default;
  };

};
