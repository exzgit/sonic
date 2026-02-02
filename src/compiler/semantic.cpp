// c++ library
#include <vector>
#include <string>
#include <algorithm>
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

  void SemanticAnalyzer::analyze(std::unique_ptr<SonicStmt> stmt) {
    if (!symbols) return;
    eager_analyze(stmt.get());
    analyze_statement(stmt.get());
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

        for (auto& child : stmt->body) {
          eager_analyze(child.get());
        }

        // declare current symbols -> temp & return symbols -> previous value
        temp->declare(progSym);
        symbols = temp;

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
        symbols->declare(funcSym);

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

        analyze_types(stmt->returnType.get());
        funcSym->type_ = stmt->returnType.get();
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

        auto temp = std::move(symbols);
        symbols = progSym;

        for (auto& child : stmt->body) {
          analyze_statement(child.get());
        }

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

        auto temp = std::move(symbols);
        symbols = funcSym;

        for (auto& child : stmt->body) {
          analyze_statement(child.get());
        }

        symbols = temp;
        break;
      }
      case StmtKind::VAR_DECL: {
        if (symbols->existingLocal(stmt->name)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->location,
            "variable '" + stmt->name + "' already declared in this scope"
          });
          break;
        }

        print("statement: analyze variable declaration '" + stmt->name + "'");

        auto varSym = create_var(stmt->name);
        varSym->mutability_ = stmt->mutability;
        varSym->public_ = stmt->isPublic;
        varSym->extern_ = stmt->isExtern;
        varSym->location = stmt->location;

        if (stmt->dataType) {
          if (stmt->dataType->kind == TypeKind::VOID) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->location,
              "variables must not be of type void"
            });
            break;
          }

          analyze_types(stmt->dataType.get());
          varSym->type_ = stmt->dataType.get();
        }

        // analyze initializer if present
        if (stmt->expr) {
          analyze_expression(stmt->expr.get());
        } else {
          // no initializer: declare variable with provided type (if any) and return
          symbols->declare(varSym);
          break;
        }

        // at this point stmt->expr exists; guard resolvedType checks
        if (!stmt->expr->resolvedType) {
          // allow UNTYPED_LITERAL and NONE to remain resolvable later; otherwise report
          if (stmt->expr->kind != ExprKind::UNTYPED_LITERAL && stmt->expr->kind != ExprKind::NONE) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->expr->location,
              "unable to determine type of initializer"
            });
            break;
          }
        }

        if (stmt->expr->resolvedType && stmt->expr->resolvedType->kind == TypeKind::VOID) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->expr->location,
            "cannot assign with void type"
          });
          break;
        }

        // declare early to prevent redefinition within initializer analysis
        symbols->declare(varSym);

        if (stmt->dataType) {
          if (stmt->expr->kind == ExprKind::UNTYPED_LITERAL) {
            SonicType* target = stmt->dataType.get();

            if (target->isInteger()) {
              // existing AST-level check
              if (!stmt->expr->fitsIntegerLiteral(target->bitWidth())) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->expr->location,
                  "integer literal overflow" 
                });
              }

              // use SonicExpr helper to test literal fit (avoids duplicating parsing logic here)
              std::string lit = stmt->expr->literal_text();
              if (lit.empty()) {
                // fallback: if earlier fitsIntegerLiteral already set resolvedType to target, declare
                if (stmt->expr->resolvedType == target) {
                  stmt->expr->resolvedType = target;
                  varSym->type_ = target;
                  // already declared above
                }
              } else {
                unsigned int bits = target->bitWidth();
                if (bits == 0) bits = 32; // fallback
                if (stmt->expr->fits_signed_integer_bits(bits)) {
                  stmt->expr->resolvedType = target;
                  varSym->type_ = target;
                } else {
                  diag->report({
                    ErrorType::SEMANTIC,
                    Severity::ERROR,
                    stmt->expr->location,
                    "integer literal overflow"
                  });
                }
              }
            }
            else if (target->isFloat()) {
              stmt->expr->resolvedType = target;
              varSym->type_ = target;
            }
            else {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                stmt->expr->location,
                "literal cannot be assigned to this type"
              });
            }
          }
          else {
            // both sides should have resolved types (not untyped literal)
            auto src = stmt->expr->resolvedType;
            auto tgt = stmt->dataType.get();

            if (!src) {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                stmt->expr->location,
                "unable to determine type of initializer"
              });
              break;
            }

            if (src->kind == tgt->kind) {
              // exact same kind
              varSym->type_ = tgt;
            }
            else if ((src->isInteger() || src->isFloat()) && (tgt->isInteger() || tgt->isFloat())) {
              // numeric conversion: allow narrowing (src.bitWidth >= tgt.bitWidth), disallow widening
              if (src->bitWidth() >= tgt->bitWidth()) {
                varSym->type_ = tgt;
              } else {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->expr->location,
                  "widening conversion requires explicit cast"
                });
              }
            }
            else {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                stmt->expr->location,
                "type mismatch"
              });
            }
          }
        } else {
          // no explicit dataType: infer from initializer
          if (stmt->expr->kind == ExprKind::NONE) {
            varSym->type_ = new SonicType();
            varSym->type_->kind = TypeKind::POINTER;
            varSym->type_->isNullable = true;
            // already declared above
            break;
          }

          if (!stmt->expr->resolvedType) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->expr->location,
              "unable to determine type of initializer"
            });
            break;
          }

          stmt->dataType = stmt->expr->resolvedType->clone();
          varSym->type_ = stmt->dataType.get();
        }
      }
      default: {
        print("unhandled statement kind in semantic analyzer '" + stmtkind_to_string(stmt->kind) + "'");
        break;
      }
    }
  }

  void SemanticAnalyzer::analyze_expression(SonicExpr* expr) {
    if (!expr) return;

    switch (expr->kind) {
      case ExprKind::UNTYPED_LITERAL: {
        // set a reasonable default for untyped integer literal (inference may override)
        expr->resolvedType = new SonicType();
        expr->resolvedType->kind = TypeKind::I64;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::STRING: {
        expr->resolvedType = new SonicType();
        expr->resolvedType->kind = TypeKind::STRING;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::CHAR: {
        expr->resolvedType = new SonicType();
        expr->resolvedType->kind = TypeKind::CHAR;
        expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::BOOL: {
        expr->resolvedType = new SonicType();
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

        expr->resolvedType = sym->type_;
        if (expr->resolvedType) expr->resolvedType->location = expr->location;
        break;
      }
      case ExprKind::SCOPE_ACCESS: {
        // ...existing code or future implementation...
        break;
      }
      case ExprKind::MEMBER_ACCESS: {
        // ...existing code or future implementation...
        break;
      }
      case ExprKind::CALL: {
        if (expr->callee->kind == ExprKind::IDENT) {
          auto sym = symbols->lookup(expr->callee->name);
          if (!sym || (sym && sym->kind != SymbolKind::FUNCTION)) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->location,
              "function '" + expr->callee->name + "' is undefined"
            });
            break;
          }

          if (!expr->genericTypes.empty()) {
            for (auto& gen : expr->genericTypes) {
              analyze_types(gen.get());
            }
          }

          size_t expected = sym->variadic_ ? expr->arguments.size() : sym->params_.size();
          for (size_t i = 0; i < expected; ++i) {
            if (i >= expr->arguments.size()) break;
            analyze_expression(expr->arguments[i].get());

            auto argType = expr->arguments[i]->resolvedType;
            if (!argType) {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                expr->arguments[i]->location,
                "unable to determine type of argument"
              });
              continue;
            }

            if (sym->variadic_ && i >= sym->params_.size()) {
              if (argType->kind != sym->params_.back()->kind) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  expr->location,
                  "type mismatch at function arguments"
                });
              }
            } else {
              if (i < sym->params_.size() && argType->kind != sym->params_[i]->kind) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  expr->location,
                  "type mismatch at function arguments"
                });
              }
            }
          }

          expr->resolvedType = sym->type_;
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
