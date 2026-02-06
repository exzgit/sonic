// c++ library
#include <vector>
#include <string>
#include <filesystem>
// #include <unordered_map>
#include <memory>

// local headers
#include "../core/debugging.h"
#include "ast.h"
#include "diagnostics.h"
#include "semantic.h"
#include "io.h"
#include "lexer.h"
#include "manager.h"
#include "parser.h"
#include "ast_io.h"
#include "symbol.h"
#include "../core/config.h"
#include "codegen.h"

using namespace sonic::debug;
using namespace sonic::backend;

namespace sonic::frontend {
  using namespace ast;
  using namespace ast::io;

  SemanticAnalyzer::SemanticAnalyzer(Symbol* sym)
  : symbols(sym), groups(sym)
  {}

  void SemanticAnalyzer::analyze(Program* pg) {
    if (!pg) return;

    if (groups->exists(sonic::io::getFileNameWithoutExt(pg->name_))) return;

    auto program = new Symbol();
    program->kind_ = SymbolKind::NAMESPACE;
    program->name_ = sonic::io::getFileNameWithoutExt(pg->name_);
    program->mangle_ = "sn_" + pg->name_;

    symbols->declare(program);

    symbols = program;
    for (auto& st : pg->statements_) eager_analyze(st.get());
    symbols = program;
    for (auto& st : pg->statements_) analyze_statement(st.get());
    symbols = groups;

    // save AST and Symbol info to cache
    sonic::io::create_folder(sonic::io::getFullPath(config::project_build + "/"));
    sonic::io::create_folder(sonic::io::getFullPath(config::project_build + "/cache/"));
    sonic::frontend::ast::io::saveProgramToFile(*pg, sonic::io::getFullPath(config::project_build + "/cache/" + sonic::io::getFileNameWithoutExt(filepath + "/" + program->name_) + ".ast.json"));

    sonic::backend::SonicCodegen codegen(symbols);
    codegen.generate(pg);
  }

