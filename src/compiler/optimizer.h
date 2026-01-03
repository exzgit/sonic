#pragma once

#include "ast/visitor.h"
#include "ast/ast_fwd.h"
#include "ast/value.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <cmath>

using namespace std;

namespace frontend {
    class Optimizer : public Visitor {
        public:
        Optimizer() = default;
        ~Optimizer() = default;
        
        private:
        // visitor for expression node
        virtual void visit(LiteralExpr& expr) {

        }

        virtual void visit(NoneExpr& expr) {

        }
        
        virtual void visit(IdentifierExpr& expr) {
            // mark usage
            use_count[expr.values]++;
        }
        
        virtual void visit(LookupExpr& expr) {

        }

        virtual void visit(CallExpr& expr) {

        }

        virtual void visit(VecExpr& expr) {

        }

        virtual void visit(MapExpr& expr) {

        }

        virtual void visit(BinOp& expr) {
            // constant folding handled in optimizeExprPtr
        }

        virtual void visit(UnOp& expr) {

        }

        virtual void visit(AddressOf& expr) {

        }

        virtual void visit(GetValuePtr& expr) {

        }

        virtual void visit(Parameter& expr) {

        }

        virtual void visit(Range& expr) {

        }

        // visitor for type
        virtual void visit(AutoType& type) {

        }

        virtual void visit(VoidType& type) {

        }

        virtual void visit(AnyType& type) {

        }

        virtual void visit(StandardType& type) {

        }

        virtual void visit(GenericType& type) {

        }

        virtual void visit(QualifiedType& type) {

        }

        virtual void visit(PointerType& type) {

        }

        virtual void visit(ReferenceType& type) {

        }

        // visitor for statement node
        virtual void visit(FunctionDecl& stmt) {

        }

        virtual void visit(LetDecl& stmt) {

        }

        virtual void visit(AssignDecl& stmt) {

        }

        virtual void visit(BranchStmt& stmt) {

        }

        virtual void visit(ExprStmt& stmt) {

        }

        virtual void visit(ForLoopStmt& stmt) {

        }

        virtual void visit(BlockStmt& stmt) {
            // run optimization on nested block
            optimizeBlock(stmt);
        }

        // non-visitor helpers
        unordered_map<string,int> use_count;

        // entry point: optimize a block's children in-place
        void optimizeBlock(BlockStmt& block) {
            use_count.clear();
            // first pass: count identifier usages
            for (auto &c : block.children) if (c) countUsagesStmt(c.get());

            // second pass: optimize each statement (constant-folding, simplify)
            for (auto &c : block.children) if (c) optimizeStmtPtr(c);

            // third pass: prune unused let decls and trivial expr stmts
            vector<unique_ptr<Stmt>> kept;
            for (auto &c : block.children) {
                if (!c) continue;
                if (auto let = dynamic_cast<LetDecl*>(c.get())) {
                    if (!let->is_public && use_count[let->name] == 0) {
                        // remove unused local let
                        continue;
                    }
                }
                if (auto exprs = dynamic_cast<ExprStmt*>(c.get())) {
                    // drop pure literal expr statements
                    if (dynamic_cast<LiteralExpr*>(exprs->expr.get())) {
                        continue;
                    }
                }
                kept.push_back(std::move(c));
            }
            block.children = std::move(kept);
        }

