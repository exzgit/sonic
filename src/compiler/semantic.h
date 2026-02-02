#pragma once

// c++ library
#include <memory>
#include <string>

// local headers
#include "ast.h"
#include "diagnostics.h"
#include "symbol.h"

namespace sonic::frontend {

  class SemanticAnalyzer {
  public:
    Symbol* symbols;
    ScopeLevel scopeLevel = ScopeLevel::GLOBAL;
    size_t offset = 0;
    size_t depth = 0;
    std::string filename = "";

    DiagnosticEngine* diag;

    SemanticAnalyzer(Symbol* sym);

    void analyze(std::unique_ptr<SonicStmt> stmt);
    void eager_analyze(SonicStmt* stmt);
    void analyze_statement(SonicStmt* stmt);
    void analyze_expression(SonicExpr* expr);
    void analyze_types(SonicType* type);

    // helper
    Symbol* create_fn(const std::string& name);
    Symbol* create_var(const std::string& name);
    TypeKind literalKindToTypeKind(ExprKind kind);
  };
};
