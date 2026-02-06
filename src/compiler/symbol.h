#pragma once

// c++ library
#include <cstdint>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

// local headers
#include "ast.h"

namespace sonic::frontend {

  enum class ScopeLevel : uint8_t {
    GLOBAL,
    STRUCT,
    FUNCTION,
  };

  enum class SymbolKind {
    NAMESPACE,
    FUNCTION,
    STRUCT,
    ENUM,
    VARIABLE,
    ALIAS,
    UNKNOWN,
  };

  inline std::string symbolkind_to_string(SymbolKind mut) {
    switch(mut) {
      case sonic::frontend::SymbolKind::NAMESPACE: return "namespace";
      case sonic::frontend::SymbolKind::FUNCTION: return "function";
      case sonic::frontend::SymbolKind::VARIABLE: return "variable";
      case sonic::frontend::SymbolKind::ALIAS: return "alias";
      default: return "unknown";
    }
  }

  struct Symbol {
    SymbolKind kind_ = SymbolKind::UNKNOWN;
    ScopeLevel scope_ = ScopeLevel::GLOBAL;

    // variable | function | struct | alias | enum | etc
    std::string name_ = "";
    std::string mangle_ = "";

    // function
    std::vector<ast::Type*> params_;
    ast::Type* type_ = nullptr;
    bool variadic_;

    Symbol* parent_ = nullptr;
    std::vector<Symbol*> children_;

    // function decl | variable decl
    bool decl_ = false;
    bool public_ = false;
    bool extern_ = false;
    bool async_ = false;

    ast::Mutability mutability_ = ast::Mutability::VARIABLE;

    Symbol* ref_ = nullptr;


    // llvm
    llvm::Type* llvm_type_ = nullptr;
    llvm::Value* llvm_value_ = nullptr;
    llvm::Function* llvm_function_ = nullptr;

    Symbol() = default;
    Symbol(std::string name) : name_(name) {}
    ~Symbol() = default;

    Symbol* clone() {
      auto symbol = new Symbol();
      symbol->kind_ = kind_;
      symbol->scope_ = scope_;
      symbol->name_ = name_;
      symbol->mangle_ = mangle_;
      symbol->variadic_ = variadic_;
      symbol->public_ = public_;
      symbol->extern_ = extern_;
      symbol->async_ = async_;
      symbol->decl_ = decl_;
      symbol->mutability_ = mutability_;

      if (parent_) symbol->parent_ = parent_->clone();
      if (type_) symbol->type_ = type_;
      if (ref_) symbol->ref_ = ref_;
      if (llvm_type_) symbol->llvm_type_ = llvm_type_;
      if (llvm_value_) symbol->llvm_value_ = llvm_value_;
      if (llvm_function_) symbol->llvm_function_ = llvm_function_;

      for (auto& par : params_) symbol->params_.push_back(par);
      for (auto& child : children_) symbol->children_.push_back(child);

      return symbol;
    }

    Symbol* lookup(const std::string& name) {
      for (auto& c : children_) {
        if (c->name_ == name) {
          return c;
        }
      }
      if (parent_) return parent_->lookup(name);
      return nullptr;
    }

    bool exists(const std::string& name) {
      for (auto& c : children_) {
        if (c->name_ == name) return true;
      }

      return false;
    }

    void declare(Symbol* sy) {
      if (exists(sy->name_)) return;
      children_.push_back(sy);
    }
  };
};
