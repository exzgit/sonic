#pragma once

// c++ library
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>

// local header
#include "source.h"

namespace sonic::frontend {

  static std::string indent_str(int indent) {
    return std::string(indent * 2, ' ');
  }

  enum class Mutability {
    STATIC,
    CONSTANT,
    DEFAULT,
  };

  inline std::string mutability_to_string(Mutability mut) {
    switch(mut) {
      case sonic::frontend::Mutability::STATIC: return "static";
      case sonic::frontend::Mutability::CONSTANT: return "constant";
      default: return "default";
    }
  }

  enum class StmtKind {
    PROGRAM,
    IMPORT,
    VAR_DECL,
    FUNC_DECL,
    RETURN,
    BLOCK,
    ASSIGN,
    EXPR,
    UNKNOWN,
  };

  inline std::string stmtkind_to_string(StmtKind mut) {
    switch(mut) {
      case sonic::frontend::StmtKind::PROGRAM: return "program";
      case sonic::frontend::StmtKind::IMPORT: return "import";
      case sonic::frontend::StmtKind::VAR_DECL: return "variable";
      case sonic::frontend::StmtKind::FUNC_DECL: return "function";
      case sonic::frontend::StmtKind::RETURN: return "return";
      case sonic::frontend::StmtKind::BLOCK: return "block";
      case sonic::frontend::StmtKind::ASSIGN: return "assign";
      case sonic::frontend::StmtKind::EXPR: return "expr";
      default: return "unknown";
    }
  }

  enum class ExprKind {
    NONE,
    IDENT,
    CALL,
    BINARY,
    UNARY,
    UNTYPED_LITERAL,
    INDEX,
    INT,
    FLOAT,
    BOOL,
    CHAR,
    STRING,
    SCOPE_ACCESS,
    MEMBER_ACCESS,
    UNKNOWN,
  };

  inline std::string exprkind_to_string(ExprKind mut) {
    switch(mut) {
      case sonic::frontend::ExprKind::NONE: return "none";
      case sonic::frontend::ExprKind::IDENT: return "variable";
      case sonic::frontend::ExprKind::CALL: return "call";
      case sonic::frontend::ExprKind::BINARY: return "binary";
      case sonic::frontend::ExprKind::UNARY: return "unary";
      case sonic::frontend::ExprKind::UNTYPED_LITERAL: return "untyped";
      case sonic::frontend::ExprKind::INDEX: return "index";
      case sonic::frontend::ExprKind::INT: return "int";
      case sonic::frontend::ExprKind::FLOAT: return "float";
      case sonic::frontend::ExprKind::BOOL: return "bool";
      case sonic::frontend::ExprKind::CHAR: return "char";
      case sonic::frontend::ExprKind::STRING: return "string";
      case sonic::frontend::ExprKind::SCOPE_ACCESS: return "scope_access";
      case sonic::frontend::ExprKind::MEMBER_ACCESS: return "member_access";
      default: return "unknown";
    }
  }

  enum class TypeKind {
    ANY,
    AUTO,
    VOID,
    I32,
    I64,
    I128,
    F32,
    F64,
    BOOL,
    CHAR,
    STRING,
    ARRAY,
    IDENT,
    POINTER,
    REFERENCE,
    SCOPE_ACCESS,
    FUNCTION,
    STRUCT,
    ENUM,
    UNRESOLVED,
    UNKNOWN,
  };

  inline std::string typekind_to_string(TypeKind mut) {
    switch(mut) {
      case sonic::frontend::TypeKind::ANY: return "any";
      case sonic::frontend::TypeKind::AUTO: return "auto";
      case sonic::frontend::TypeKind::VOID: return "void";
      case sonic::frontend::TypeKind::I32: return "i32";
      case sonic::frontend::TypeKind::I64: return "i64";
      case sonic::frontend::TypeKind::I128: return "i128";
      case sonic::frontend::TypeKind::F32: return "f32";
      case sonic::frontend::TypeKind::F64: return "f64";
      case sonic::frontend::TypeKind::BOOL: return "bool";
      case sonic::frontend::TypeKind::CHAR: return "char";
      case sonic::frontend::TypeKind::STRING: return "string";
      case sonic::frontend::TypeKind::ARRAY: return "array";
      case sonic::frontend::TypeKind::IDENT: return "variable";
      case sonic::frontend::TypeKind::STRUCT: return "struct";
      case sonic::frontend::TypeKind::ENUM: return "enum";
      case sonic::frontend::TypeKind::SCOPE_ACCESS: return "scope_access";
      case sonic::frontend::TypeKind::FUNCTION: return "function";
      case sonic::frontend::TypeKind::UNRESOLVED: return "unresolved";
      default: return "unknown";
    }
  }

  // MARK: Sonic Type
  struct SonicType {
    TypeKind kind = TypeKind::UNKNOWN;
    SourceLocation location;

