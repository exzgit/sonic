#pragma once

#include "ast/visitor.h"
#include "symboltable.h"
#include "ast/ast_fwd.h"
#include "ast/type.h"
#include "ast/value.h"
#include <optional>
#include <algorithm>

namespace frontend {
    class SemanticAnalyzer : public Visitor {
    public:
        SemanticAnalyzer() {
            symbolTable = SymbolTable();
        }
        ~SemanticAnalyzer() = default;

    private:
        SymbolTable symbolTable;

        // resolve AST `Type` -> SymbolTable::TypeInfo
        frontend::TypeInfo resolveType(frontend::Type* t) {
            using namespace frontend;
            if (!t) return TypeInfo();
            // try dynamic casts
            if (dynamic_cast<AnyType*>(t)) {
                return TypeInfo::make_any();
            }
            if (auto stdt = dynamic_cast<StandardType*>(t)) {
                switch (stdt->kinds) {
                    case StandardTypeKind::String_Type: return TypeInfo::make_primitive(PrimitiveType::String);
                    case StandardTypeKind::Char_Type: return TypeInfo::make_primitive(PrimitiveType::Char);
                    case StandardTypeKind::Boolean_Type: return TypeInfo::make_primitive(PrimitiveType::Bool);
                    case StandardTypeKind::I8_Type: return TypeInfo::make_primitive(PrimitiveType::I8);
                    case StandardTypeKind::I16_Type: return TypeInfo::make_primitive(PrimitiveType::I16);
                    case StandardTypeKind::I32_Type: return TypeInfo::make_primitive(PrimitiveType::I32);
                    case StandardTypeKind::I64_Type: return TypeInfo::make_primitive(PrimitiveType::I64);
                    case StandardTypeKind::I128_Type: return TypeInfo::make_primitive(PrimitiveType::I128);
                    case StandardTypeKind::U8_Type: return TypeInfo::make_primitive(PrimitiveType::U8);
                    case StandardTypeKind::U16_Type: return TypeInfo::make_primitive(PrimitiveType::U16);
                    case StandardTypeKind::U32_Type: return TypeInfo::make_primitive(PrimitiveType::U32);
                    case StandardTypeKind::U64_Type: return TypeInfo::make_primitive(PrimitiveType::U64);
                    case StandardTypeKind::U128_Type: return TypeInfo::make_primitive(PrimitiveType::U128);
                    case StandardTypeKind::F32_Type: return TypeInfo::make_primitive(PrimitiveType::F32);
                    case StandardTypeKind::F64_Type: return TypeInfo::make_primitive(PrimitiveType::F64);
                    default: return TypeInfo();
                }
            }
            if (auto q = dynamic_cast<QualifiedType*>(t)) {
                // join segments as a name
                std::string name;
                for (size_t i = 0; i < q->segments.size(); ++i) {
                    if (i) name += "::";
                    name += q->segments[i];
                }
                return TypeInfo::make_named(name);
            }

            // fallback: unknown
            return TypeInfo();
        }

        // resolve expression types conservatively
        frontend::TypeInfo resolveExprType(frontend::Expr* e) {
            using namespace frontend;
            if (!e) return TypeInfo();
            if (auto lit = dynamic_cast<LiteralExpr*>(e)) {
                switch (lit->kinds) {
                    case LiteralKind::String: return TypeInfo::make_primitive(PrimitiveType::String);
                    case LiteralKind::Char: return TypeInfo::make_primitive(PrimitiveType::Char);
                    case LiteralKind::Boolean: return TypeInfo::make_primitive(PrimitiveType::Bool);
                    case LiteralKind::Digits: return TypeInfo::make_primitive(PrimitiveType::I64);
                    case LiteralKind::ScientificNotation: return TypeInfo::make_primitive(PrimitiveType::F64);
                    case LiteralKind::Hex: return TypeInfo::make_primitive(PrimitiveType::I64);
                    default: return TypeInfo();
                }
            }
            if (auto id = dynamic_cast<IdentifierExpr*>(e)) {
                auto sym = symbolTable.lookup(id->values, false);
                if (sym) return (*sym)->type;
                return TypeInfo();
            }

            // for other expressions be conservative (unknown)
            return TypeInfo();
        }

        bool typesEqual(const frontend::TypeInfo &a, const frontend::TypeInfo &b) {
            using namespace frontend;
            if (a.is_any || b.is_any) return true;
            if (a.kind != b.kind) return false;
            if (a.kind == TypeInfo::Kind::Primitive) return a.primitive == b.primitive;
            if (a.kind == TypeInfo::Kind::Named) return a.name == b.name;
            if (a.kind == TypeInfo::Kind::Function) {
                if (!a.return_type || !b.return_type) return false;
                if (!typesEqual(*a.return_type, *b.return_type)) return false;
                if (a.params.size() != b.params.size()) return false;
                for (size_t i = 0; i < a.params.size(); ++i) if (!typesEqual(a.params[i], b.params[i])) return false;
                return true;
            }
            // fallback: compare name and generics
            return a.name == b.name;
        }

        // visitor for expression node
        virtual void visit(LiteralExpr& expr) {}

        virtual void visit(NoneExpr& expr) {}
        
        virtual void visit(IdentifierExpr& expr) {
            auto sym = symbolTable.lookup(expr.values, false);
            if (!sym) {
                std::cerr << "Semantic error: undefined identifier '" << expr.values << "'" << std::endl;
            }
        }
        
        virtual void visit(LookupExpr& expr) {
            if (expr.object) expr.object->accept(*this);
            if (expr.target) expr.target->accept(*this);
        }
        
        virtual void visit(CallExpr& expr) {
            if (expr.callee) expr.callee->accept(*this);
            for (auto &a : expr.arguments) {
                if (a) a->accept(*this);
            }
        }
        
