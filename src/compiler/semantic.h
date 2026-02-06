#pragma once

// c++ library
#include <string>

// local headers
#include "ast.h"
#include "diagnostics.h"
#include "symbol.h"

namespace sonic::frontend {
  namespace ast {
    struct Program;
    struct Statement;
  }

  class SemanticAnalyzer {
  public:
    Symbol* symbols;
    Symbol* groups;
    Symbol* entrySymbol = nullptr;

    ScopeLevel scopeLevel = ScopeLevel::GLOBAL;
    size_t offset = 0;
    size_t depth = 0;
    std::string filename = "";
    std::string filepath = "";

    DiagnosticEngine* diag;

    SemanticAnalyzer(Symbol* sym);

    void analyze(ast::Program* stmt);

    void eager_analyze(ast::Statement* st);
    void analyze_statement(ast::Statement* st);
    void analyze_expression(ast::Expression* ex);
    void analyze_type(ast::Type* ty);

    Symbol* lookup_type(ast::Type* ty);
    bool match_type(ast::Type* l, ast::Type* r);

    // Helper methods for flexible module resolution
  private:
    enum class ModuleSource {
      LOCAL,      // Local directory of current file
      PROJECT,    // Root project directory (where main.sn is)
      EXTERNAL    // External library
    };

    struct ModuleResolution {
      std::string path;
      ModuleSource source;
      bool isDirectory;
    };

    std::string getExternalLibPath();
    ModuleResolution resolveModulePath(const std::vector<std::unique_ptr<ast::Statement>>& qualified);
    std::unique_ptr<ast::Program> loadAndAnalyzeModule(const std::string& modulePath);
    void loadDirectoryAsNamespace(const std::string& dirPath, Symbol* parentSymbol);
  };
};