    std::string name;
    std::unique_ptr<SonicType> scope;

    std::vector<std::unique_ptr<SonicType>> params;
    std::unique_ptr<SonicType> returnType; // optional

    bool isNullable = false;
    bool isReference = false;
    bool isPointer = false;

    std::unique_ptr<SonicType> clone() const {
      auto t = std::make_unique<SonicType>();
      t->kind = kind;
      t->location = location;
      t->name = name;
      t->isNullable = isNullable;
      t->isReference = isReference;
      t->isPointer = isPointer;

      if (scope)
        t->scope = scope->clone();

      for (auto& p : params)
        t->params.push_back(p->clone());

      if (returnType)
        t->returnType = returnType->clone();

      return t;
    }

    std::string to_string(int indent = 0) {
      std::string out;
      auto ind = indent_str(indent);

      out += ind + "(Type ";
      if (!name.empty())
        out += name + " ";
      out += "kind=" + typekind_to_string(kind);

      if (isNullable)
        out += " nullable";

      if (isPointer)
        out += " ptr";

      if (isReference)
        out += " ref";

      out += "\n";

      if (scope) {
        out += ind + "  scope:\n";
        out += scope->to_string(indent + 2) + "\n";
      }

      if (!params.empty()) {
        out += ind + "  params:\n";
        for (auto& p : params)
          out += p->to_string(indent + 2) + "\n";
      }

      if (returnType) {
        out += ind + "  return:\n";
        out += returnType->to_string(indent + 2) + "\n";
      }

      out += ind + ")";
      return out;
    }

    int bitWidth() const {
      switch (kind) {
        case TypeKind::F32: return 32;
        case TypeKind::I32: return 32;
        case TypeKind::F64: return 64;
        case TypeKind::I64: return 64;
        case TypeKind::I128: return 128;
        default: return 0;
      }
    }

    bool isInteger() const {
      return (kind == TypeKind::I32 || kind == TypeKind::I64 || kind == TypeKind::I128);
    }

    bool isFloat() const {
      return (kind == TypeKind::F32 || kind == TypeKind::F64);
    }
  };

  // MARK: Sonic Expression
  struct SonicExpr {
    ExprKind kind = ExprKind::UNKNOWN;
    SourceLocation location;

    // IDENT / LITERAL
    std::string name;
    std::string value;
    std::string rawValue;

    // NESTED
    std::unique_ptr<SonicExpr> nested;
    std::unique_ptr<SonicExpr> index;

    // CALL
    std::unique_ptr<SonicExpr> callee;
    std::vector<std::unique_ptr<SonicType>> genericTypes;
    std::vector<std::unique_ptr<SonicExpr>> arguments;

    // BINARY / UNARY
    std::string op;
    std::unique_ptr<SonicExpr> lhs;
    std::unique_ptr<SonicExpr> rhs;

    // semantic
    SonicType* resolvedType = nullptr; // non-owning
    void* symbol = nullptr;            // non-owning

    std::unique_ptr<SonicExpr> clone() const {
      auto e = std::make_unique<SonicExpr>();
      e->kind = kind;
      e->location = location;
      e->name = name;
      e->value = value;
      e->op = op;

      if (nested) e->nested = nested->clone();
      if (callee) e->callee = callee->clone();
      if (index) e->index = index->clone();
      if (lhs) e->lhs = lhs->clone();
      if (rhs) e->rhs = rhs->clone();

      for (auto& a : arguments)
        e->arguments.push_back(a->clone());

      for (auto& a : genericTypes)
        e->genericTypes.push_back(a->clone());

      return e;
    }

    std::string to_string(int indent = 0) {
      std::string out;
      auto ind = indent_str(indent);

      out += ind + "(Expr kind=" + exprkind_to_string(kind);

      if (!op.empty())
        out += " op='" + op + "'";

      out += "\n";

      if (!name.empty())
        out += ind + "  name: " + name + "\n";

      if (!value.empty())
        out += ind + "  value: '" + value + "'\n";

      if (nested) {
        out += ind + "  nested:\n";
        out += nested->to_string(indent + 2) + "\n";
      }

      if (index) {
        out += ind + "  index:\n";
        out += index->to_string(indent + 2) + "\n";
      }

      if (callee) {
        out += ind + "  callee:\n";
        out += callee->to_string(indent + 2) + "\n";
      }

      if (!genericTypes.empty()) {
        out += ind + "  generics:\n";
        for (auto& g : genericTypes)
          out += g->to_string(indent + 2) + "\n";
      }

      if (!arguments.empty()) {
        out += ind + "  args:\n";
        for (auto& a : arguments)
          out += a->to_string(indent + 2) + "\n";
      }

      if (lhs) {
        out += ind + "  lhs:\n";
        out += lhs->to_string(indent + 2) + "\n";
      }

      if (rhs) {
        out += ind + "  rhs:\n";
        out += rhs->to_string(indent + 2) + "\n";
      }

      out += ind + ")";
      return out;
    }

