#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ast_fwd.h"
#include "stmt.h"

using namespace std;

namespace frontend {

  class Visitor;

  struct Expr : Stmt {
    virtual ~Expr() = default;
    virtual void accept(Visitor& v) = 0;
  };

  enum LiteralKind {
    String,
    Char,
    Digits,
    ScientificNotation,
    Hex,
    Boolean
  };

  inline string literalKindToString(LiteralKind& kind) {
    switch (kind) {

      case String: return "string";
      case Char: return "char";
      case Digits: return "digit";
      case ScientificNotation: return "scientific notation";
      case Hex: return "hex";
      case Boolean: return "boolean";
    }

    return "unknown";
  }

  struct LiteralExpr : Expr {
    LiteralKind kinds;
    string values;

    LiteralExpr(
      LiteralKind k,
      string v
    ) : kinds(std::move(k)), values(std::move(v)) {}
  
    void accept(Visitor& v) override;
  };

  struct NoneExpr : Expr {
    NoneExpr() {}
  
    void accept(Visitor& v) override;
  };

  struct IdentifierExpr : Expr {
    string values;

    IdentifierExpr(string v) : values(std::move(v)) {}
  
    void accept(Visitor& v) override;
  };

  struct LookupExpr : Expr {
    unique_ptr<Expr> object;
    unique_ptr<Expr> target;

    LookupExpr(unique_ptr<Expr> obj, unique_ptr<Expr> targ) : object(std::move(obj)), target(std::move(targ)) {}
 
    void accept(Visitor& v) override;
  };

  struct CallExpr : Expr {
    unique_ptr<Expr> callee;
    vector<unique_ptr<Expr>> arguments;
    bool hasTypeArgs;
    vector<unique_ptr<Type>> typeArgs;   // optional

    CallExpr(
      unique_ptr<Expr> callee,
      vector<unique_ptr<Expr>> args,
      bool hasTypeArgs,
      vector<unique_ptr<Type>> typeArgs
    ) :
    callee(std::move(callee)),
    arguments(std::move(args)),
    hasTypeArgs(hasTypeArgs),
    typeArgs(std::move(typeArgs)) {}
  
    void accept(Visitor& v) override;
  };

  struct VecExpr : Expr {
    vector<unique_ptr<Expr>> elems;

    VecExpr(vector<unique_ptr<Expr>> e) : elems(std::move(e)) {}
 
    void accept(Visitor& v) override;
  };

  struct MapExpr : Expr {
    vector<unique_ptr<Expr>> keys;
    vector<unique_ptr<Expr>> values;

    MapExpr(vector<unique_ptr<Expr>> keys,
    vector<unique_ptr<Expr>> values) : keys(std::move(keys)), values(std::move(values)) {}
  
    void accept(Visitor& v) override;
  };

  struct BinOp : Expr {
    string op;
    unique_ptr<Expr> lhs;
    unique_ptr<Expr> rhs;

    BinOp(string op,
    unique_ptr<Expr> lhs,
    unique_ptr<Expr> rhs)
    : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  
    void accept(Visitor& v) override;
  };

  struct UnOp : Expr {
    string op;
    unique_ptr<Expr> values;

    UnOp(string op, unique_ptr<Expr> values)
    : op(std::move(op)), values(std::move(values)) {}
 
    void accept(Visitor& v) override;
  };

  struct AddressOf : Expr {
    unique_ptr<Expr> target;

    AddressOf(unique_ptr<Expr> target)
    : target(std::move(target)) {}
 
    void accept(Visitor& v) override;
  };

  struct GetValuePtr : Expr {
    unique_ptr<Expr> addr;
    GetValuePtr(unique_ptr<Expr> addr) : addr(std::move(addr)) {}
    void accept(Visitor& v) override;
  };
};
