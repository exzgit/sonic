#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

#include "span.h"

using namespace std;

namespace frontend {

struct Expr;
struct Type;

class Visitor;

enum Mutability {
  STATIC,
  CONST,
  LETERAL
};

enum Visibility {
  PUBLIC,
  PRIVATE
};

inline string mutabilityToString(Mutability& mut) {
  switch (mut) {
    case Mutability::STATIC: return "static";
    case Mutability::CONST: return "const";
    case Mutability::LETERAL: return "leteral";
    default: return "__";
  }
}

inline string visibilityToString(Visibility v) {
  switch (v) {
    case Visibility::PRIVATE: return "private";
    case Visibility::PUBLIC: return "public";
    default: return "__";
  }
}

struct Stmt {
  virtual ~Stmt() = default;
  virtual void accept(Visitor& v) = 0;
};

struct FunctionDecl : Stmt {
  string name;
  unique_ptr<Type> return_types;
  vector<unique_ptr<Stmt>> params;
  unique_ptr<Stmt> body;

  Visibility visibility;

  bool hasTypeParams;
  vector<unique_ptr<Type>> typeParams;

  FunctionDecl(
      string name, 
      unique_ptr<Type> return_types, 
      vector<unique_ptr<Stmt>> params,
      unique_ptr<Stmt> body,
      Visibility vis,
      bool hasTypeParams,
      vector<unique_ptr<Type>> typeParams)
    : name(std::move(name)), return_types(std::move(return_types)),
    params(std::move(params)), body(std::move(body)),
    visibility(vis), hasTypeParams(hasTypeParams),
    typeParams(std::move(typeParams)) {}

  void accept(Visitor& v) override;
};

struct VarDecl : Stmt {
  string name;
  unique_ptr<Type> types;
  unique_ptr<Expr> values;

  Mutability mutability;
  Visibility visibility;

  VarDecl(string name, unique_ptr<Type> types, unique_ptr<Expr> values, Mutability mut, Visibility vis)
  : name(std::move(name)), types(std::move(types)), values(std::move(values)), mutability(mut), visibility(vis) {}

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
  unique_ptr<Stmt> body_if;
  unique_ptr<Stmt> body_else;

  BranchStmt(unique_ptr<Expr> conditions, unique_ptr<Stmt> body_if, unique_ptr<Stmt> body_else)
    : conditions(std::move(conditions)), body_if(std::move(body_if)), body_else(std::move(body_else)) {}

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

struct ForLoopStmt : Stmt {
  unique_ptr<Stmt> initializer;
  unique_ptr<Stmt> iterator;
  unique_ptr<Stmt> body;

  ForLoopStmt(unique_ptr<Stmt> initializer, unique_ptr<Stmt> iterator, unique_ptr<Stmt> body)
    : initializer(std::move(initializer)),
    iterator(std::move(iterator)),
    body(std::move(body)) {}

  void accept(Visitor& v) override;
};

struct BlockStmt : Stmt {
  vector<unique_ptr<Stmt>> children;

  BlockStmt(vector<unique_ptr<Stmt>> children) : children(std::move(children)) {}

  void accept(Visitor& v) override;
};

};
