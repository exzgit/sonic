#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "symbol.h"
#include "ast_json.h"
#include "ast.h"

namespace sonic::frontend {
  using namespace ast::json;

  inline nlohmann::json symbolToJson(Symbol* sym) {
    if (!sym) return nullptr;

    nlohmann::json j;
    j["name"] = sym->name_;
    j["mangle"] = sym->mangle_;
    j["kind"] = static_cast<int>(sym->kind_);
    j["scope"] = static_cast<int>(sym->scope_);
    j["public"] = sym->public_;
    j["extern"] = sym->extern_;
    j["async"] = sym->async_;
    j["decl"] = sym->decl_;
    j["variadic"] = sym->variadic_;
    j["mutability"] = static_cast<int>(sym->mutability_);
    j["parent"] = symbolToJson(sym->parent_);
    j["type"] = sym->type_ ? ast::json::serializeType(*sym->type_) : nullptr;

    // Parameters
    for (auto& param : sym->params_) {
      j["params"].push_back(ast::json::serializeType(*param));
    }

    // Children
    j["children"] = nlohmann::json::array();
    for (auto& child : sym->children_) {
      j["children"].push_back(symbolToJson(child));
    }

    j["ref"] = symbolToJson(sym->ref_);

    return j;
  }

  inline Symbol* jsonToSymbol(const nlohmann::json& j) {
    if (j.is_null()) return nullptr;

    Symbol* sym = new Symbol();
    sym->name_ = j.value("name", "");
    sym->mangle_ = j.value("mangle", "");
    sym->kind_ = static_cast<SymbolKind>(j.value("kind", 0));
    sym->scope_ = static_cast<ScopeLevel>(j.value("scope", 0));
    sym->public_ = j.value("public", 0);
    sym->extern_ = j.value("extern", 0);
    sym->async_ = j.value("async_", 0);
    sym->decl_ = j.value("decl", false);
    sym->variadic_ = j.value("variadic", false);
    sym->mutability_ = static_cast<ast::Mutability>(j.value("mutability", 0));
    sym->parent_ = jsonToSymbol(j.value("parent", nullptr));

    if (j.contains("type") && !j["type"].is_null()) {
      sym->type_ = ast::json::deserializeType(j["type"]).release();
    }

    // Parameters
    if (j.contains("params")) {
      for (auto& paramJson : j["params"]) {
        auto paramType = ast::json::deserializeType(paramJson);
        sym->params_.push_back(paramType.release());
      }
    }

    // Children
    if (j.contains("children")) {
      for (auto& childJson : j["children"]) {
        Symbol* childSym = jsonToSymbol(childJson);
        if (childSym) {
          sym->children_.push_back(childSym);
        }
      }
    }

    sym->ref_ = jsonToSymbol(j.value("ref", nullptr));

    return sym;
  }

}