        void countUsagesExpr(Expr* e) {
            if (!e) return;
            if (auto id = dynamic_cast<IdentifierExpr*>(e)) {
                use_count[id->values]++;
                return;
            }
            if (auto bin = dynamic_cast<BinOp*>(e)) {
                countUsagesExpr(bin->lhs.get());
                countUsagesExpr(bin->rhs.get());
                return;
            }
            if (auto call = dynamic_cast<CallExpr*>(e)) {
                if (call->callee) countUsagesExpr(call->callee.get());
                for (auto &a : call->arguments) if (a) countUsagesExpr(a.get());
                return;
            }
            if (auto vec = dynamic_cast<VecExpr*>(e)) {
                for (auto &el : vec->elems) if (el) countUsagesExpr(el.get());
                return;
            }
            if (auto map = dynamic_cast<MapExpr*>(e)) {
                for (auto &k : map->keys) if (k) countUsagesExpr(k.get());
                for (auto &v : map->values) if (v) countUsagesExpr(v.get());
                return;
            }
            if (auto lookup = dynamic_cast<LookupExpr*>(e)) {
                if (lookup->object) countUsagesExpr(lookup->object.get());
                if (lookup->target) countUsagesExpr(lookup->target.get());
                return;
            }
            if (auto un = dynamic_cast<UnOp*>(e)) {
                if (un->values) countUsagesExpr(un->values.get());
                return;
            }
            if (auto addr = dynamic_cast<AddressOf*>(e)) {
                if (addr->target) countUsagesExpr(addr->target.get());
                return;
            }
            if (auto gv = dynamic_cast<GetValuePtr*>(e)) {
                if (gv->addr) countUsagesExpr(gv->addr.get());
                return;
            }
        }

        void countUsagesStmt(Stmt* s) {
            if (!s) return;

            if (auto f = dynamic_cast<FunctionDecl*>(s)) {
                if (f->body) countUsagesStmt(f->body.get());
                return;
            }

            if (auto let = dynamic_cast<LetDecl*>(s)) {
                if (let->values) countUsagesExpr(let->values.get());
                return;
            }

            if (auto assign = dynamic_cast<AssignDecl*>(s)) {
                if (assign->target) countUsagesExpr(assign->target.get());
                if (assign->values) countUsagesExpr(assign->values.get());
                return;
            }

            if (auto branch = dynamic_cast<BranchStmt*>(s)) {
                if (branch->conditions) countUsagesExpr(branch->conditions.get());
                if (branch->body_if) countUsagesStmt(branch->body_if.get());
                if (branch->body_else) countUsagesStmt(branch->body_else.get());
                return;
            }

            if (auto exprs = dynamic_cast<ExprStmt*>(s)) {
                if (exprs->expr) countUsagesExpr(exprs->expr.get());
                return;
            }

            if (auto forl = dynamic_cast<ForLoopStmt*>(s)) {
                if (forl->initializer) countUsagesStmt(forl->initializer.get());
                if (forl->iterator) countUsagesStmt(forl->iterator.get());
                if (forl->body) countUsagesStmt(forl->body.get());
                return;
            }

            if (auto block = dynamic_cast<BlockStmt*>(s)) {
                for (auto &c : block->children) if (c) countUsagesStmt(c.get());
                return;
            }

            if (auto ret = dynamic_cast<ReturnStmt*>(s)) {
                if (ret->values) countUsagesExpr(ret->values.get());
                return;
            }
        }

