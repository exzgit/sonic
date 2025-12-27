#include "value.h"
#include "type.h"
#include "stmt.h"
#include "visitor.h"

using namespace frontend;

void FunctionDecl::accept(Visitor& v) { v.visit(*this); }
void VarDecl::accept(Visitor& v) { v.visit(*this); }
void AssignDecl::accept(Visitor& v) { v.visit(*this); }
void BranchStmt::accept(Visitor& v) { v.visit(*this); }
void ExprStmt::accept(Visitor& v) { v.visit(*this); }
void Parameter::accept(Visitor& v) { v.visit(*this); }
void ForLoopStmt::accept(Visitor& v) { v.visit(*this); }
void BlockStmt::accept(Visitor& v) { v.visit(*this); }

// Expr accept implementations
void LiteralExpr::accept(Visitor& v) { v.visit(*this); }
void NoneExpr::accept(Visitor& v) { v.visit(*this); }
void IdentifierExpr::accept(Visitor& v) { v.visit(*this); }
void LookupExpr::accept(Visitor& v) { v.visit(*this); }
void CallExpr::accept(Visitor& v) { v.visit(*this); }
void VecExpr::accept(Visitor& v) { v.visit(*this); }
void MapExpr::accept(Visitor& v) { v.visit(*this); }
void BinOp::accept(Visitor& v) { v.visit(*this); }
void UnOp::accept(Visitor& v) { v.visit(*this); }
void AddressOf::accept(Visitor& v) { v.visit(*this); }
void GetValuePtr::accept(Visitor& v) { v.visit(*this); }

// Type accept implementations
void AutoType::accept(Visitor& v) { v.visit(*this); }
void VoidType::accept(Visitor& v) { v.visit(*this); }
void AnyType::accept(Visitor& v) { v.visit(*this); }
void StandardType::accept(Visitor& v) { v.visit(*this); }
void GenericType::accept(Visitor& v) { v.visit(*this); }
void QualifiedType::accept(Visitor& v) { v.visit(*this); }
void PointerType::accept(Visitor& v) { v.visit(*this); }
void ReferenceType::accept(Visitor& v) { v.visit(*this); }
