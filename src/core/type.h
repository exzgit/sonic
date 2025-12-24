#pragma once

#include <string>
#include <memory>

struct Type;

struct Symbol;

using TypePtr = std::shared_ptr<Type>;

enum TypeKind {
  Primitive,
  Struct,
  Enum,
  Vector,
  Map,
  Pointer,
  Function,
  GenericParam,
  GenericInstance
};


enum TypePrimitive {
  String,
  Char,
  I8,
  I16,
  I32,
  I64,
  I128,
  U8,
  U16,
  U32,
  U64,
  U128,
  Bool
};

struct Type {
  TypeKind kind;

  // for primitive types
  TypePrimitive primitive;

  // for struct / enum / named types
  std::string name;
  Symbol* symbol = nullptr;

  // for function
  std::vector<TypePtr> param_types;
  TypePtr return_type;

  // for generic instantiation
  std::vector<TypePtr> type_args;

  // for generic param
  std::string generic_name;

  // modifiers
  bool is_mutable = false;
};
