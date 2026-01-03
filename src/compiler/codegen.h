#pragma once

#include "ast/visitor.h"
#include "ast/ast_fwd.h"
#include "ast/value.h"
#include <memory>
#include <string>
#include <vector>

using namespace frontend;
using namespace std;

namespace backend {

    // generate intermediate code from ast to llvm ir.
    class Codegen : public Visitor {
    public:
        Codegen() = default;
        ~Codegen() = default;
    private:
        string code_generated;        
        // visitor for expression node
        virtual void visit(LiteralExpr& expr) {
            if (expr.kinds == LiteralKind::String) {
                code_generated += "\"" + expr.values + "\"\n";
            } else if (expr.kinds == LiteralKind::Digits) {
                code_generated += expr.values;
            } else if (expr.kinds == LiteralKind::Hex) {
                code_generated += to_string(stoll(expr.values, nullptr, 16));
            } else if (expr.kinds == LiteralKind::Boolean) {
                code_generated += expr.values;
            } else if (expr.kinds == LiteralKind::Char) {
                code_generated += expr.values;
            }
        }

        virtual void visit(NoneExpr& expr) {
            code_generated += "NULL\n";
        }

        virtual void visit(IdentifierExpr& expr) {
            code_generated += expr.values + "\n";
        }

        virtual void visit(LetDecl& stmt) {
          code_generated += stmt.types->accept(*this);
          code_generated += stmt.name + " = ";
          code_generated += stmt.values->accept(*this) + ";\n";
        }
    };
};
