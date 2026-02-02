// c++ library
#include <vector>
#include <string>
// #include <unordered_map>
#include <memory>

// local headers
#include "../core/debugging.h"
#include "diagnostics.h"
#include "semantic.h"
#include "startup.h"
#include "symbol.h"
#include "ast.h"

using namespace sonic::debug;

namespace sonic::frontend {

  SemanticAnalyzer::SemanticAnalyzer(Symbol* sym)
      : symbols(sym) {}

  void SemanticAnalyzer::analyze(SonicStmt* stmt) {
    if (!symbols) return;
    eager_analyze(stmt);
    analyze_statement(stmt);
  }

  void SemanticAnalyzer::eager_analyze(SonicStmt* stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
      case StmtKind::PROGRAM: {

        // hentikan analisis jika program sudah di analisis sebelumnya
        if (symbols->exists(stmt->name))
          break;

        print("eager: analyze program '" + stmt->name + "'");
        filename = stmt->name;

        // create program symbols
        auto progSym = new Symbol();
        progSym->kind = SymbolKind::NAMESPACE;
        progSym->name_ = stmt->name;
        progSym->scopeLevel = scopeLevel;
        progSym->depth = depth;
        progSym->offset = offset;

        // move current symbols -> temp & move program symbols -> symbols
        auto temp = symbols;
        symbols = progSym;


        auto currentScope = scopeLevel;
        scopeLevel = ScopeLevel::GLOBAL;

        for (auto& child : stmt->body) {
          eager_analyze(child.get());
        }

        scopeLevel = currentScope;

        // declare current symbols -> temp & return symbols -> previous value
        symbols = temp;
        symbols->declare(progSym);

        break;
      }
      case StmtKind::FUNC_DECL: {
        if (symbols->lookup_local(stmt->name)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->location,
            "function '" + stmt->name + "' already '" + (stmt->isExtern && stmt->body.empty() ? "declared'" : "defined'")
          });
          break;
        }

        print("eager: analyze function '" + stmt->name + "'");
        auto funcSym = create_fn(stmt->name);

        if (stmt->returnType) {
          analyze_types(stmt->returnType.get());
          funcSym->type_ = stmt->returnType.get();
        } else {
          stmt->returnType = std::make_unique<SonicType>();
          stmt->returnType->kind = TypeKind::VOID;
          funcSym->type_ = stmt->returnType.get();
        }

