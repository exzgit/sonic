#pragma once

#include "visitor.h"
#include "ast_fwd.h"
#include <iostream>
#include <sstream>

using namespace std;

namespace frontend {

class ASTDebugger : public Visitor {
  private:
    int idx = 0;

    string indent() {
      string s;
      for (int i = 0; i < idx; i++)
        s += " ";
      return s;
    }

  public:
    void visit(LiteralExpr& expr) {
      cout << indent() << "- LiteralExpr\n";
      idx++;
        cout << indent() << "kind: " << literalKindToString(expr.kinds) << "\n";
        cout << indent() << "value: " << expr.values << "\n";
      idx--;
    }

    void visit(NoneExpr& expr) {
      cout << indent() << "- NoneExpr\n";
    }

    virtual void visit(IdentifierExpr& expr) {
      cout << indent() << "- Identifier\n";
      idx++;
        cout << indent() << "value: " << expr.values << "\n";
      idx--;
    }

    virtual void visit(LookupExpr& expr) {
      cout << indent() << "- LookupExpr\n";
      idx++;
        cout << indent() << "object:\n";
        idx++;
          expr.object->accept(*this);
        idx--;
        cout << indent() << "target:\n";
        idx++;
          expr.target->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(CallExpr& expr) {
      cout << indent() <<  "- CallExpr\n";
      idx++;
        cout << indent() << "callee:\n";
        idx++;
          expr.callee->accept(*this);
        idx--;
        cout << indent() << "arguments[" << expr.arguments.size() << "]\n"; 
        idx++;
          for (auto& arg : expr.arguments) arg->accept(*this);
        idx--;

      if (expr.hasTypeArgs) {
        cout << indent() << "has_type_args: " << (expr.hasTypeArgs ? "yes" : "no") << ")\n";
        cout << indent() << "type_args[" << expr.typeArgs.size() << "]\n";
        idx++;
          for (auto& arg : expr.typeArgs) {
            arg->accept(*this);
          }
        idx--;
      }

      idx--;
    }

    virtual void visit(VecExpr& expr) {
      cout << indent() <<  "- VecExpr[" << expr.elems.size() << "]\n";
      idx++;
        cout << indent() << "elements:\n";
        idx++;
          for (auto& e : expr.elems) e->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(MapExpr& expr) {
      cout << indent() << "- MapExpr\n";
      idx++;
        cout << indent() << "keys:\n";
        idx++;
          for (auto& k : expr.keys) k->accept(*this);
        idx--;
        cout << indent() << "values:\n";
        idx++;
          for (auto& v : expr.values) v->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(BinOp& expr) {
      cout << indent() << "- BinOp(" << expr.op << ")\n";
      idx++;
        cout << "lhs:\n";
        idx++;
          expr.lhs->accept(*this);
        idx--;
        cout << "rhs:\n";
        idx++;
          expr.rhs->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(UnOp& expr) {
      cout << indent() << "- UnOp(" << expr.op << ")\n";
      idx++;
        cout << "value: \n";
        idx++;
          expr.values->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(AddressOf& expr) {
      cout << indent() << "- AddressOf\n";
      idx++;
        cout << indent() << "target:\n";
        idx++;
          expr.target->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(GetValuePtr& expr) {
      cout << indent() << "- GetValuePtr\n";
      idx++;
        cout << indent() << "addr:\n";
        idx++;
          expr.addr->accept(*this);
        idx--;
      idx--;
    }

    // visitor for type
    virtual void visit(AutoType& type) {
      cout << indent() << "- Type(Auto)\n";
    }

    virtual void visit(VoidType& type) {
      cout << indent() << "- Type(Void)\n";
    }

    virtual void visit(AnyType& type) {
      cout << indent() << "- Type(Any)\n";
    }

    virtual void visit(StandardType& type) {
      cout << indent() << "- Type(StandardType)\n";
      idx++;
        cout << indent() << "types: " << standardTypeKindToString(type.kinds) << "\n";
      idx--;
    }

    virtual void visit(GenericType& type) {
      cout << indent() << "- Type(GenericType)\n";
      idx++;
        cout << indent() << "base:\n";
        idx++;
          type.base->accept(*this);
        idx--;
        cout << indent() << "types[" << type.types.size() << "]\n";
        idx++;
          for (auto& type : type.types) type->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(QualifiedType& type) {
      cout << indent() << "- Type(QualifiedType)\n";
      idx++;
        cout << indent() << "segments: ";
        for (int i = 0; i < type.segments.size(); i++) {
          cout << type.segments[i];
          if (i < type.segments.size() - 1) cout << "::";
        }
        cout << "\n";
      idx--;
    }

    virtual void visit(PointerType& type) {
      cout << indent() << "PointerType\n";
      idx++;
        cout << indent() << "base:\n";
        idx++;
          type.base->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(ReferenceType& type) {
      cout << indent() << "PointerType\n";
      idx++;
        cout << indent() << "base:\n";
        idx++;
          type.base->accept(*this);
        idx--;
      idx--;
    }

    // visitor for statement node
    virtual void visit(FunctionDecl& stmt) {
      cout << indent() << "FunctionDecl(" << stmt.name << ")\n";
      idx++;
        cout << indent() << "params[" << stmt.params.size() << "]\n";
        idx++;
          for (auto& arg : stmt.params) arg->accept(*this);
        idx--;
        cout << indent() << "body[" << stmt.body.size() << "]\n";
        idx++;
          for (auto& child : stmt.body) child->accept(*this);
        idx--;
        cout << indent() << "return_type:\n";
        idx++;
          stmt.return_types->accept(*this);
        idx--;
        cout << indent() << "isPublic: " << (stmt.isPublic ? "yes" : "no") << "\n";
        if (stmt.hasTypeParams) {
          cout << indent() << "typeParams[" << stmt.typeParams.size() << "]\n";
          idx++;
            for (auto& tParams : stmt.typeParams) tParams->accept(*this);
          idx--;
        }
      idx--;
    }

    virtual void visit(Parameter& stmt) {
      cout << indent() << "- Parameter(" << stmt.name << ")\n";
      idx++;
        cout << indent() << "types:\n";
        idx++;
          stmt.types->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(LeteralDecl& stmt) {
      cout << indent() << "- LeteralDecl(" << stmt.name << ")\n";
      idx++;
        cout << indent() << "types:\n";
        idx++;
          stmt.types->accept(*this);
        idx--;
        cout << indent() << "values:\n";
        idx++;
          stmt.values->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(AssignDecl& stmt) {
      cout << indent() << "- AssignDecl\n";
      idx++;
        cout << indent() << "target:\n";
        idx++;
          stmt.target->accept(*this);
        idx--;
        cout << indent() << "value:\n";
        idx++;
          stmt.values->accept(*this);
        idx--;
      idx--;
    }

    virtual void visit(BranchStmt& stmt) {
      cout << indent() << "- BranchStmt\n";
      idx++;
        cout << indent() << "condition:\n";
        idx++;
          stmt.conditions->accept(*this);
        idx--;
        cout << indent() << "body_if:\n";
        idx++;
          for (auto& child : stmt.body_if) child->accept(*this);
        idx--;
        if (stmt.body_else.size() > 0) {
          cout << indent() << "body_else:\n";
          idx++;
            for (auto& child : stmt.body_else) child->accept(*this);
          idx--;
        }
      idx--;
    }

    virtual void visit(ExprStmt& stmt) {
      stmt.expr->accept(*this);
    }
};

};
