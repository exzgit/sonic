#pragma once

#include "stmt.h"
#include "type.h"
#include "value.h"
#include "span.h"

#include <iostream>

using namespace std;

namespace frontend {

class Visitor {
  public:
  virtual ~Visitor() = default;

  // visitor for expression node
  virtual void visit(LiteralExpr& expr)       = 0;
  virtual void visit(NoneExpr& expr)          = 0;
  virtual void visit(IdentifierExpr& expr)    = 0;
  virtual void visit(LookupExpr& expr)        = 0;
  virtual void visit(CallExpr& expr)          = 0;
  virtual void visit(VecExpr& expr)           = 0;
  virtual void visit(MapExpr& expr)           = 0;
  virtual void visit(BinOp& expr)             = 0;
  virtual void visit(UnOp& expr)              = 0;
  virtual void visit(AddressOf& expr)         = 0;
  virtual void visit(GetValuePtr& expr)       = 0;
  virtual void visit(Parameter& expr)         = 0;
  virtual void visit(Range& expr)             = 0;

  // visitor for type
  virtual void visit(AutoType& type)          = 0;
  virtual void visit(VoidType& type)          = 0;
  virtual void visit(AnyType& type)           = 0;
  virtual void visit(StandardType& type)      = 0;
  virtual void visit(GenericType& type)       = 0;
  virtual void visit(QualifiedType& type)     = 0;
  virtual void visit(PointerType& type)       = 0;
  virtual void visit(ReferenceType& type)     = 0;

  // visitor for statement node
  virtual void visit(FunctionDecl& stmt)      = 0;
  virtual void visit(LetDecl& stmt)           = 0;
  virtual void visit(AssignDecl& stmt)        = 0;
  virtual void visit(BranchStmt& stmt)        = 0;
  virtual void visit(ExprStmt& stmt)          = 0;
  virtual void visit(ForLoopStmt& stmt)       = 0;
  virtual void visit(BlockStmt& stmt)         = 0;
};

};
