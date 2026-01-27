#pragma once

#include <string>
#include <vector>
#include "../source.h"

namespace sonic::frontend {
  struct Symbol;

  enum class Primitive {
    I32,
    I64,
    I128,
    F32,
    F64,
    BOOL,
    CHAR,
    STR,
    UNKNOWN,
  };

  enum class TypeKind {
    PRIMITIVE,
    FUNC,
    LOOKUP,
    IDENT,
    AUTO,
    ANY,
    VOID,
    NULLABLE,
  };

  inline std::string typeKindToString(TypeKind k) {
    switch (k) {
      case TypeKind::PRIMITIVE: return "primitive";
      case TypeKind::FUNC: return "func";
      case TypeKind::LOOKUP: return "lookup";
      case TypeKind::IDENT: return "ident";
      case TypeKind::AUTO: return "auto";
      case TypeKind::ANY: return "any";
      case TypeKind::VOID: return "void";
      case TypeKind::NULLABLE: return "nullable";
    }

    return "<unknown-type>";
  }

  inline std::string primitiveToString(Primitive p) {
    switch (p) {
      case Primitive::I32: return "i32";
      case Primitive::I64: return "i64";
      case Primitive::I128: return "i128";
      case Primitive::F32: return "f32";
      case Primitive::F64: return "f64";
      case Primitive::BOOL: return "bool";
      case Primitive::CHAR: return "char";
      case Primitive::STR: return "str";
      case Primitive::UNKNOWN: return "<unknown>";
    }

    return "<unknown-primitive>";
  }

  struct Type {
    TypeKind kind = TypeKind::VOID;

    SourceLocation locations;

    Primitive primitive = Primitive::UNKNOWN;

    std::string name;
    std::string mangledName;

    std::vector<Type*> genericType;
    Type* object = nullptr;

    bool isSignedInteger = false;
    bool isPointer = false;
    bool isReference = false;
    bool isArray = false;
    bool isStruct = false;
    bool isEnum = false;
    bool isNullable = false;

    Symbol* symbol = nullptr;

    Type() = default;
    ~Type() = default;
  };
}
