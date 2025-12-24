#pragma once

#include <string>
#include <memory>
#include <vector>

using namespace std;

namespace frontend {

  enum SymbolKind {
    Function,
    Variable,
    Struct,
    Enum,
    Parameter
  };

  struct Symbol {
    // symbol identifier
    string name;
    SymbolKind kind;
    
    size_t scope_level;
    size_t offset;

    // for function
    vector<Symbol*> params;
    Type return_type;
    Type generic_params;
  };
};
