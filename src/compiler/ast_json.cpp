// ast_json.cpp

#include "ast_json.h"
#include "ast.h"
#include <iostream>

namespace sonic::frontend::ast::json {

  using json = nlohmann::json;
  using namespace sonic::frontend::ast;

  nlohmann::json serializeLoc(const SourceLocation& loc) {
    return {
      {"path", loc.path},
      {"lines", loc.lines},
      {"raw", loc.raw_value},

      {"line", loc.line},
      {"column", loc.column},
      {"offset", loc.offset},
      {"start", loc.start},
      {"end", loc.end},
    };
  }

  SourceLocation deserializeLoc(const nlohmann::json& j) {
    SourceLocation loc;

    loc.path      = j.value("path", "");
    loc.lines     = j.value("lines", "");
    loc.raw_value = j.value("raw", "");

    loc.line   = j.value("line", 0u);
    loc.column = j.value("column", 0u);
    loc.offset = j.value("offset", 0u);
    loc.start  = j.value("start", loc.column);
    loc.end    = j.value("end", loc.start + 1);

    return loc;
  }

  json serializeType(const Type& t) {
    json j;
    j["kind"] = to_int(t.kind_);
    j["literal"] = to_int(t.literal_);
    j["name"] = t.name_;
    j["nullable"] = t.nullable_;
    j["loc"] = serializeLoc(t.loc_);

    if (t.nested_)
      j["nested"] = serializeType(*t.nested_);

    j["generics"] = json::array();
    for (auto& g : t.generics_)
      j["generics"].push_back(serializeType(*g));

    return j;
  }

  std::unique_ptr<Type> deserializeType(const json& j) {
    auto t = std::make_unique<Type>();
    t->kind_ = from_int<TypeKind>(j.at("kind").get<int>());
    t->literal_ = from_int<LiteralKind>(j.at("literal").get<int>());
    t->name_ = j.value("name", "");
    t->nullable_ = j.value("nullable", false);
    t->loc_ = deserializeLoc(j.at("loc"));

    if (j.contains("nested"))
      t->nested_ = deserializeType(j["nested"]);

    for (auto& g : j["generics"])
      t->generics_.push_back(deserializeType(g));

    return t;
  }

  json serializeExpr(const Expression& e) {
    json j;
    j["kind"] = to_int(e.kind_);
    j["literal"] = to_int(e.literal_);
    j["name"] = e.name_;
    j["value"] = e.value_;
    j["raw"] = e.raw_;
    j["loc"] = serializeLoc(e.loc_);

    j["generics"] = json::array();
    for (auto& g : e.generics_)
      j["generics"].push_back(serializeType(*g));

    j["args"] = json::array();
    for (auto& a : e.args_)
      j["args"].push_back(serializeExpr(*a));

    if (e.nested_)  j["nested"]  = serializeExpr(*e.nested_);
    if (e.index_)   j["index"]   = serializeExpr(*e.index_);
    if (e.callee_)  j["callee"]  = serializeExpr(*e.callee_);
    if (e.lhs_)     j["lhs"]     = serializeExpr(*e.lhs_);
    if (e.rhs_)     j["rhs"]     = serializeExpr(*e.rhs_);

    return j;
  }

  std::unique_ptr<Expression> deserializeExpr(const json& j) {
    auto e = std::make_unique<Expression>();
    e->kind_ = from_int<ExprKind>(j.at("kind").get<int>());
    e->literal_ = from_int<LiteralKind>(j.at("literal").get<int>());
    e->name_ = j.value("name", "");
    e->value_ = j.value("value", "");
    e->raw_ = j.value("raw", "");

    e->loc_ = deserializeLoc(j.at("loc"));

    for (auto& g : j["generics"])
      e->generics_.push_back(deserializeType(g));

    for (auto& a : j["args"])
      e->args_.push_back(deserializeExpr(a));

    if (j.contains("nested")) e->nested_ = deserializeExpr(j["nested"]);
    if (j.contains("index"))  e->index_  = deserializeExpr(j["index"]);
    if (j.contains("callee")) e->callee_ = deserializeExpr(j["callee"]);
    if (j.contains("lhs"))    e->lhs_    = deserializeExpr(j["lhs"]);
    if (j.contains("rhs"))    e->rhs_    = deserializeExpr(j["rhs"]);

    return e;
  }


