#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace frontend {

// Strongly-typed enums for clarity and ABI stability
enum class SymbolKind : uint8_t { Function, Variable, Struct, Enum, Block, Unknown };
enum class Mutability : uint8_t { Static, Constant, Mutable };
enum class Visibility : uint8_t { Public, Private, Protected };

enum class PrimitiveType : uint8_t {
  U8, U16, U32, U64, U128,
  I8, I16, I32, I64, I128,
  F32, F64,
  Bool, String, Char
};

// A compact, extensible representation of a type for the symbol table.
struct TypeInfo {
  enum class Kind : uint8_t { Primitive, Named, Generic, Function, Unknown } kind = Kind::Unknown;

  // payload depending on kind
  std::optional<PrimitiveType> primitive;
  std::string name; // for named types (structs, enums, aliases)
  std::vector<TypeInfo> generics; // generic type parameters

  // for function types
  std::vector<TypeInfo> params; // parameter types
  std::shared_ptr<TypeInfo> return_type; // use pointer to avoid recursive containment

  bool is_reference = false;
  bool is_any = false; // true when the type is `Any`

  // utility constructors
  static TypeInfo make_primitive(PrimitiveType p) {
    TypeInfo t; t.kind = Kind::Primitive; t.primitive = p; return t;
  }

  static TypeInfo make_named(std::string n) {
    TypeInfo t;
    t.kind = Kind::Named;
    t.name = std::move(n);
    return t;
  }

  static TypeInfo make_function(std::vector<TypeInfo> p, TypeInfo r) {
    TypeInfo t; t.kind = Kind::Function; t.params = std::move(p); t.return_type = std::make_shared<TypeInfo>(std::move(r)); return t;
  }

  static TypeInfo make_any() {
    TypeInfo t; t.kind = Kind::Unknown; t.is_any = true; return t;
  }
};

// SymbolEntry represents a declared or defined symbol in a particular scope.
struct SymbolEntry {
  std::string name;
  SymbolKind kind = SymbolKind::Unknown;
  Mutability mutability = Mutability::Mutable;
  Visibility visibility = Visibility::Private;

  TypeInfo type; // type of the symbol (for variables) or function signature

  // index of the scope where this symbol was declared (0 = global)
  size_t scope_index = 0;

  // optional children (e.g., members of a struct, or inner symbols)
  std::vector<std::shared_ptr<SymbolEntry>> children;

  // source location / metadata can be added later
};

// A simple hierarchical symbol table with scope stack and lexical lookup.
class SymbolTable {
public:
  SymbolTable();

  // Scope management
  size_t push_scope(std::string name = "");
  void pop_scope();
  size_t current_scope() const noexcept { return scopes_.size() - 1; }

  // Insert a symbol into the current scope. Returns false if a symbol
  // with the same name already exists in the current scope.
  bool insert(std::shared_ptr<SymbolEntry> entry);

  // Lookup by name. If `current_scope_only` is false, search up the scope stack.
  std::optional<std::shared_ptr<SymbolEntry>> lookup(const std::string& name, bool current_scope_only = false) const;

  // Clear all scopes and restore a single global scope
  void clear();

private:
  // Each scope is a map from name -> symbol entry.
  std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>> scopes_;
};

} // namespace frontend
