#pragma once

#include <string>
#include <vector>

#include "type.h"
#include "expr.h"
#include "../token.h"
#include "../source.h"

namespace sonic::frontend {
  struct Symbol;

  enum class StmtKind {
    PROGRAM,
    BLOCK,
    FUNCTION,
    VARDECL,
    EXPRTOSTMT,
    ASSIGN,
    STRUCT,
    RETURN,
    IMPORT,
    IFELSE,
    FORLOOP,
    WHILELOOP,
  };

  inline std::string stmtKindToString(StmtKind k) {
    switch (k) {
      case StmtKind::PROGRAM: return "program";
      case StmtKind::BLOCK: return "block";
      case StmtKind::FUNCTION: return "function";
      case StmtKind::VARDECL: return "vardecl";
      case StmtKind::EXPRTOSTMT: return "expertoStmt";
      case StmtKind::RETURN: return "return";
      case StmtKind::ASSIGN: return "assign";
      case StmtKind::STRUCT: return "struct";
      case StmtKind::IFELSE: return "ifelse";
      case StmtKind::IMPORT: return "import";
      case StmtKind::FORLOOP: return "forloop";
      case StmtKind::WHILELOOP: return "whileloop";
      default: return "<unknown-stmt>";
    }

    return "<unknown-stmt>";
  }

  struct Generic {
    std::string name;
    Type* type = nullptr;

    SourceLocation locations;

    Generic() = default;
    Generic(std::string name, Type* type = nullptr)
      : name(name), type(type) {}
  };

  struct MacroAttr {
    std::string name;
    std::vector<Token> tokens;

    SourceLocation locations;

    MacroAttr() = default;
  };

  struct ImportItem {
    std::string name;
    std::string alias = "";
    bool useAlias = false;

    SourceLocation locations;

    ImportItem() = default;
  };

  struct Params {
    std::string name;
    Type* type;
    bool isVariadic = false;

    SourceLocation locations;

    Params() = default;
    Params(std::string name, Type* type, bool isVariadic, SourceLocation location)
      : name(name), type(type), isVariadic(isVariadic), locations(location) {}
  };

  struct StructField {
    std::string name;
    Type* type = nullptr;

    SourceLocation locations;

    StructField() = default;
  };

  struct EnumVariant {
    std::string name;
    Expr* value = nullptr;

    SourceLocation locations;
    EnumVariant() = default;
  };

  struct Stmt {
    StmtKind kind;
    SourceLocation locations;
    std::string filename;

    std::string name;
    std::vector<std::string> importPath;

    std::vector<Params> params;
    std::vector<Generic*> genericType;
    std::vector<StructField> structFields;
    std::vector<Stmt*> children;
    std::vector<MacroAttr> macros;
    std::vector<EnumVariant> enumVariants;
    std::vector<ImportItem> importItems;

    Stmt* body = nullptr;
    Stmt* then_block = nullptr;
    Stmt* else_block = nullptr;

    Expr* object = nullptr;
    Expr* value = nullptr;

    Type* returnType = nullptr;
    Type* dataType = nullptr;

    bool isPublic = false;
    bool isStatic = false;
    bool isConstant = false;
    bool isExtern = false;
    bool isDefine = false;
    bool isDeclare = false;
    bool isImportAll = false;

    Symbol* symbol = nullptr;

    Stmt() = default;
  };

};
