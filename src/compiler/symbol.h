#pragma once

// c++ library
#include <cstdint>
#include <llvm/IR/Value.h>
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

    llvm::Value* llvmValue = nullptr;

    Symbol* lookup_local(const std::string& name) {
      auto it = children_.find(name);
      return it != children_.end() ? it->second : nullptr;
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

    std::string to_string(int indent = 0) const {
      auto pad = [&](int n) {
        return std::string(n * 2, ' ');
      };

      std::string out;
      out += pad(indent);
      out += "Symbol {\n";

      out += pad(indent + 1) + "kind: " + symbolkind_to_string(kind) + "\n";
      out += pad(indent + 1) + "name: " + name_ + "\n";

      if (!mangleName_.empty())
        out += pad(indent + 1) + "mangled: " + mangleName_ + "\n";

      out += pad(indent + 1) + "type:\n";
      if (type_)
        out += type_->to_string(indent + 2) + "\n";
      else
        out += std::string(indent + 2, ' ') +  "<null>\n";

      out += pad(indent + 1) + "scope: ";
      switch (scopeLevel) {
        case ScopeLevel::GLOBAL: out += "GLOBAL"; break;
        case ScopeLevel::FUNC:   out += "FUNC"; break;
        case ScopeLevel::BLOCK:  out += "BLOCK"; break;
      }
      out += "\n";

      out += pad(indent + 1) + "depth: " + std::to_string(depth) + "\n";
      out += pad(indent + 1) + "offset: " + std::to_string(offset) + "\n";

      if (!params_.empty()) {
        out += pad(indent + 1) + "params:\n";
        for (auto* p : params_) {
          out += pad(indent + 2);
          out += p ? p->name : "<null>";
          out += "\n";
        }
      }

      if (!generics_.empty()) {
        out += pad(indent + 1) + "generics:\n";
        for (auto& g : generics_) {
          out += pad(indent + 2) + g.first + " : ";
          out += g.second ? g.second->name : "<null>";
          out += "\n";
        }
      }

      if (!children_.empty()) {
        out += pad(indent + 1) + "children[" + std::to_string(children_.size()) + "]:\n";
        for (auto& [name, child] : children_) {
          if (!child) continue;
          out += child->to_string(indent + 2);
        }
      }

      out += pad(indent) + "}\n";
      return out;
    }

  };
};