        for (auto& param : stmt->parameters) {
          if (funcSym->exists(param->name)) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->location,
              "parameter name '" + param->name + "' already exists"
            });
            continue;
          } else {
            funcSym->variadic_ = param->variadic;
            analyze_types(param->type.get());
            funcSym->params_.push_back(param->type.get());

            if (funcSym->variadic_) break;
          }
        }
        symbols->declare(funcSym);

        break;
      }
      default: break;
    }
  }

  void SemanticAnalyzer::analyze_statement(SonicStmt* stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
      case StmtKind::PROGRAM: {
        auto progSym = symbols->lookup(stmt->name);
        if (!progSym) break;

        auto temp = symbols;
        symbols = progSym;

        auto currentScope = scopeLevel;
        scopeLevel = ScopeLevel::GLOBAL;

        for (auto& child : stmt->body) {
          analyze_statement(child.get());
        }

        scopeLevel = currentScope;
        symbols = temp;
        break;
      }
      case StmtKind::FUNC_DECL: {
        if (scopeLevel == ScopeLevel::FUNC) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->location,
            "nested function definitions are not allowed"
          });
          break;
        }

        print("statement: analyze function '" + stmt->name + "'");

        if (symbols->scopeLevel == ScopeLevel::GLOBAL) {
          offset = 0;
          depth = 0;
        }

        auto funcSym = symbols->lookup_local(stmt->name);
        if (!funcSym) break;

        auto temp = symbols;
        symbols = funcSym;

        scopeLevel = ScopeLevel::FUNC;

        for (auto& child : stmt->body) {
          analyze_statement(child.get());
        }

        scopeLevel = ScopeLevel::GLOBAL;

        symbols = temp;
        break;
      }
      case StmtKind::VAR_DECL:
        analyze_variable_declaration(stmt);
        break;
      default: {
        print("unhandled statement kind in semantic analyzer '" + stmtkind_to_string(stmt->kind) + "'");
        break;
      }
    }
  }

  void SemanticAnalyzer::analyze_variable_declaration(SonicStmt* stmt) {
    if (!stmt) return;

    if (scopeLevel == ScopeLevel::FUNC) {
      bool isErr = false;

      if (stmt->isExtern) {
        diag->report({
          ErrorType::SEMANTIC,
          Severity::ERROR,
          stmt->location,
          "local variables cannot be extern"
        });
        isErr = true;
      }

      if (stmt->mutability == Mutability::STATIC || stmt->mutability == Mutability::CONSTANT) {
        diag->report({
          ErrorType::SEMANTIC,
          Severity::ERROR,
          stmt->location,
          "local variables cannot be " + mutability_to_string(stmt->mutability)
        });
        isErr = true;
      }

      if (stmt->isPublic) {
        diag->report({
          ErrorType::SEMANTIC,
          Severity::ERROR,
          stmt->location,
          "local variables cannot be public"
        });
        isErr = true;
      }

      if (isErr) return;
    }

    auto varSym = create_var(stmt->name);
    varSym->extern_ = stmt->isExtern;
    varSym->public_ = stmt->isPublic;
    varSym->mutability_ = stmt->mutability;

    if (stmt->dataType) {
      if (stmt->dataType->kind == TypeKind::VOID) {
        diag->report({
          ErrorType::SEMANTIC,
          Severity::ERROR,
          stmt->location,
          "variable cannot be void"
        });
        return;
      }

      analyze_types(stmt->dataType.get());
      varSym->type_ = stmt->dataType.get();
    }

    if (!stmt->expr) {
      symbols->declare(varSym);
      return;
    }

    analyze_expression(stmt->expr.get());

    if (stmt->expr->resolvedType && stmt->expr->resolvedType->kind == TypeKind::VOID) {
      diag->report({
        ErrorType::SEMANTIC,
        Severity::ERROR,
        stmt->location,
        "variable cannot be assigned with void"
      });
      return;
    }

    if (stmt->dataType) {
      bool isNullable = stmt->dataType->isNullable;

      if (stmt->dataType->kind == TypeKind::AUTO) {
        stmt->dataType = stmt->expr->resolvedType->clone();

        if (stmt->expr->resolvedType->kind == TypeKind::UNRESOLVED) {
          if (stmt->expr->kind == ExprKind::INT) {
            if (stmt->expr->inferIntegerBitWidth() == 32) stmt->dataType->kind = TypeKind::I64;
            else if (stmt->expr->inferIntegerBitWidth() == 64) stmt->dataType->kind = TypeKind::I64;
            else if (stmt->expr->inferIntegerBitWidth() == 128) stmt->dataType->kind = TypeKind::I128;
          } else if (stmt->expr->kind == ExprKind::FLOAT) {
            stmt->dataType->kind = TypeKind::F64;
          }

          stmt->expr->resolvedType = stmt->dataType->clone();
        }

        stmt->dataType->isNullable = isNullable;
        varSym->type_ = stmt->dataType.get();
        symbols->declare(varSym);
        return;
      } else {
        if (!stmt->expr->resolvedType)
          return;

        if (stmt->dataType->kind == TypeKind::POINTER && stmt->expr->kind != ExprKind::REFERENCE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->expr->resolvedType->location,
            "expected pointer address"
          });
          return;
        }
        else if (stmt->dataType->kind == TypeKind::REFERENCE && stmt->expr->kind != ExprKind::DEREFERENCE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->expr->location,
            "expected object reference"
          });
          return;
        }

        if (stmt->dataType->isInteger() && stmt->expr->resolvedType->kind == TypeKind::UNRESOLVED) {
          if (stmt->expr->kind == ExprKind::INT) {
            if (stmt->expr->inferIntegerBitWidth() <= stmt->dataType->bitWidth()) stmt->expr->resolvedType = stmt->dataType->clone();
            else {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                stmt->expr->location,
                "cannot infer literal"
              });
            }
          } else if (stmt->expr->kind == ExprKind::FLOAT) {
            stmt->expr->resolvedType->kind = TypeKind::F64;
          }

          symbols->declare(varSym);
          return;
        }


        if (stmt->dataType->isInteger() && stmt->expr->resolvedType->isInteger()) {
          if (stmt->dataType->bitWidth() != stmt->expr->resolvedType->bitWidth()) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->expr->location,
              "variable type mismatch"
            });
            return;
          }

          symbols->declare(varSym);
        }
      }
    }

  }

  void SemanticAnalyzer::analyze_expression(SonicExpr* expr) {
    if (!expr) return;

    switch (expr->kind) {
      case ExprKind::FLOAT:
      case ExprKind::INT: {
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::UNRESOLVED;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::STRING: {
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::STRING;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::CHAR: {
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::CHAR;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::BOOL: {
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::BOOL;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::IDENT: {
        auto sym = symbols->lookup(expr->name);

        if (!sym) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->location,
            "variable is undefined"
          });
          break;
        }

        if (sym->kind == SymbolKind::FUNCTION || sym->kind == SymbolKind::NAMESPACE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->location,
            "variable is function"
          });
          break;
        }

        expr->resolvedType = sym->type_->clone();
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::REFERENCE: {
        if (!expr->nested) return;
        if (expr->nested->kind != ExprKind::IDENT) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->nested->location,
            "expected variable after '&' reference"
          });
        }
        analyze_expression(expr->nested.get());
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::REFERENCE;
        expr->resolvedType->elementType = expr->nested->resolvedType->clone();
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::DEREFERENCE: {
        if (!expr->nested) return;
        if (expr->nested->kind != ExprKind::IDENT) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->nested->location,
            "expected variable after '*' dereference"
          });
        }
        analyze_expression(expr->nested.get());
        expr->resolvedType = std::make_unique<SonicType>();
        expr->resolvedType->kind = TypeKind::POINTER;
        expr->resolvedType->elementType = expr->nested->resolvedType->clone();
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::SCOPE_ACCESS: {

      }
      case ExprKind::MEMBER_ACCESS: {

      }
      case ExprKind::CALL: {
        if (expr->callee->kind == ExprKind::IDENT) {
          auto sym = symbols->lookup(expr->callee->name);
          if (!sym || (sym && sym->kind != SymbolKind::FUNCTION)) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->callee->location,
              "function '" + expr->callee->name + "' is undefined"
            });
            break;
          }

          if (!expr->genericTypes.empty()) {
            for (auto& gen : expr->genericTypes) {
              analyze_types(gen.get());
            }
          }

          for (int i = 0; i < (sym->variadic_ ? expr->arguments.size() : sym->params_.size()); i++) {
            analyze_expression(expr->arguments[i].get());
            if (sym->variadic_ && i >= sym->params_.size()) {
              if (expr->arguments[i]->resolvedType->kind != sym->params_.back()->kind) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  expr->location,
                  "type mismatch at function arguments"
                });
              }
            } else {
              if (expr->arguments[i]->resolvedType->kind != sym->params_[i]->kind) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  expr->location,
                  "type mismatch at function arguments"
                });
              }
            }
          }

          expr->resolvedType = sym->type_->clone();
        } else {
          analyze_expression(expr->callee.get());
        }
        break;
      }
      default: break;
    }
  }

  void SemanticAnalyzer::analyze_types(SonicType* type) {
    //
  }

  // helper definitions
  Symbol* SemanticAnalyzer::create_fn(const std::string& name) {
    auto sym = new Symbol();
    sym->kind = SymbolKind::FUNCTION;
    sym->scopeLevel = scopeLevel;
    sym->name_ = name;
    sym->parent_ = symbols;
    sym->mangleName_ = startup::pathToNamespace(symbols->name_) + "@" + name;
    sym->offset = offset;
    sym->depth = depth;

    return sym;
  }

  Symbol* SemanticAnalyzer::create_var(const std::string& name) {
    auto sym = new Symbol();
    sym->kind = SymbolKind::VARIABLE;
    sym->scopeLevel = scopeLevel;
    sym->name_ = name;
    sym->parent_ = symbols;
    if (symbols->kind == SymbolKind::NAMESPACE) {
      sym->mangleName_ = startup::pathToNamespace(filename) + "@" + name;
    } else {
      sym->mangleName_ = startup::pathToNamespace(filename) + "@" + symbols->name_ + "_" + name;
    }
    sym->offset = offset;
    sym->depth = depth;

    return sym;
  }

};
