#pragma once

#include <string>
#include "parser.h"
#include "lexer.h"
#include "ast/stmt.h"
#include "ast/type.h"
#include "ast/expr.h"
#include "diagnostics.h"
#include "symbol.h"
#include "../core/io.h"
#include "../core/config.h"
#include "operand.h"
#include <cassert>

using namespace std;

namespace sonic::frontend {

  class SemanticAnalyzer {
    public:
      SemanticAnalyzer() {
        universe = new Symbol();
        universe->kind = SymbolKind::NAMESPACE;
        universe->scopeLevel = GLOBAL_SCOPE;
        universe->table = new SymbolTable();
      }

      SemanticAnalyzer(Symbol* symbols) {
        universe = symbols;
      }

      ~SemanticAnalyzer() = default;

      ScopeLevel scopeLevel = GLOBAL_SCOPE;

      Symbol* universe = nullptr;
      Symbol* symbol = nullptr;
      DiagnosticEngine* diag = nullptr;


      void lazy_analyze(Stmt* stmt);
      void analyze(Stmt* stmt);
      void analyze_stmt(Stmt* stmt);
      void analyze_expr(Expr* expr);
      void analyze_type(Type* type);
      std::string resolveTypeToString(Type* type);
      Type* resolveBinaryExpr(Type* left, BinaryOp op, Type* right);
  };
}