        // optimize expression owned by unique_ptr (allows replacing node)
        void optimizeExprPtr(std::unique_ptr<Expr> &e) {
            if (!e) return;

            if (auto bin = dynamic_cast<BinOp*>(e.get())) {
                optimizeExprPtr(bin->lhs);
                optimizeExprPtr(bin->rhs);
                // constant fold if both sides are literals
                if (bin->lhs && bin->rhs) {
                    auto L = dynamic_cast<LiteralExpr*>(bin->lhs.get());
                    auto R = dynamic_cast<LiteralExpr*>(bin->rhs.get());
                    if (L && R) {
                        // try to evaluate as integer or float or string
                        try {
                            if (L->kinds == LiteralKind::Digits && R->kinds == LiteralKind::Digits) {
                                long long a = std::stoll(L->values);
                                long long b = std::stoll(R->values);
                                long long out = 0;
                                if (bin->op == "+") out = a + b;
                                else if (bin->op == "-") out = a - b;
                                else if (bin->op == "*") out = a * b;
                                else if (bin->op == "/") { if (b!=0) out = a / b; else throw std::runtime_error("div0"); }
                                else goto no_int;
                                e = std::make_unique<LiteralExpr>(LiteralKind::Digits, std::to_string(out));
                                return;
                            }
                            no_int:;
                            if ((L->kinds == LiteralKind::Digits || L->kinds == LiteralKind::ScientificNotation) && (R->kinds == LiteralKind::Digits || R->kinds == LiteralKind::ScientificNotation)) {
                                double a = std::stod(L->values);
                                double b = std::stod(R->values);
                                double out = 0.0;
                                if (bin->op == "+") out = a + b;
                                else if (bin->op == "-") out = a - b;
                                else if (bin->op == "*") out = a * b;
                                else if (bin->op == "/") { if (b!=0.0) out = a / b; else throw std::runtime_error("div0"); }
                                else goto no_float;
                                e = std::make_unique<LiteralExpr>(LiteralKind::ScientificNotation, std::to_string(out));
                                return;
                            }
                            no_float:;
                            if (L->kinds == LiteralKind::String && R->kinds == LiteralKind::String && bin->op == "+") {
                                std::string out = L->values + R->values;
                                e = std::make_unique<LiteralExpr>(LiteralKind::String, out);
                                return;
                            }
                        } catch (...) {
                            // ignore fold errors
                        }
                    }
                }
                return;
            }

            if (auto call = dynamic_cast<CallExpr*>(e.get())) {
                if (call->callee) optimizeExprPtr(call->callee);
                for (auto &a : call->arguments) if (a) optimizeExprPtr(a);
                return;
            }

            if (auto vec = dynamic_cast<VecExpr*>(e.get())) {
                for (auto &el : vec->elems) if (el) optimizeExprPtr(el);
                return;
            }

            if (auto map = dynamic_cast<MapExpr*>(e.get())) {
                for (auto &k : map->keys) if (k) optimizeExprPtr(k);
                for (auto &v : map->values) if (v) optimizeExprPtr(v);
                return;
            }

            if (auto lookup = dynamic_cast<LookupExpr*>(e.get())) {
                if (lookup->object) optimizeExprPtr(lookup->object);
                if (lookup->target) optimizeExprPtr(lookup->target);
                return;
            }

            if (auto un = dynamic_cast<UnOp*>(e.get())) {
                if (un->values) optimizeExprPtr(un->values);
                return;
            }

            if (auto addr = dynamic_cast<AddressOf*>(e.get())) {
                if (addr->target) optimizeExprPtr(addr->target);
                return;
            }

            if (auto gv = dynamic_cast<GetValuePtr*>(e.get())) {
                if (gv->addr) optimizeExprPtr(gv->addr);
                return;
            }
        }

        void optimizeStmtPtr(std::unique_ptr<Stmt> &s) {
            if (!s) return;

            if (auto f = dynamic_cast<FunctionDecl*>(s.get())) {
                if (f->body) optimizeStmtPtr(f->body);
                return;
            }

            if (auto let = dynamic_cast<LetDecl*>(s.get())) {
                if (let->values) optimizeExprPtr(let->values);
                return;
            }

            if (auto assign = dynamic_cast<AssignDecl*>(s.get())) {
                if (assign->target) optimizeExprPtr(assign->target);
                if (assign->values) optimizeExprPtr(assign->values);
                return;
            }

            if (auto branch = dynamic_cast<BranchStmt*>(s.get())) {
                if (branch->conditions) optimizeExprPtr(branch->conditions);
                if (branch->body_if) optimizeStmtPtr(branch->body_if);
                if (branch->body_else) optimizeStmtPtr(branch->body_else);
                return;
            }

            if (auto exprs = dynamic_cast<ExprStmt*>(s.get())) {
                if (exprs->expr) optimizeExprPtr(exprs->expr);
                return;
            }

            if (auto forl = dynamic_cast<ForLoopStmt*>(s.get())) {
                if (forl->initializer) optimizeStmtPtr(forl->initializer);
                if (forl->iterator) optimizeStmtPtr(forl->iterator);
                if (forl->body) optimizeStmtPtr(forl->body);
                return;
            }

            if (auto block = dynamic_cast<BlockStmt*>(s.get())) {
                // optimize nested block
                optimizeBlock(*block);
                return;
            }

            if (auto ret = dynamic_cast<ReturnStmt*>(s.get())) {
                if (ret->values) optimizeExprPtr(ret->values);
                return;
            }
        }
    };
};
