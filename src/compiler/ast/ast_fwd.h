#pragma once

namespace frontend {



	struct Stmt;
	struct Expr;
	struct Type;

	struct FunctionDecl;
	struct VarDecl;
	struct AssignDecl;
	struct BranchStmt;
  struct ExprStmt;
  struct Parameter;
  struct ForLoopStmt;
  struct BlockStmt;

  struct LiteralExpr;
	struct NoneExpr;
	struct IdentifierExpr;
	struct LookupExpr;
	struct CallExpr;
	struct VecExpr;
	struct MapExpr;
	struct Range;
	struct BinOp;
	struct UnOp;
	struct AddressOf;
	struct GetValuePtr;

	struct AutoType;
	struct VoidType;
	struct AnyType;
	struct StandardType;
	struct GenericType;
	struct QualifiedType;
	struct PointerType;
	struct ReferenceType;

};