        virtual void visit(VecExpr& expr) {
            for (auto &e : expr.elems) if (e) e->accept(*this);
        }
        
        virtual void visit(MapExpr& expr) {
            for (auto &k : expr.keys) if (k) k->accept(*this);
            for (auto &v : expr.values) if (v) v->accept(*this);
        }
        
        virtual void visit(BinOp& expr) {
            if (expr.lhs) expr.lhs->accept(*this);
            if (expr.rhs) expr.rhs->accept(*this);
        }
        
        virtual void visit(UnOp& expr) {
            if (expr.values) expr.values->accept(*this);
        }
        
        virtual void visit(AddressOf& expr) {
            if (expr.target) expr.target->accept(*this);
        }
        
        virtual void visit(GetValuePtr& expr) {
            if (expr.addr) expr.addr->accept(*this);
        }
        
        virtual void visit(Parameter& expr) {
            SymbolEntry param;
            param.name = expr.name;
            param.kind = SymbolKind::Variable;
            if (!symbolTable.insert(std::make_shared<SymbolEntry>(param))) {
                std::cerr << "Semantic error: parameter '" << expr.name << "' already defined in this scope" << std::endl;
            }
        }
        
        virtual void visit(Range& expr) {
            if (expr.start) expr.start->accept(*this);
            if (expr.end) expr.end->accept(*this);
        }

        // visitor for type
        virtual void visit(AutoType& type) {}
        virtual void visit(VoidType& type) {}
        virtual void visit(AnyType& type) {}
        virtual void visit(StandardType& type) {}
        virtual void visit(GenericType& type) {}
        virtual void visit(QualifiedType& type) {}
        virtual void visit(PointerType& type) {}
        virtual void visit(ReferenceType& type) {}

        // visitor for statement node
        virtual void visit(FunctionDecl& stmt) {
            std::cout << "SemanticAnalyzer: Visiting FunctionDecl " << stmt.name << std::endl;
            SymbolEntry funcSymbol;
            funcSymbol.name = stmt.name;
            funcSymbol.kind = SymbolKind::Function;

            if (!symbolTable.insert(std::make_shared<SymbolEntry>(funcSymbol))) {
                    std::cerr << "Error: Function " << stmt.name << " is already defined in the current scope." << std::endl;
            }

            symbolTable.push_scope(stmt.name);

            // insert parameters into the function scope
            for (auto &p : stmt.params) {
                if (p) p->accept(*this);
            }

            // visit body
            if (stmt.body) stmt.body->accept(*this);

            symbolTable.pop_scope();
        }

        virtual void visit(LetDecl& stmt) {
            if (stmt.values) stmt.values->accept(*this);

            frontend::TypeInfo declaredType;
            if (stmt.types) declaredType = resolveType(stmt.types.get());

            frontend::TypeInfo valueType;
            if (stmt.values) valueType = resolveExprType(dynamic_cast<frontend::Expr*>(stmt.values.get()));

            if (!declaredType.is_any && stmt.types && !(declaredType.kind==frontend::TypeInfo::Kind::Unknown) && !(valueType.kind==frontend::TypeInfo::Kind::Unknown)) {
                if (!typesEqual(declaredType, valueType)) {
                    std::cerr << "Semantic error: type mismatch in declaration '" << stmt.name << "'" << std::endl;
                }
            }

            SymbolEntry entry;
            entry.name = stmt.name;
            entry.kind = SymbolKind::Variable;
            // set symbol type: prefer declared, otherwise inferred from value
            if (stmt.types) entry.type = declaredType;
            else entry.type = valueType;

            if (!symbolTable.insert(std::make_shared<SymbolEntry>(entry))) {
                    std::cerr << "Semantic error: variable '" << stmt.name << "' already defined in this scope" << std::endl;
            }
        }

        virtual void visit(AssignDecl& stmt) {
            if (stmt.target) stmt.target->accept(*this);
            if (stmt.values) stmt.values->accept(*this);

            // handle simple identifier targets
            if (stmt.target) {
                if (auto id = dynamic_cast<frontend::IdentifierExpr*>(stmt.target.get())) {
                    auto sym = symbolTable.lookup(id->values, false);
                    frontend::TypeInfo targetType;
                    if (sym) targetType = (*sym)->type;

                    frontend::TypeInfo valueType = resolveExprType(stmt.values.get());

                    if (!(targetType.is_any) && !(targetType.kind==frontend::TypeInfo::Kind::Unknown) && !(valueType.kind==frontend::TypeInfo::Kind::Unknown)) {
                        if (!typesEqual(targetType, valueType)) {
                            std::cerr << "Semantic error: type mismatch in assignment to '" << id->values << "'" << std::endl;
                        }
                    }
                }
            }
        }

        virtual void visit(BranchStmt& stmt) {
            if (stmt.conditions) stmt.conditions->accept(*this);
            if (stmt.body_if) stmt.body_if->accept(*this);
            if (stmt.body_else) stmt.body_else->accept(*this);
        }

        virtual void visit(ExprStmt& stmt) {
            if (stmt.expr) stmt.expr->accept(*this);
        }

        virtual void visit(ForLoopStmt& stmt) {
            if (stmt.initializer) stmt.initializer->accept(*this);
            if (stmt.iterator) stmt.iterator->accept(*this);
            if (stmt.body) stmt.body->accept(*this);
        }

        virtual void visit(BlockStmt& stmt) {
            symbolTable.push_scope("block");
            for (auto &c : stmt.children) if (c) c->accept(*this);
            symbolTable.pop_scope();
        }

        virtual void visit(ReturnStmt& stmt) {
            if (stmt.values) stmt.values->accept(*this);
        }
    };
};
