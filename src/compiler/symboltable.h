#pragma once

#include <string>
#include <memory>

using namespace std;

#define GLOBAL_SCOPE_INDEX 0

namespace frontend {

  enum SymbolType {
    BlockFn,
    Variable,
    Struct,
    Enum,
    Unknown,
  };

  enum Mutability {
    Static,
    Constant,
    Default
  };

  enum Visibility {
    Public,
    Private
  };

  struct PrimitiveType {
    
  };

  struct DataType {

  };

  struct Symbol {
    string name = "__symbol__";
    SymbolType type = SymbolType::Unknown;

    Mutability mutability = Mutability::Default;
    Visibility visibility = Visibility::Private;

  };
};