  json serializeStmt(const Statement& s) {
    json j;
    j["kind"] = to_int(s.kind_);
    j["name"] = s.name_;
    j["public"] = s.public_;
    j["extern"] = s.extern_;
    j["async"] = s.async_;
    j["mutability"] = to_int(s.mutability);
    j["declare"] = s.declare_;
    j["variadic"] = s.variadic_;
    j["import_all"] = s.import_all_;
    j["import_alias"] = s.import_alias_;

    j["loc"] = serializeLoc(s.loc_);

    if (s.assign_) j["assign"] = serializeExpr(*s.assign_);
    if (s.value_)  j["value"]  = serializeExpr(*s.value_);
    if (s.type_)   j["type"]   = serializeType(*s.type_);

    j["import_qualified"] = arr_ptr(s.import_qualified_, serializeStmt);
    j["import_items"] = arr_ptr(s.import_items_, serializeStmt);

    j["generics"] = arr_ptr(s.generics_, serializeStmt);
    j["params"] = arr_ptr(s.params_, serializeStmt);
    j["body"] = arr_ptr(s.body_, serializeStmt);
    j["then"] = arr_ptr(s.then_, serializeStmt);
    j["else"] = arr_ptr(s.else_, serializeStmt);
    j["try"] = arr_ptr(s.try_, serializeStmt);
    j["catch"] = arr_ptr(s.catch_, serializeStmt);
    j["finally"] = arr_ptr(s.finally_, serializeStmt);

    return j;
  }

  std::unique_ptr<Statement> deserializeStmt(const json& j) {
    auto s = std::make_unique<Statement>();
    s->kind_ = from_int<StmtKind>(j.at("kind").get<int>());
    s->name_ = j.value("name", "");
    s->public_ = j.value("public", 0);
    s->extern_ = j.value("extern", 0);
    s->async_ = j.value("async", 0);
    s->mutability = from_int<Mutability>(j.at("mutability").get<int>());
    s->declare_ = j.value("declare", false);
    s->variadic_ = j.value("variadic", false);
    s->import_all_ = j.value("import_all", false);
    s->import_alias_ = j.value("import_alias", "");

    s->loc_ = deserializeLoc(j.at("loc"));

    if (j.contains("assign")) s->assign_ = deserializeExpr(j["assign"]);
    if (j.contains("value"))  s->value_  = deserializeExpr(j["value"]);
    if (j.contains("type"))   s->type_   = deserializeType(j["type"]);

    auto load = [&](auto& vec, const char* key) {
      if (!j.contains(key)) return;
      for (auto& v : j[key])
        vec.push_back(deserializeStmt(v));
    };

    load(s->generics_, "generics");
    load(s->import_items_, "import_items");
    load(s->import_qualified_, "import_qualified");
    load(s->params_, "params");
    load(s->body_, "body");
    load(s->then_, "then");
    load(s->else_, "else");
    load(s->try_, "try");
    load(s->catch_, "catch");
    load(s->finally_, "finally");

    return s;
  }

  json serializeProgram(const Program& p) {
    json j;
    j["name"] = p.name_;
    j["statements"] = json::array();
    for (auto& s : p.statements_)
      j["statements"].push_back(serializeStmt(*s->clone()));
    return j;
  }

  Program deserializeProgram(const json& j) {
    Program p;
    p.name_ = j.value("name", "");
    for (auto& s : j["statements"])
      p.statements_.push_back(deserializeStmt(s));
    return p;
  }
};
