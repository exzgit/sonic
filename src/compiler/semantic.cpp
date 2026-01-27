#include "semantic.h"
#include "diagnostics.h"
#include "../core/startup.h"
#include "symbol.h"
#include "type.h"

using namespace sonic::startup;
using namespace sonic::config;
using namespace sonic::io;

namespace sonic::frontend {
  void SemanticAnalyzer::lazy_analyze(Stmt* stmt) {
    switch (stmt->kind) {
      case StmtKind::PROGRAM: {
        std::cout << "(semantic) stmt: lazy analyze program statement\n";
        std::cout << "\t" << stmt->filename << "\n";

        Symbol* nmSymbol = new Symbol();
        nmSymbol->kind = SymbolKind::NAMESPACE;
        nmSymbol->name = stmt->filename;
        nmSymbol->table = new SymbolTable(symbol);
        nmSymbol->statement = stmt;
        nmSymbol->scopeLevel = scopeLevel;

        auto* save = symbol;
        symbol = nmSymbol;

        for (auto* child : stmt->children) {
          lazy_analyze(child);
        }

        symbol = save;

        if (!symbol->table) {
          universe->table = new SymbolTable();
        }
        universe->table->insert(nmSymbol);
        stmt->symbol = nmSymbol;

        break;
      }
      case StmtKind::FUNCTION: {
        std::cout << "(semantic) stmt: lazy analyze function statement\n";
        std::cout << "\t" << stmt->name << "\n";
        Symbol* fnSymbol = new Symbol();

        if (symbol->table->is_exists(stmt->name)) {
          auto* existing = symbol->table->find(stmt->name);
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "function `" + stmt->name + "` is already " + (existing->isDeclare ? "declared" : "defined")
          });
          return;
        }

        auto* save_symbol = symbol;

        fnSymbol->kind = SymbolKind::FUNCTION;
        fnSymbol->name = stmt->name;
        scopeLevel = FUNCTION_SCOPE;
        fnSymbol->scopeLevel = scopeLevel;
        fnSymbol->location = stmt->locations;
        fnSymbol->returnType = stmt->returnType;
        fnSymbol->isDeclare = true;
        fnSymbol->isPublic = stmt->isPublic;
        fnSymbol->isStatic = stmt->isStatic;
        fnSymbol->table = new SymbolTable(symbol);
        fnSymbol->scopeLevel = scopeLevel;
        std::cout << "(semantic) stmt: save function " + stmt->name + " in symbol table\n";
        symbol->table->insert(fnSymbol);

        symbol = fnSymbol;
        if (stmt->isExtern) {
          fnSymbol->mangledName = stmt->name;
        } else {
          fnSymbol->mangledName = pathToNamespace(stmt->locations.path) + "_" + stmt->name;
        }

        if (!stmt->params.empty()) {
          fnSymbol->mangledName += "_";
        } else {
          fnSymbol->mangledName += "_v";
        }

        for (auto& param : stmt->params) {
          fnSymbol->mangledName += "_";

          if (param.isVariadic) {
            fnSymbol->mangledName += "V";
            fnSymbol->isVariadic = true;
          }

          Symbol* paramSymbol = new Symbol();
          paramSymbol->kind = SymbolKind::PARAMETER;
          paramSymbol->name = param.name;
          paramSymbol->dataType = param.type;
          fnSymbol->mangledName += resolveTypeToString(param.type);
          fnSymbol->table->insert(paramSymbol);
        }

        if (stmt->isDeclare) {
          fnSymbol->isDeclare = true;
          break;
        }

        fnSymbol->statement = stmt;
        symbol = save_symbol;
        scopeLevel = GLOBAL_SCOPE;
        break;
      }
      case StmtKind::VARDECL: {
        if (scopeLevel == FUNCTION_SCOPE) {
          // local variable
          break;
        }
        std::cout << "(semantic) stmt: lazy analyze variable declaration\n";
        std::cout << "\t" << stmt->name << "\n";

        Symbol* varSymbol = new Symbol();

        if (!stmt->isStatic && !stmt->isConstant) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "local variable `" + stmt->name + "` is not supported in global scope"
          });
          break;
        }

        auto* existing = symbol->table->find(stmt->name);
        if (existing) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable `" + stmt->name + "` is already " + (existing->isDeclare ? "declared" : "defined")
          });
          return;
        }

        varSymbol->scopeLevel = scopeLevel;
        varSymbol->kind = SymbolKind::VARIABLE;
        varSymbol->name = stmt->name;
        varSymbol->scopeLevel = scopeLevel;
        varSymbol->isStatic = stmt->isStatic;
        varSymbol->isConst = stmt->isConstant;
        varSymbol->dataType = stmt->dataType;
        varSymbol->location = stmt->locations;

        if (stmt->isExtern) {
          varSymbol->mangledName = stmt->name;
        } else {
          varSymbol->mangledName = pathToNamespace(stmt->locations.path) + "_" + stmt->name;
        }

        varSymbol->statement = stmt;

        std::cout << "(semantic) stmt: save variable " + stmt->name + " in symbol table\n";

        if (stmt->isDeclare) {
          varSymbol->isDeclare = true;
        }

        if (stmt->value) {
          analyze_expr(stmt->value);
        } else {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable declaration `" + stmt->name + "` must be initialized"
          });
          break;
        }

        symbol->table->insert(varSymbol);

        if (stmt->isConstant) {
          if (stmt->dataType->isNullable) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->locations,
              "constant variable `" + stmt->name + "` cannot be nullable"
            });
            break;
          }

          if (stmt->value->type->kind == TypeKind::NULLABLE) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->locations,
              "constant variable `" + stmt->name + "` cannot be assigned a nullable value"
            });
            break;
          }
        }
        else if (!stmt->dataType->isNullable && stmt->value->type->kind == TypeKind::NULLABLE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "cannot assign non-nullable value to nullable variable `" + stmt->name + "`"
          });
        } else {
          if (stmt->dataType->isNullable && stmt->value->type->kind == TypeKind::NULLABLE) {
            stmt->dataType->isNullable = true;
          } else {
            if (stmt->value->type->kind == TypeKind::NULLABLE) {
              stmt->dataType->isNullable = true;
            }
            else {
              if (stmt->dataType->kind == TypeKind::PRIMITIVE && stmt->value->type->kind == TypeKind::PRIMITIVE) {
                if (stmt->dataType->primitive != stmt->value->type->primitive) {
                  diag->report({
                    ErrorType::SEMANTIC,
                    Severity::ERROR,
                    stmt->locations,
                    "type mismatch in variable declaration `" + stmt->name + "`"
                  });
                } else {
                  stmt->dataType = stmt->value->type;
                }
              } else if (stmt->dataType->mangledName != stmt->value->type->mangledName) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->locations,
                  "type mismatch in variable declaration `" + stmt->name + "`"
                });
              } else {
                stmt->dataType = stmt->value->type;
              }
            }
          }
        }

        symbol->table->insert(varSymbol);

        break;
      }
      default: return;
    }
  }

  void SemanticAnalyzer::analyze(Stmt* stmt) {
    if (!symbol) {
      symbol = new Symbol();
    }

    lazy_analyze(stmt);
    scopeLevel = GLOBAL_SCOPE;
    analyze_stmt(stmt);
  }

  void SemanticAnalyzer::analyze_stmt(Stmt* stmt) {
    switch(stmt->kind) {
      case StmtKind::PROGRAM: {
        std::cout << "(semantic) stmt: analyze program statement\n";

        auto* saveSymbol = symbol;
        symbol = universe->table->find(stmt->filename);

        auto& children = stmt->children;

        assert(children.data() != nullptr);
        assert(children.size() < 100000);

        for (auto* child : stmt->children) {
          analyze_stmt(child);
        }

        symbol = saveSymbol;

        break;
      }
      // IMPORT STATEMENT
      case StmtKind::IMPORT: {
        std::string modulePath = "";

        for (const auto& path : stmt->importPath) {
          modulePath += "/" + path;
        }

        modulePath = project_root + modulePath + ".sn";

        if (!sonic::io::is_exists(modulePath)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "module not found '" + modulePath + "'"
          });
          break;
        } else if (sonic::io::is_exists(modulePath) && !sonic::io::is_file(modulePath)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "module is not file '" + modulePath + "'"
          });
          break;
        }

        std::string content = read_file(modulePath);

        sonic::frontend::Lexer lexer(content, modulePath);
        lexer.diag = diag;
        sonic::frontend::Parser parser(modulePath, &lexer);
        parser.diag = diag;
        auto* prog = parser.parse();

        auto* nmSymbol = new Symbol();
        nmSymbol->kind = SymbolKind::NAMESPACE;
        nmSymbol->name = modulePath;
        nmSymbol->table = new SymbolTable();
        nmSymbol->statement = stmt;

        SemanticAnalyzer semantic(universe);
        semantic.diag = diag;
        semantic.analyze(prog);

        Symbol* moduleSymbol = universe->table->find(modulePath);

        if (!stmt->isImportAll) {
          for (const auto& item : stmt->importItems) {
            auto* useSymbol = moduleSymbol->table->find(item.name);

            if (useSymbol) {

              if (!useSymbol->isPublic) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  useSymbol->location,
                  symbolKindToString(useSymbol->kind) + " '" + useSymbol->name + "' is not public",
                  "",
                  "use 'public' syntax before 'func' syntax"
                });
              }

              auto* aliasSymbol = new Symbol();
              aliasSymbol->kind = SymbolKind::ALIAS;
              aliasSymbol->scopeLevel = scopeLevel;
              aliasSymbol->location = item.locations;
              aliasSymbol->isPublic = false;
              aliasSymbol->name = (item.useAlias ? item.alias : item.name);
              aliasSymbol->ref = useSymbol;
              symbol->table->insert(aliasSymbol);
            } else {
              std::cout << "undefined import module '" + item.name + "'\n";
              break;
            }
          }
        } else {
          for (auto* useSymbol : moduleSymbol->table->symbols) {
            if (useSymbol->kind == SymbolKind::FUNCTION
              || useSymbol->kind == SymbolKind::STRUCT
              || useSymbol->kind == SymbolKind::ENUM
              || useSymbol->kind == SymbolKind::VARIABLE)
            {

              if (!useSymbol->isPublic) {
                continue;
              }

              auto* aliasSymbol = new Symbol();
              std::cout << "alias: " << useSymbol->name << " as " << useSymbol->name << "\n";
              aliasSymbol->kind = SymbolKind::ALIAS;
              aliasSymbol->location = useSymbol->location;
              aliasSymbol->name = useSymbol->name;
              aliasSymbol->scopeLevel = scopeLevel;
              aliasSymbol->isPublic = false;
              aliasSymbol->ref = useSymbol;
              symbol->table->insert(aliasSymbol);
            }
          }
        }
        break;
      }
      case StmtKind::FUNCTION: {
        std::cout << "(semantic) stmt: analyze function statement '" + stmt->name + "'\n";
        if (scopeLevel == FUNCTION_SCOPE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "nested function definitions are not allowed"
          });
          break;
        }

        if (!symbol) {
          std::cout << "Symbol is null " << stmt->name << "\n";
          return;
        }

        Symbol* sym = symbol->table->find_local(stmt->name);
        if (!sym) {
          std::cout << "report\n";
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "undefined function '" + stmt->name + "'"
          });
          return;
        }

        if (sym && sym->kind != SymbolKind::FUNCTION) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable '" + stmt->name + "' is not function"
          });
          return;
        }

        if (sym->isExtern && sym->isDeclare) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "function '" + stmt->name + "' is extern declared"
          });
          return;
        }

        sym->statement = stmt;
        sym->scopeLevel = FUNCTION_SCOPE;

        Symbol* current_symbol = symbol;
        symbol->scopeLevel = FUNCTION_SCOPE;
        scopeLevel = FUNCTION_SCOPE;
        symbol = sym;

        scopeLevel = FUNCTION_SCOPE;
        analyze_stmt(stmt->body);
        scopeLevel = GLOBAL_SCOPE;

        symbol = current_symbol;
        scopeLevel = GLOBAL_SCOPE;

        break;
      }
      case StmtKind::BLOCK: {
        std::cout << "(semantic) stmt: analyze block statement\n";
        for (auto* child : stmt->children) {
          analyze_stmt(child);
        }
        break;
      }
      case StmtKind::RETURN: {
        if (scopeLevel == GLOBAL_SCOPE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "return statement outside of function or block"
          });
          return;
        }

        std::cout << "(semantic) stmt: analyze return statement\n";

        if (scopeLevel == FUNCTION_SCOPE && scopeLevel == BLOCK_SCOPE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "return statement outside of function"
          });
          return;
        }

        // nullability untuk tipe data
        bool isNullable = false;

        // jika value tidak null, maka lanjutkan analisa expression
        if (stmt->value)
          analyze_expr(stmt->value);

        // check data type dan nullability dari symbol untuk mengubah variable isNullability
        // check return type dan nullability dari symbol untuk mengubah variable isNullability
        if (symbol->returnType)
          isNullable = symbol->returnType->isNullable;

        // check apakah value ada saat symbol saat ini adalah variable
        if (symbol->returnType->kind == TypeKind::VOID && stmt->value) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "expected return void but got '" + resolveTypeToString(stmt->value->type) + "'"
          });
        }

        if (stmt->value) {
          if (scopeLevel == BLOCK_SCOPE) {
            if (!stmt->value->type) {
              return;
            }

            if (stmt->value->type->kind == TypeKind::NULLABLE) {
              if (!isNullable) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->locations,
                  "unexpected none at return value"
                });
              }
              symbol->returnType = stmt->value->type;
              break;
            } else

            // check tipe untuk primitive
            if (symbol->dataType->kind == TypeKind::PRIMITIVE && stmt->value->type->kind == TypeKind::PRIMITIVE) {
              if (symbol->dataType->primitive == stmt->value->type->primitive) {
                symbol->returnType = stmt->value->type;
                symbol->returnType->isNullable = isNullable;
              } else {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->value->locations,
                  "return type mismatch"
                });
              }
            }

            break;
          } else if (scopeLevel == FUNCTION_SCOPE) {
            if (stmt->value->type->kind == TypeKind::NULLABLE) {
              if (!isNullable) {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->locations,
                  "unexpected none at return value"
                });
              }
              break;
            }

            // check tipe untuk primitive
            if (symbol->returnType->kind == TypeKind::PRIMITIVE && stmt->value->type->kind == TypeKind::PRIMITIVE) {
              if (symbol->returnType->primitive == stmt->value->type->primitive) {
                stmt->dataType = stmt->value->type;
              } else {
                diag->report({
                  ErrorType::SEMANTIC,
                  Severity::ERROR,
                  stmt->value->locations,
                  "return type mismatch"
                });
              }
            }
          }
        } else {
          auto* ty = new Type();
          ty->kind = TypeKind::VOID;

          if (symbol->returnType && symbol->returnType->kind == TypeKind::VOID) {
            symbol->returnType = ty;
          } else {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->value->locations,
              "function is not return void type"
            });
          }
        }
        break;
      }
      case StmtKind::IFELSE: {
        std::cout << "(semantic) stmt: analyze conditional statement\n";
        if (scopeLevel == GLOBAL_SCOPE) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->value->locations,
            "conditional statements are not supported at global scope."
          });
          break;
        }

        if (stmt->value)
          analyze_expr(stmt->value);
        else {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->value->locations,
            "expected condition"
          });
        }

        if (stmt->value->type) {
          if (stmt->value->type->isNullable) {}
          else if (stmt->value->type->kind == TypeKind::PRIMITIVE) {
          } else if (stmt->value->type->kind == TypeKind::NULLABLE) {
          } else {
            std::string msg = "condition must be of type bool, but got '" + resolveTypeToString(stmt->value->type) + "'";

            if (stmt->value->type->kind == TypeKind::AUTO) {
              msg = "condition must be of type bool, but got '" + resolveTypeToString(stmt->value->type) + "'";
            }

            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->value->locations,
              msg
            });
          }
        }

        analyze_stmt(stmt->then_block);
        if (stmt->else_block)
          analyze_stmt(stmt->else_block);

        break;
      }
      case StmtKind::VARDECL: {
        if (scopeLevel == GLOBAL_SCOPE) {
          std::cout << "(semantic) stmt: analyze global variable declaration '" + stmt->name + "'\n";
          // global variable
          break;
        }
        std::cout << "(semantic) stmt: analyze variable declaration '" + stmt->name + "'\n";
        if (symbol->table->find_local(stmt->name)) {
          auto* existing = symbol->table->find_local(stmt->name);
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable `" + stmt->name + "` is already " + (existing->isDeclare ? "declared" : "defined")
          });
          return;
        }

        if (stmt->dataType && stmt->dataType->kind == TypeKind::VOID) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable declaration with void type"
          });
        }

        if (symbol->kind == SymbolKind::FUNCTION && (stmt->isStatic || stmt->isConstant)) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            stmt->locations,
            "variable declaration in function scope"
          });
        }

        Symbol* varSymbol = new Symbol();
        varSymbol->kind = SymbolKind::VARIABLE;
        varSymbol->name = stmt->name;
        varSymbol->scopeLevel = scopeLevel;
        varSymbol->mangledName = pathToNamespace(stmt->locations.path) + "_" + symbol->name + "_" + stmt->name;
        varSymbol->dataType = stmt->dataType;
        varSymbol->returnType = stmt->dataType;
        varSymbol->statement = stmt;
        varSymbol->isDeclare = stmt->value ? false : true;
        varSymbol->table = new SymbolTable(symbol);

        if (stmt->value) {
          stmt->value->symbol = varSymbol;
          analyze_expr(stmt->value);
        }

        if (stmt->value) {

          if (!stmt->value->type) {
            return;
          }

          if (!stmt->dataType->isNullable && stmt->value->type->kind == TypeKind::NULLABLE) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              stmt->value->locations,
              "cannot assign non-nullable value to nullable variable `" + stmt->name + "`"
            });
          } else {
            if (stmt->dataType->isNullable && stmt->value->type->kind == TypeKind::NULLABLE) {
              stmt->dataType->isNullable = true;
            } else {
              if (stmt->value->type->kind == TypeKind::NULLABLE) {
                stmt->dataType->isNullable = true;
              }
              else {
                if (stmt->dataType->kind == TypeKind::PRIMITIVE && stmt->value->type->kind == TypeKind::PRIMITIVE) {
                  if (stmt->dataType->primitive != stmt->value->type->primitive) {
                    diag->report({
                      ErrorType::SEMANTIC,
                      Severity::ERROR,
                      stmt->locations,
                      "type mismatch in variable declaration `" + stmt->name + "`"
                    });
                  } else {
                    stmt->dataType = stmt->value->type;
                    if (stmt->dataType->isNullable) {
                      stmt->dataType->isNullable = true;
                    }
                  }
                } else if (stmt->dataType->mangledName != stmt->value->type->mangledName) {
                  diag->report({
                    ErrorType::SEMANTIC,
                    Severity::ERROR,
                    stmt->locations,
                    "type mismatch in variable declaration `" + stmt->name + "`"
                  });
                } else {
                  stmt->dataType = stmt->value->type;
                  if (stmt->dataType->isNullable) {
                    stmt->dataType->isNullable = true;
                  }
                }
              }
            }
          }
        }

        symbol->table->insert(varSymbol);
        break;
      }
      default: break;
    }
  }

  void SemanticAnalyzer::analyze_expr(Expr* expr) {
    switch (expr->kind) {
      case ExprKind::NONE: {
        std::cout << "(semantic) expr: analyze none expression\n";
        expr->type = new Type();
        expr->type->kind = TypeKind::NULLABLE;
        break;
      }
      case ExprKind::PRIMITIVE:
      {
        std::cout << "(semantic) expr: analyze primitive expression '" << expr->value << "'\n";
        switch (expr->primitive) {
          case Primitive::I32: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::I32;
            break;
          }
          case Primitive::I64: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::I64;
            break;
          }
          case Primitive::F32: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::F32;
            break;
          }
          case Primitive::F64: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::F64;
            break;
          }
          case Primitive::BOOL: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::BOOL;
            break;
          }
          case Primitive::STR: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::STR;
            break;
          }
          case Primitive::CHAR: {
            expr->type = new Type();
            expr->type->kind = TypeKind::PRIMITIVE;
            expr->type->primitive = Primitive::CHAR;
            break;
          }
          default: break;
        }
        break;
      }
      case ExprKind::IDENT: {
        std::cout << "(semantic) expr: analyze identifier expression '" << expr->value << "'\n";
        Symbol* sym = symbol->table->find(expr->value);

        if (!sym) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->locations,
            "variable '" + expr->value + "' is not defined"
          });
          expr->type = nullptr;
          return;
        }

        if (sym && sym->kind == SymbolKind::ALIAS) {
          if (sym->ref->kind == SymbolKind::VARIABLE) {
            expr->type = sym->ref->dataType;
            expr->name = sym->ref->name;
            expr->mangledName = sym->ref->mangledName;
            expr->symbol = sym->ref;
          } else {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->locations,
              "identifier " + expr->value + " is not defined"
            });
          }
        }
        else if (sym && sym->kind == SymbolKind::VARIABLE) {
          expr->type = sym->dataType;
          expr->name = sym->name;
          expr->mangledName = sym->mangledName;
          expr->symbol = sym;
          break;
        } else {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->locations,
            "identifier " + expr->value + " is not defined"
          });
        }
        break;
      }
      case ExprKind::CALL: {
        std::cout << "(semantic) expr: analyze call expression to '" << expr->callee->name << "'\n";
        if (expr->callee->kind == ExprKind::IDENT) {
          Symbol* sym = symbol->table->find(expr->callee->value);

          if (!sym) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->callee->locations,
              "called function '" + expr->callee->value + "' is not defined"
            });
            break;
          }

          if (sym && sym->kind == SymbolKind::ALIAS) {
            if (sym->ref->kind == SymbolKind::FUNCTION) {
              expr->type = sym->ref->dataType;
              expr->name = sym->ref->name;
              expr->mangledName = sym->ref->mangledName;
              expr->symbol = sym->ref;
            } else {
              diag->report({
                ErrorType::SEMANTIC,
                Severity::ERROR,
                expr->locations,
                "called symbol '" + expr->callee->value + "' is not a function"
              });
            }
          }
          else if (sym->kind != SymbolKind::FUNCTION) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->locations,
              "called symbol '" + expr->callee->value + "' is not a function"
            });
            break;
          } else {
            expr->type = sym->returnType;
            expr->symbol = sym;
          }
        } else {
          analyze_expr(expr->callee);
          if (expr->callee->symbol->kind != SymbolKind::FUNCTION) {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              expr->locations,
              "called expression is not a function"
            });
            break;
          } else {
            expr->type = expr->callee->symbol->returnType;
            expr->symbol = expr->callee->symbol;
          }
        }
        break;
      }
      case ExprKind::LOOKUP: {
        std::cout << "(semantic) expr: analyze lookup '" << expr->value << "'\n";
        analyze_expr(expr->object);
        Symbol* sym = expr->object->symbol->table->find(expr->value);
        if (!sym) {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            expr->locations,
            "member '" + expr->value + "' is not defined in type '" + resolveTypeToString(expr->type) + "'"
          });
        } else {
          expr->type = sym->dataType;
          expr->symbol = sym;
        }
        break;
      }
      case ExprKind::BINARY: {
        std::cout << "(semantic) expr: analyze binary expression\n";
        analyze_expr(expr->left);
        analyze_expr(expr->right);
        expr->type = resolveBinaryExpr(expr->left->type, expr->binOp, expr->right->type);
        expr->symbol = expr->left->symbol;
        break;
      }
      case ExprKind::BLOCK: {
        std::cout << "(semantic) expr: analyze block expression\n";
        auto currentScopeLevel = scopeLevel;

        auto* symbolBackup = symbol;
        symbol = expr->symbol;

        scopeLevel = BLOCK_SCOPE;
        analyze_stmt(expr->block);

        scopeLevel = currentScopeLevel;
        symbol = symbolBackup;

        expr->type = symbol->returnType;
        break;
      }
      default: break;
    }
  }

  void SemanticAnalyzer::analyze_type(Type* type) {

  }

  std::string SemanticAnalyzer::resolveTypeToString(Type* type) {
    switch (type->kind) {
      case TypeKind::NULLABLE:
        return "none";
      case TypeKind::VOID:
        return "void";
      case TypeKind::PRIMITIVE: {
        return primitiveToString(type->primitive);
      }
      case TypeKind::ANY: {
        return "any";
      }
      case TypeKind::IDENT: {
        if (symbol->table->is_exists(type->name)) {
          Symbol* sym = symbol->table->find(type->name);
          if (sym->kind == SymbolKind::STRUCT || sym->kind == SymbolKind::ENUM) {
            return sym->name;
          } else {
            diag->report({
              ErrorType::SEMANTIC,
              Severity::ERROR,
              sym->statement->locations,
              "Expected struct or enum type, but found " + symbolKindToString(sym->kind)
            });
          }
        } else {
          diag->report({
            ErrorType::SEMANTIC,
            Severity::ERROR,
            type->locations,
            "Unknown type: " + type->name + " (" + typeKindToString(type->kind) + ")"
          });
        }
      }
      default:
        return "undefined";
    }
  }

  Type* SemanticAnalyzer::resolveBinaryExpr(Type* left, BinaryOp op, Type* right) {
    if (left->kind == TypeKind::PRIMITIVE && right->kind == TypeKind::PRIMITIVE) {
      if (left->primitive == right->primitive) {
        return left;
      } else {
        diag->report({
          ErrorType::SEMANTIC,
          Severity::ERROR,
          left->locations,
          "Incompatible types for binary operation: " + primitiveToString(left->primitive) + " and " + primitiveToString(right->primitive)
        });
      }
    } else {
      diag->report({
        ErrorType::SEMANTIC,
        Severity::ERROR,
        left->locations,
        "Expected primitive type, but found " + typeKindToString(left->kind)
      });
    }
    return nullptr;
  }

}