    int inferIntegerBitWidth() {
      std::string s = value;

      // trim leading zero
      s.erase(0, s.find_first_not_of('0'));
      if (s.empty()) s = "0";

      auto fits = [&](int bits) {
        // hitung max = 2^(bits-1) - 1
        std::string max = "1";
        for (int i = 0; i < bits - 1; ++i) {
          int carry = 0;
          for (int j = max.size() - 1; j >= 0; --j) {
            int d = (max[j] - '0') * 2 + carry;
            max[j] = char('0' + (d % 10));
            carry = d / 10;
          }
          if (carry) max.insert(max.begin(), char('0' + carry));
        }

        // minus one
        for (int i = max.size() - 1; i >= 0; --i) {
          if (max[i] > '0') {
            max[i]--;
            break;
          }
          max[i] = '9';
        }

        // compare
        if (s.size() != max.size())
          return s.size() < max.size();
        return s <= max;
      };

      if (fits(32))  return 32;
      if (fits(64))  return 64;
      if (fits(128)) return 128;

      return 0; // overflow > i128
    }

  };

  // MARK: Sonic Parameter
  struct FunctionParameter {
    std::string name;
    std::unique_ptr<SonicType> type;
    bool variadic = false;

    SourceLocation location;

    std::unique_ptr<FunctionParameter> clone() const {
      auto p = std::make_unique<FunctionParameter>();
      p->name = name;
      p->location = location;
      p->variadic = variadic;
      p->type = type ? type->clone() : nullptr;
      return p;
    }

    std::string to_string(int indent = 0) {
      std::string out;
      auto ind = indent_str(indent);

      out += ind + "(Param ";
      out += name;

      if (variadic)
        out += " variadic";

      out += "\n";

      if (type) {
        out += ind + "  type:\n";
        out += type->to_string(indent + 2) + "\n";
      }

      out += ind + ")";
      return out;
    }
  };

  // MARK: Sonic Statement
  struct SonicStmt {
    StmtKind kind = StmtKind::UNKNOWN;
    SourceLocation location;

    std::string name;


    // assignment target
    std::unique_ptr<SonicExpr> target;

    // VAR / RETURN / EXPR
    std::unique_ptr<SonicExpr> expr;
    std::unique_ptr<SonicType> dataType;
    Mutability mutability = Mutability::DEFAULT;

    // FUNCTION
    std::vector<std::unique_ptr<SonicType>> genericParams;
    std::vector<std::unique_ptr<FunctionParameter>> parameters;
    std::unique_ptr<SonicType> returnType;
    std::vector<std::unique_ptr<SonicStmt>> body;


    // declaration options
    bool isPublic = false;
    bool isAsync = false;
    bool isExtern = false;

    // semantic
    void* symbol = nullptr;

    std::unique_ptr<SonicStmt> clone() const {
      auto s = std::make_unique<SonicStmt>();
      s->kind = kind;
      s->location = location;
      s->name = name;
      s->isPublic = isPublic;
      s->isExtern = isExtern;
      s->isAsync = isAsync;

      if (target) s->target = target->clone();
      if (expr) s->expr = expr->clone();
      if (returnType) s->returnType = returnType->clone();
      if (dataType) s->dataType = dataType->clone();

      for (auto& p : parameters)
        s->parameters.push_back(p->clone());

      for (auto& b : body)
        s->body.push_back(b->clone());

      return s;
    }

    std::string to_string(int indent = 0) {
      std::string out;
      auto ind = indent_str(indent);

      out += ind + "(Stmt kind=" + stmtkind_to_string(kind);

      if (!name.empty())
        out += " name=" + name;

      if (isPublic)
        out += " public";

      if (isExtern)
        out +=" extern";

      if (isAsync)
        out += " async";

      out += "\n";

      if (target) {
        out += ind + "  target:\n";
        out += target->to_string(indent + 2) + "\n";
      }

      if (expr) {
        out += ind + "  expr:\n";
        out += expr->to_string(indent + 2) + "\n";
      }

      if (dataType) {
        out += ind + "  type:\n";
        out += dataType->to_string(indent + 2) + "\n";
      }

      if (!genericParams.empty()) {
        out += ind + "  generics:\n";
        for (auto& generic : genericParams) {
          out += generic->to_string(indent + 3) + "\n";
        }
      }

      if (!parameters.empty()) {
        out += ind + "  params:\n";
        for (auto& p : parameters)
          out += p->to_string(indent + 2) + "\n";
      }

      if (returnType) {
        out += ind + "  return:\n";
        out += returnType->to_string(indent + 2) + "\n";
      }

      if (!body.empty()) {
        out += ind + "  body:\n";
        for (auto& b : body)
          out += b->to_string(indent + 2) + "\n";
      }

      out += ind + ")";
      return out;
    }

  };

}
