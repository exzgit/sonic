#pragma once

#include <nlohmann/json.hpp>
#include "ast.h"

namespace sonic::frontend::ast::json {

  using json = nlohmann::json;
  using namespace sonic::frontend::ast;

  template<typename E>
  int to_int(E e) {
    return static_cast<int>(e);
  }

  template<typename E>
  E from_int(int v) {
    return static_cast<E>(v);
  }

  template<typename T, typename Fn>
  json arr_ptr(const std::vector<std::unique_ptr<T>>& v, Fn fn) {
    json a = json::array();
    for (const auto& x : v) {
      a.push_back(fn(*x));
    }
    return a;
  }

  nlohmann::json serializeLoc(const SourceLocation& loc);
  SourceLocation deserializeLoc(const nlohmann::json& j);
  json serializeType(const Type& t);
  std::unique_ptr<Type> deserializeType(const json& j);
  static json serializeExpr(const Expression& e);
  std::unique_ptr<Expression> deserializeExpr(const json& j);
  json serializeStmt(const Statement& s);
  std::unique_ptr<Statement> deserializeStmt(const json& j);
  json serializeProgram(const Program& p);
  Program deserializeProgram(const json& j);
}
