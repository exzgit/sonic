#include "debug.h"
#include "expr.h"
#include "source.h"
#include "stmt.h"
#include "type.h"
#include "../operand.h"
#include <iostream>
#include <string>

namespace sonic::frontend {
  void ASTPrinter::print(std::string v) {
    std::cout << std::string(indent_level * 2, ' ') << v << std::endl;
  }

  void ASTPrinter::print_stmt(const Stmt* stmt) {
    if (!stmt) {
      std::cout << "<stmt-nullptr>\n";
      return;
    }

    print("stmt@" + stmtKindToString(stmt->kind));

    indent_level++;
    if (!stmt->filename.empty())
      print("filename: " + stmt->filename);

    if (!stmt->name.empty())
      print("name: " + stmt->name);

    if (!stmt->params.empty())
    {
      print("params[" + std::to_string(stmt->params.size()) + "]");
      indent_level++;
      for (auto& param : stmt->params) {
        print("arg@" + param.name);
        indent_level++;
        print_type(param.type);
        if (param.isVariadic) print("isVariadic: true");
        indent_level--;
      }
      indent_level--;
    }

    if (!stmt->genericType.empty()) {
      print("genericType[" + std::to_string(stmt->genericType.size()) + "]");
      indent_level++;
      for (auto& generic : stmt->genericType) {
        print(generic->name);
        if (!generic->type) {
          indent_level++;
          print_type(generic->type);
          indent_level--;
        }
      }
      indent_level--;
    }

    if (!stmt->children.empty()) {
      print("children[" + std::to_string(stmt->children.size()) + "]");
      indent_level++;
      for (auto& child : stmt->children) {
        print_stmt(child);
      }
      indent_level--;
    }

    if (!stmt->macros.empty()) {
      print("macros:");
      indent_level++;
      for (auto& attr : stmt->macros) {
        print(attr.name);
        indent_level++;
        for (auto& tok : attr.tokens) {
          print("token: " + tok.value);
        }
        indent_level--;
      }
      indent_level--;
    }

    if (stmt->body) {
      print("body:");
      indent_level++;
      print_stmt(stmt->body);
      indent_level--;
    }

    if (stmt->then_block) {
      print("then_block:");
      indent_level++;
      print_stmt(stmt->then_block);
      indent_level--;
    }


    if (stmt->else_block) {
      print("else_block:");
      indent_level++;
      print_stmt(stmt->else_block);
      indent_level--;
    }

    if (stmt->object) {
      print("object:");
      indent_level++;
      print_expr(stmt->object);
      indent_level--;
    }

    if (stmt->value) {
      print("value:");
      indent_level++;
      print_expr(stmt->value);
      indent_level--;
    }

    if (stmt->returnType) {
      print("returnType:");
      indent_level++;
      print_type(stmt->returnType);
      indent_level--;
    }

    if (stmt->dataType) {
      print("dataType:");
      indent_level++;
      print_type(stmt->dataType);
      indent_level--;
    }

    if (stmt->isPublic) print("isPublic: true");
    if (stmt->isStatic) print("isStatic: true");
    if (stmt->isConstant) print("isConstant: true");
    if (stmt->isExtern) print("isExtern: true");
    if (stmt->isDefine) print("isDefine: true");
    if (stmt->isDeclare) print("isDeclare: true");

    debug_location(stmt->locations);

    indent_level--;
  }

  void ASTPrinter::print_expr(const Expr* expr) {
    if (!expr) {
      std::cout << "<expr-nullptr>\n";
      return;
    }

    print("expr@" + exprKindToString(expr->kind));
    indent_level++;
    if (expr->kind == ExprKind::PRIMITIVE && expr->primitive != Primitive::UNKNOWN) {
      print("primitive@" + primitiveToString(expr->primitive));
    }

    if (!expr->value.empty()) print("value: " + expr->value);

    if (expr->left) {
      print("left:");
      indent_level++;
      print_expr(expr->left);
      indent_level--;
    }

    if (expr->right) {
      print("right:");
      indent_level++;
      print_expr(expr->right);
      indent_level--;
    }

    if (expr->object) {
      print("object:");
      indent_level++;
      print_expr(expr->object);
      indent_level--;
    }

    if (expr->binOp != BinaryOp::UNK) {
      print("binaryOp: " + binaryOpToString(expr->binOp));
    }

    if (expr->unOp != UnaryOp::UNK) {
      print("unaryOp: " + unaryOpToString(expr->unOp));
    }

    if (expr->callee) {
      print("callee:");
      indent_level++;
      print_expr(expr->callee);
      indent_level--;
    }

    if (expr->exprValue) {
      print("exprValue:");
      indent_level++;
      print_expr(expr->exprValue);
      indent_level--;
    }

    if (!expr->args.empty()) {
      print("args:");
      indent_level++;
      for (auto* arg : expr->args) {
        print_expr(arg);
      }
      indent_level--;
    }

    if (!expr->elements.empty()) {
      print("elements:");
      indent_level++;
      for (auto* elem : expr->elements) {
        print_expr(elem);
      }
      indent_level--;
    }

    if (!expr->keys.empty()) {
      print("keys:");
      indent_level++;
      for (auto* key : expr->keys) {
        print_expr(key);
      }
      indent_level--;
    }

    if (!expr->values.empty()) {
      print("values:");
      indent_level++;
      for (auto* value : expr->values) {
        print_expr(value);
      }
      indent_level--;
    }

    if (!expr->genericType.empty()) {
      print("genericType:");
      indent_level++;
      for (auto* generic : expr->genericType) {
        print_type(generic);
      }
      indent_level--;
    }

    if (expr->block) {
      print_stmt(expr->block);
    }

    if (expr->isSignedInteger) print("isSignedInteger: true");

    debug_location(expr->locations);

    indent_level--;
  }

  void ASTPrinter::print_type(const Type* type) {
    if (!type) {
      std::cout << "<type-nullptr>\n";
      return;
    }

    print("type@" + typeKindToString(type->kind));

    indent_level++;
    if (type->kind == TypeKind::PRIMITIVE) {
      print("type@" + primitiveToString(type->primitive));
    }

    if (!type->name.empty())
      print("name: " + type->name);

    if (!type->genericType.empty()) {
      print("genericType[" + std::to_string(type->genericType.size()) + "]");
      indent_level++;
      for (auto* ty : type->genericType) {
        print_type(ty);
      }
      indent_level--;
    }

    if (type->object) {
      print("object:");
      indent_level++;
      print_type(type->object);
      indent_level--;
    }

    if (type->isSignedInteger) print("isSignedInteger: true");
    if (type->isPointer) print("isPointer: true");
    if (type->isReference) print("isReference: true");
    if (type->isArray) print("isArray: true");
    if (type->isStruct) print("isStruct: true");
    if (type->isEnum) print("isEnum: true");
    if (type->isNullable) print("isNullable: true");

    debug_location(type->locations);

    indent_level--;
  }

  void ASTPrinter::debug_location(SourceLocation location) {
    // print("location " + location.path + ":" + std::to_string(location.line) + ":" + std::to_string(location.column));
  }

}
