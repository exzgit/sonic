#pragma once

#include <string>
#include <vector>

#include "source.h"
#include "stmt.h"
#include "expr.h"
#include "type.h"

#include <iostream>

#include "ast/debug.h"

using namespace std;

namespace sonic::frontend {

  class SymbolTable;

  enum class SymbolKind {
    FUNCTION,
    PARAMETER,
    LETDECL,
    STRUCT,
    ENUM,
    VARIABLE,
    NAMESPACE,
    BLOCK,
    ALIAS,
    UNKNOWN,
  };

  inline std::string symbolKindToString(SymbolKind kind) {
    switch (kind) {
      case SymbolKind::FUNCTION: return "function";
      case SymbolKind::PARAMETER: return "parameter";
      case SymbolKind::LETDECL: return "let declaration";
      case SymbolKind::STRUCT: return "struct";
      case SymbolKind::ENUM: return "enum";
      case SymbolKind::VARIABLE: return "variable";
      case SymbolKind::NAMESPACE: return "namespace";
      case SymbolKind::BLOCK: return "block";
      case SymbolKind::ALIAS: return "alias";
      case SymbolKind::UNKNOWN: return "unknown";
    }
    return "unknown";
  }

  enum ScopeLevel {
    GLOBAL_SCOPE = 0,
    FUNCTION_SCOPE = 1,
    BLOCK_SCOPE = 2,
  };

  class Symbol {
    public:
    SymbolKind kind = SymbolKind::UNKNOWN;
    std::string name;
    std::string mangledName;
    SourceLocation location;

    Type* dataType = nullptr;
    Type* returnType = nullptr;

    Expr* expression = nullptr;

    Stmt* statement = nullptr;

    vector<Type*> parameterTypes;
    vector<Generic*> genericTypes;
    vector<MacroAttr*> macroAttributes;

    bool isConst    = false;
    bool isStatic   = false;
    bool isAlive    = true;
    bool isVariadic = false;
    bool isExtern   = false;
    bool isDeclare  = false;
    bool isPublic   = false;

    Symbol* ref = nullptr;
    SymbolTable* table = nullptr;

    ScopeLevel scopeLevel = GLOBAL_SCOPE;

    Symbol() = default;
    ~Symbol() = default;
  };

  class SymbolTable {
    public:
    Symbol* parent = nullptr;
    std::vector<Symbol*> symbols;

    SymbolTable() = default;
    explicit SymbolTable(Symbol* parent) : parent(parent) {}
    ~SymbolTable() = default;

    bool is_exists(const std::string& name) {
      for (auto symbol : symbols) {
        if (symbol->name == name) {
          return true;
        }
      }
      return false;
    }

    Symbol* find(const std::string& name) {
      for (auto& symbol : symbols) {
        if (symbol->name == name) {
          return symbol;
        }
      }

      if (!parent) return nullptr;

      return parent->table->find(name);
    }

    Symbol* find_local(const std::string& name) {
      for (auto& symbol : symbols) {
        if (symbol->name == name) {
          return symbol;
        }
      }
      return nullptr;
    }

    void insert(Symbol* symbol) {
      symbols.push_back(symbol);
    }
  };

  inline void printSymbol(const Symbol* symbol, int indent = 0) {
    if (!symbol) {
      std::cout << "<symbol-nullptr>\n";
      return;
    }

    if (symbol->kind != SymbolKind::UNKNOWN) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "symbol@" << symbolKindToString(symbol->kind) << " " << symbol->name << " (" << symbol->mangledName << ")\n";
    }

    if (symbol->ref) {
      std::cout << "ref:\n";
      printSymbol(symbol->ref, indent + 1);
    }

    if (symbol->table) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "symbol table:\n";
      for (auto& sym : symbol->table->symbols) {
        printSymbol(sym, indent + 1);
      }
    }

    if (symbol->statement) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "defined at: " << symbol->statement->locations.toString() << "\n";
    }

    if (symbol->dataType) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "data type:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      printer.print_type(symbol->dataType);
    }

    if (symbol->returnType) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "return type:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      printer.print_type(symbol->returnType);
    }

    if (symbol->statement) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "statement:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      printer.print_stmt(symbol->statement);
    }

    if (symbol->expression) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "expression:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      printer.print_expr(symbol->expression);
    }

    if (!symbol->genericTypes.empty()) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "generic types:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      for (auto& generic : symbol->genericTypes) {
        printer.print_type(generic->type);
      }
    }

    if (!symbol->macroAttributes.empty()) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "macro attributes:\n";
      ASTPrinter printer;
      printer.indent_level = indent + 1;
      for (auto& macro : symbol->macroAttributes) {
        printer.print(macro->name);
      }
    }

    if (symbol->isConst) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isConst: true\n";
    }

    if (symbol->isStatic) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isStatic: true\n";

    }

    if (symbol->isAlive) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isAlive: true\n";
    }

    if (symbol->isVariadic) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isVariadic: true\n";
    }

    if (
    symbol->isExtern) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isExtern: true\n";
    }

    if (symbol->isDeclare) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isDeclare: true\n";
    }

    if (symbol->isPublic) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "isPublic: true\n";
    }

    if (!symbol->location.path.empty()) {
      for (int i = 0; i < indent; i++) std::cout << "  ";
      std::cout << "location: " << symbol->location.toString() << "\n";
    }
  }
}
