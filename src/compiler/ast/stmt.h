#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

using namespace std;

namespace frontend {

struct Expr;
struct Type;

class Visitor;

struct Stmt {
  virtual ~Stmt() = default;
  virtual void accept(Visitor& v) = 0;
};

struct FunctionDecl : Stmt {
  string name;
  unique_ptr<Type> return_types;
  vector<unique_ptr<Stmt>> params;
  vector<unique_ptr<Stmt>> body;

  bool isPublic;

  bool hasTypeParams;
  vector<unique_ptr<Type>> typeParams;

  FunctionDecl(
      string name, 
      unique_ptr<Type> return_types, 
      vector<unique_ptr<Stmt>> params, 
      vector<unique_ptr<Stmt>> body,
      bool isPublic,
      bool hasTypeParams,
      vector<unique_ptr<Type>> typeParams)
    : name(std::move(name)), return_types(std::move(return_types)),
    params(std::move(params)), body(std::move(body)),
    isPublic(isPublic), hasTypeParams(hasTypeParams),
    typeParams(std::move(typeParams)) {}

  void accept(Visitor& v) override;
};

struct LeteralDecl : Stmt {
  string name;
  unique_ptr<Type> types;
  unique_ptr<Expr> values;

  LeteralDecl(string name, unique_ptr<Type> types, unique_ptr<Expr> values)
  : name(std::move(name)), types(std::move(types)), values(std::move(values)) {}

  void accept(Visitor& v) override;
};

struct AssignDecl : Stmt {
  string op;
  unique_ptr<Expr> target;
  unique_ptr<Expr> values;

  AssignDecl(string op, unique_ptr<Expr> target, unique_ptr<Expr> values) : op(std::move(op)), target(std::move(target)), values(std::move(values)) {}

  void accept(Visitor& v) override;
};

struct BranchStmt : Stmt {
  unique_ptr<Expr> conditions;
  vector<unique_ptr<Stmt>> body_if;
  vector<unique_ptr<Stmt>> body_else;

  void accept(Visitor& v) override;
};

struct ExprStmt : Stmt {
  unique_ptr<Expr> expr;
  ExprStmt(unique_ptr<Expr> expr) : expr(std::move(expr)) {}

  void accept(Visitor& v) override;
};

struct Parameter : Stmt {
  string name;
  unique_ptr<Type> types;

  Parameter(string name, unique_ptr<Type> types) : name(std::move(name)), types(std::move(types)) {}

  void accept(Visitor& v) override;
};

};