  void SemanticAnalyzer::eager_analyze(Statement* st) {
    if (!st) return;

    switch (st->kind_) {
      case StmtKind::FUNCTION: {

        if (symbols->exists(st->name_)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            st->loc_,
            "function already defined"
          });
          break;
        }

        auto function = new Symbol();
        function->kind_ = SymbolKind::FUNCTION;
        function->scope_ = scopeLevel;
        function->name_ = st->name_;
        function->mangle_ = symbols->mangle_ + "_" + st->name_;
        function->variadic_ = st->variadic_;
        function->public_ = st->public_;
        function->extern_ = st->extern_;
        function->async_ = st->async_;
        function->parent_ = symbols;
        function->decl_ = st->declare_;

        if (st->name_ == "main" && entrySymbol) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            st->loc_,
            "function entry point already defined"
          });
          break;
        } else if (st->name_ == "main") {
          entrySymbol = function;
          function->mangle_ = st->name_;
          function->public_ = true;
        }

        st->symbols_ = function;
        symbols->declare(function);

        std::vector<std::string> argName;
        for (auto& arg : st->params_) {
          bool used = false;
          for (auto& a : argName) {
            if (a == arg->name_) {
              used = true;
              break;
            }
          }

          if (used) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              arg->loc_,
              "parameter name already used"
            });
            continue;
          }

          argName.push_back(arg->name_);

          analyze_type(arg->type_.get());
          arg->type_->symbols_ = lookup_type(arg->type_.get());
          function->params_.push_back(arg->type_.get());
        }

        if (st->type_) {
          analyze_type(st->type_.get());
          function->type_ = st->type_.get();
        }

        break;
      }
      default:
        return;
    }
  }

  void SemanticAnalyzer::analyze_statement(Statement* st) {
    if (!st) return;

    switch (st->kind_) {
      case StmtKind::IMPORT: {
        std::unique_ptr<ast::Program> module = nullptr;
        Symbol* moduleNamespace = nullptr;

        // Resolve the module path using flexible search strategy
        ModuleResolution resolution = resolveModulePath(st->import_qualified_);

        if (resolution.path.empty()) {
          // Module not found in any search path
          std::string hintPath = "";
          for (size_t i = 0; i < st->import_qualified_.size(); ++i) {
            if (i > 0) hintPath += "::";
            hintPath += st->import_qualified_[i]->name_;
          }

          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            st->import_qualified_.back()->loc_,
            "module '" + hintPath + "' not found"
          });
          break;
        }

        // Case 1: Import from a specific .sn file
        if (!resolution.isDirectory) {
          module = loadAndAnalyzeModule(resolution.path);

          if (!module) {

            std::string hintPath = "";
            for (size_t i = 0; i < st->import_qualified_.size(); ++i) {
              if (i > 0) hintPath += "::";
              hintPath += st->import_qualified_[i]->name_;
            }

            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              st->import_qualified_.back()->loc_,
              "failed to parse module '" + hintPath + "'"
            });
            break;
          }

          moduleNamespace = symbols->lookup(sonic::io::getFileNameWithoutExt(module->name_));
        }
        // Case 2: Import from a directory (load all .sn files as namespace)
        else {
          // Create a namespace for the directory
          std::string dirName = st->import_qualified_.back()->name_;

          auto dirNamespace = new Symbol();
          dirNamespace->kind_ = SymbolKind::NAMESPACE;
          dirNamespace->name_ = dirName;
          dirNamespace->scope_ = ScopeLevel::GLOBAL;
          dirNamespace->mangle_ = symbols->mangle_ + "_" + dirName;

          symbols->declare(dirNamespace);

          // Load all .sn files and subdirectories in this directory
          loadDirectoryAsNamespace(resolution.path, dirNamespace);

          moduleNamespace = dirNamespace;
        }

        // Handle import items (specific imports)
        if (!st->import_all_ && module) {
          for (auto& c : st->import_items_) {
            bool ok = false;

            for (auto& m : module->statements_) {
              if (c->name_ == m->name_) {
                if (m->public_) {
                  auto alias = new Symbol();
                  alias->name_ = c->import_alias_.empty() ? c->name_ : c->import_alias_;
                  alias->kind_ = SymbolKind::ALIAS;
                  alias->scope_ = ScopeLevel::GLOBAL;
                  alias->ref_ = (Symbol*)m->symbols_;
                  symbols->declare(alias);

                  c->symbols_ = alias;
                } else {
                  diag->report({
                    ErrorType::SEMANTIC,
                    Severity::ERROR,
                    c->loc_,
                    "symbol '" + c->name_ + "' is not public"
                  });
                }
                ok = true;
                break;
              }
            }

            if (!ok) {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                c->loc_,
                "symbol '" + c->name_ + "' not found in module"
              });
            }
          }
        }
        // Handle import all (*) or directory imports
        else if (st->import_all_ && (module || moduleNamespace)) {
          Symbol* source = module ? moduleNamespace : moduleNamespace;

          if (source) {
            for (auto& sym : source->children_) {
              if (sym->public_ || sym->kind_ == SymbolKind::NAMESPACE) {
                auto alias = new Symbol();
                alias->name_ = sym->name_;
                alias->scope_ = ScopeLevel::GLOBAL;
                alias->kind_ = SymbolKind::ALIAS;
                alias->ref_ = sym;
                symbols->declare(alias);
              }
            }
          }
        }

        break;
      }
      case StmtKind::FUNCTION: {
        auto temp = symbols;
        symbols = (Symbol*)st->symbols_;

        if (st->declare_) {
          symbols = temp;
          break;
        }

        for (auto& ch : st->body_)
          analyze_statement(ch.get());

        symbols = temp;
        break;
      }
      case StmtKind::RETURN: {
        analyze_expression(st->value_.get());
        if (st->value_) {
          if (symbols->type_) {
            // return with value
            if (st->value_->type_->isIntegerType()) {
              if (st->value_->bitWidth() <= 64 && st->value_ != 0) {
                st->value_->literal_ = LiteralKind::I64;
              } else if (st->value_->bitWidth() > 64 && st->value_ != 0) {
                st->value_->literal_ = LiteralKind::I128;
              } else {
                // todo -> error
              }
            } else if (st->value_->type_->isFloatType()) {
              if (st->value_->bitWidth() <= 64 && st->value_ != 0) {
                st->value_->literal_ = LiteralKind::F64;
              } else {
                // todo -> error
              }
            }
            else if (match_type(symbols->type_, st->value_->type_)) {
              // type match
            } else {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                st->value_->loc_,
                "return type mismatch"
              });
            }
          } else {
            // function has no return type
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              st->loc_,
              "function has no return type"
            });
          }
        } else {
          if (symbols->type_) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              st->loc_,
              "return value expected"
            });
          }
        }
        break;
      }
      case StmtKind::VARIABLE: {
        if (symbols->exists(st->name_)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            st->loc_,
            "variable already exists"
          });
          break;
        }

        auto variable = new Symbol();
        variable->name_ = st->name_;
        variable->mangle_ = symbols->mangle_ + "_" + st->name_;
        variable->public_ = st->public_;
        variable->extern_ = st->extern_;
        variable->async_ = st->async_;
        variable->mutability_ = st->mutability;
        variable->parent_ = symbols;

        auto dty = lookup_type(st->type_.get());
        analyze_expression(st->value_.get());
        st->symbols_ = variable;

        if (st->value_) {
          if (st->value_->type_->isIntegerType()) {
            if (st->value_->bitWidth() <= 64 && st->value_ != 0) {
              st->value_->literal_ = LiteralKind::I64;
            } else if (st->value_->bitWidth() > 64 && st->value_ != 0) {
              st->value_->literal_ = LiteralKind::I128;
            } else {
              // todo -> error
            }
          } else if (st->value_->type_->isFloatType()) {
            if (st->value_->bitWidth() <= 64 && st->value_ != 0) {
              st->value_->literal_ = LiteralKind::I64;
            } else {
              // todo -> error
            }
          }

          if (!st->type_ && st->value_->type_) st->type_ = st->value_->type_->clone();
          else if (dty && !match_type(st->type_.get(), st->value_->type_)) {
            // todo -> error
          }
        }

        symbols->declare(variable);

        break;
      }
      case StmtKind::EXPR: {
        analyze_expression(st->value_.get());
        break;
      }
      default:
        return;
    }
  }

  void SemanticAnalyzer::analyze_expression(Expression* ex) {
    if (!ex) return;
    ex->type_ = new Type();

    switch (ex->kind_) {
      case ExprKind::LITERAL: {
        ex->type_->kind_ = ast::TypeKind::LITERAL;
        ex->type_->literal_ = ex->literal_;
        break;
      }
      case ExprKind::VARIABLE: {
        auto variable = symbols->lookup(ex->name_);

        if (!variable) break;

        if (variable->kind_ == SymbolKind::ALIAS) {
          variable = variable->ref_;
        }

        ex->symbols_ = variable;
        ex->type_ = variable->type_;
        break;
      }
      case ExprKind::SCOPE: {
        analyze_expression(ex->nested_.get());
        auto scope = (Symbol*)ex->nested_->symbols_;
        if (!scope) {
          // todo -> error
          break;
        }
        auto nested = scope->lookup(ex->name_);
        if (!nested) {
          // todo error
          break;
        }
        ex->symbols_ = nested;
        ex->type_ = nested->type_;
        break;
      }
      case ExprKind::MEMBER: {
        analyze_expression(ex->nested_.get());
        auto scope = (Symbol*)ex->nested_->symbols_;
        if (!scope) {
          // todo -> error
          break;
        }
        auto nested = scope->lookup(ex->name_);
        if (!nested) {
          // todo error
          break;
        }
        ex->symbols_ = nested;
        ex->type_ = nested->type_;
        break;
      }
      case ExprKind::CALL: {
        analyze_expression(ex->callee_.get());
        auto sym = (Symbol*)ex->callee_->symbols_;

        if (!sym) {
          // todo -> error
          break;
        }

        if (sym->kind_ == SymbolKind::ALIAS) {
          sym = sym->ref_;
        }

        if (sym->kind_ != SymbolKind::FUNCTION) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            ex->loc_,
            "called symbol is not a function"
          });
          break;
        }

        for (auto& arg : ex->args_) {
          analyze_expression(arg.get());
        }

        if (sym->kind_ != SymbolKind::FUNCTION) {
          // todo -> error
          break;
        }

        ex->type_ = sym->type_;
        ex->symbols_ = sym;
        break;
      }
      default: return;
    }
  }

  void SemanticAnalyzer::analyze_type(Type* ty) {
    if (!ty) return;

    switch(ty->kind_) {
      case TypeKind::SCOPE: { break; }
      default: return;
    }
  }

  Symbol* SemanticAnalyzer::lookup_type(Type* ty) {
    if (!ty) return nullptr;

    switch(ty->kind_) {
      case TypeKind::OBJECT: {
        auto sym = symbols->lookup(ty->name_);
        ty->symbols_ = sym;
        return sym;
      }
      case TypeKind::SCOPE: {
        auto sym = lookup_type(ty->nested_.get());
        if (!sym) return nullptr;
        sym = sym->lookup(ty->name_);
        ty->symbols_ = sym;
        return sym;
      }
      default:
        return nullptr;
    }
  }

  bool SemanticAnalyzer::match_type(Type* l, Type* r) {
    if (!l) return false;
    if (!r) return false;

    if (l->kind_ == TypeKind::LITERAL && r->kind_ == TypeKind::LITERAL) {
      if (l->isIntegerType() && r->isIntegerType()) {
        if (r->literal_ == LiteralKind::UNK_INT) r = l;
        if (l->bitWidth() == r->bitWidth()) return true;
        else return false;
      } else if (l->isFloatType() && r->isFloatType()) {
        if (r->literal_ == LiteralKind::UNK_FLOAT) r = l;

        if (l->bitWidth() == r->bitWidth()) return true;
        else return false;
      } else return false;
    }

    auto lsym = lookup_type(l);
    auto rsym = lookup_type(r);
    if (!lsym) return false;
    return lsym == rsym;
  }

  std::string SemanticAnalyzer::getExternalLibPath() {
    #ifdef _WIN32
      // Windows: program_files/sonic_lib/
      const char* programFiles = std::getenv("ProgramFiles");
      if (programFiles) {
        return std::string(programFiles) + "\\sonic_lib";
      }
    #else
      // Linux/Mac: ~/.local/share/lib/sonic_lib/
      const char* homeDir = std::getenv("HOME");
      if (homeDir) {
        return std::string(homeDir) + "/.local/share/lib/sonic_lib";
      }
    #endif
    return "";
  }

  SemanticAnalyzer::ModuleResolution SemanticAnalyzer::resolveModulePath(const std::vector<std::unique_ptr<ast::Statement>>& qualified) {
    if (qualified.empty()) {
      return {"", ModuleSource::LOCAL, false};
    }

    // Build the relative path from qualified names
    std::string relativePath = "";
    for (size_t i = 0; i < qualified.size(); i++) {
      if (i > 0) relativePath += "/";
      relativePath += qualified[i]->name_;
    }


    // 1. Try local directory (where current file is)
    std::string localPath = sonic::io::getFullPath(filepath) + "/" + relativePath;

    // Check if it's a file with .sn extension
    if (sonic::io::is_exists(localPath + ".sn") && sonic::io::is_file(localPath + ".sn")) {
      return {localPath + ".sn", ModuleSource::LOCAL, false};
    }

    // Check if it's a directory
    if (sonic::io::is_exists(localPath) && !sonic::io::is_file(localPath)) {
      return {localPath, ModuleSource::LOCAL, true};
    }

    // 2. Try project root directory (where main.sn is)
    // Get project root by going up from filepath until we find main.sn
    std::string currentDir = sonic::io::getPathWithoutFile(sonic::io::getFullPath(filepath));
    while (!currentDir.empty() && currentDir != "/") {
      std::string projectPath = currentDir + "/" + relativePath;

      if (sonic::io::is_exists(projectPath + ".sn") && sonic::io::is_file(projectPath + ".sn")) {
        return {projectPath + ".sn", ModuleSource::PROJECT, false};
      }

      if (sonic::io::is_exists(projectPath) && !sonic::io::is_file(projectPath)) {
        return {projectPath, ModuleSource::PROJECT, true};
      }

      // Move up one directory
      size_t lastSlash = currentDir.find_last_of('/');
      if (lastSlash == std::string::npos) break;
      currentDir = currentDir.substr(0, lastSlash);
    }

    // 3. Try external libraries
    std::string externalLib = getExternalLibPath();
    if (!externalLib.empty()) {

      std::string externalPath = externalLib + "/" + relativePath;

      if (sonic::io::is_exists(externalPath + ".sn") && sonic::io::is_file(externalPath + ".sn")) {
        return {externalPath + ".sn", ModuleSource::EXTERNAL, false};
      }

      if (sonic::io::is_exists(externalPath) && !sonic::io::is_file(externalPath)) {

        return {externalPath, ModuleSource::EXTERNAL, true};
      }
    }

    // Module not found
    return {"", ModuleSource::LOCAL, false};
  }

  std::unique_ptr<ast::Program> SemanticAnalyzer::loadAndAnalyzeModule(const std::string& modulePath) {
    if (!sonic::io::is_exists(modulePath) || !sonic::io::is_file(modulePath)) {
      return nullptr;
    }

    std::string content = sonic::io::read_file(modulePath);
    sonic::frontend::Lexer lexer(content, sonic::io::getFullPath(modulePath));
    lexer.diag = diag;

    sonic::frontend::Parser parser(sonic::io::getFullPath(modulePath), &lexer);
    parser.diag = diag;
    std::unique_ptr<ast::Program> program = parser.parse();

    if (!program) return nullptr;

    SemanticAnalyzer analyzer(groups);
    analyzer.filepath = sonic::io::getPathWithoutFile(modulePath);
    analyzer.diag = diag;
    analyzer.entrySymbol = entrySymbol;
    analyzer.analyze(program.get());

    return program;
  }

  void SemanticAnalyzer::loadDirectoryAsNamespace(const std::string& dirPath, Symbol* parentSymbol) {
    namespace fs = std::filesystem;

    // Check if directory exists
    if (!sonic::io::is_exists(dirPath)) {
      return;
    }

    // Iterate through directory entries
    for (const auto& entry : fs::directory_iterator(dirPath)) {
      if (entry.is_regular_file() && entry.path().extension() == ".sn") {
        // Parse and analyze each .sn file
        std::string filePath = entry.path().string();
        std::unique_ptr<ast::Program> module = loadAndAnalyzeModule(filePath);

        if (module && parentSymbol) {
          // Create namespace for this file
          auto nsSymbol = new Symbol();
          nsSymbol->kind_ = SymbolKind::NAMESPACE;
          nsSymbol->name_ = sonic::io::getFileNameWithoutExt(entry.path().filename().string());
          nsSymbol->mangle_ = parentSymbol->mangle_ + "_" + nsSymbol->name_;

          // Add public symbols from module to namespace
          for (auto& stmt : module->statements_) {
            if (stmt->public_) {
              auto alias = new Symbol();
              alias->name_ = stmt->name_;
              alias->kind_ = SymbolKind::ALIAS;
              alias->ref_ = (Symbol*)stmt->symbols_;
              nsSymbol->declare(alias);
            }
          }

          parentSymbol->declare(nsSymbol);
        }
      }
      else if (entry.is_directory()) {
        // Recursively load subdirectories
        auto subDirSymbol = new Symbol();
        subDirSymbol->kind_ = SymbolKind::NAMESPACE;
        subDirSymbol->name_ = entry.path().filename().string();
        subDirSymbol->mangle_ = parentSymbol->mangle_ + "_" + subDirSymbol->name_;

        parentSymbol->declare(subDirSymbol);
        loadDirectoryAsNamespace(entry.path().string(), subDirSymbol);
      }
    }
  }
};
