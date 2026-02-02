#pragma once

// c++ library
#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

// local headers
#include "ast.h"
#include "source.h"

namespace sonic::frontend {

  enum class ScopeLevel : uint8_t {
    GLOBAL,
    BLOCK,
    FUNC
  };

  enum class SymbolKind {
    NAMESPACE,
    FUNCTION,
    VARIABLE,
    UNKNOWN,
  };

  inline std::string symbolkind_to_string(SymbolKind mut) {
    switch(mut) {
      case sonic::frontend::SymbolKind::NAMESPACE: return "namespace";
      case sonic::frontend::SymbolKind::FUNCTION: return "function";
      case sonic::frontend::SymbolKind::VARIABLE: return "variable";
      default: return "unknown";
    }
  }

  class Symbol {
  public:
    SymbolKind kind = SymbolKind::UNKNOWN;
    std::string name_;
    std::string mangleName_;
    SonicType* type_ = nullptr;

    Symbol* parent_ = nullptr;

    std::vector<SonicType*> params_;
    std::unordered_map<std::string, SonicType*> generics_;
    std::unordered_map<std::string, Symbol*> children_;

    SourceLocation location;

    size_t offset           = 0;
    size_t depth            = 0;
    ScopeLevel scopeLevel   = ScopeLevel::FUNC;

    bool variadic_          = false;
    bool extern_            = false;
    Mutability mutability_  = Mutability::DEFAULT;
    bool public_            = false;

    Symbol* lookup_local(const std::string& name) {
      auto it = children_.find(name);
      if (it == children_.end()) return nullptr;
      return it->second;
    }

    Symbol* lookup(const std::string& name) {
      auto sym = lookup_local(name);
      if (!sym && parent_) return parent_->lookup(name);
      return sym;
    }

    bool existingLocal(const std::string& name) { return children_.find(name) != children_.end(); }

    bool exists(const std::string & name) {
      if (existingLocal(name)) return true;
      else if (parent_) return parent_->exists(name);
      else return false;
    }

    void declare(Symbol* sym) {
      children_[sym->name_] = sym;
    }

    std::unique_ptr<Symbol> clone() {
      auto sym = std::make_unique<Symbol>();
      sym->name_ = name_;
      sym->mangleName_ = mangleName_;
      sym->type_ = type_;
      sym->parent_ = parent_;
      sym->location = location.clone();
      sym->offset = offset;
      sym->depth = depth;
      sym->scopeLevel = scopeLevel;
      sym->variadic_ = variadic_;
      sym->extern_ = extern_;
      sym->mutability_ = mutability_;
      sym->public_ = public_;

      for (auto& generic : generics_) {
        sym->generics_.emplace(generic.first, generic.second);
      }

      for (auto& param : params_) {
        sym->params_.push_back(param);
      }

      for (auto& child : children_) {
        sym->children_.emplace(child.first, child.second);
      }

      return sym;
    }

    std::string to_string(int indent = 0) {
      std::string out;
      auto ind = indent_str(indent);

      out += ind + "(Symbol ";
      out += name_ + " kind=" + symbolkind_to_string(kind) + "\n";

      if (type_) {
        out += ind + "  type:\n";
        out += type_->to_string(indent + 2) + "\n";
      }

      if (!params_.empty()) {
        out += ind + "  params:\n";
        for (auto& p : params_) {
          out += ind + "    - " + typekind_to_string(p->kind) + "\n";
        }
      }

      out += ind + ")";
      return out;
    }
  };
};
