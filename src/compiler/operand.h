#pragma once

#include <string>

enum class BinaryOp {
  ADD,
  SUB,
  DIV,
  MOD,
  MUL,
  POW,
  LE,
  GE,
  LT,
  GT,
  AND,
  OR,
  EQ,
  NEQ,
  UNK,
};

enum class UnaryOp {
  PLUS,
  MINUS,
  PTR,
  REF,
  UNK,
};

inline std::string binaryOpToString(BinaryOp binop) {
  switch (binop) {
    case BinaryOp::ADD: return "Add";
    case BinaryOp::SUB: return "Sub";
    case BinaryOp::DIV: return "Div";
    case BinaryOp::MUL: return "Mul";
    case BinaryOp::MOD: return "Mod";
    case BinaryOp::POW: return "Pow";
    case BinaryOp::LE: return "LE";
    case BinaryOp::GE: return "GE";
    case BinaryOp::LT: return "LT";
    case BinaryOp::GT: return "GT";
    case BinaryOp::AND: return "And";
    case BinaryOp::OR: return "Or";
    default: return "<unknown-op>";
  }
}

inline std::string unaryOpToString(UnaryOp unop) {
  switch (unop) {
    case UnaryOp::PLUS: return "Plus";
    case UnaryOp::MINUS: return "Minus";
    case UnaryOp::PTR: return "Ptr";
    case UnaryOp::REF: return "Ref";
    default: return "<unknown-op>";
  }
}

inline BinaryOp stringToBinaryOp(std::string op) {
  if (op == "+") return BinaryOp::ADD;
  if (op == "-") return BinaryOp::SUB;
  if (op == "/") return BinaryOp::DIV;
  if (op == "*") return BinaryOp::MUL;
  if (op == "^") return BinaryOp::POW;
  if (op == "%") return BinaryOp::MOD;
  if (op == "<") return BinaryOp::LE;
  if (op == ">") return BinaryOp::GE;
  if (op == "<=") return BinaryOp::LT;
  if (op == ">=") return BinaryOp::GT;
  if (op == "&&") return BinaryOp::AND;
  if (op == "||") return BinaryOp::OR;
  if (op == "==") return BinaryOp::EQ;
  if (op == "!=") return BinaryOp::NEQ;

  return BinaryOp::UNK;
}

inline UnaryOp stringToUnaryOp(std::string op) {
  if (op == "+") return UnaryOp::PLUS;
  if (op == "-") return UnaryOp::MINUS;
  if (op == "&") return UnaryOp::REF;
  if (op == "*") return UnaryOp::PTR;

  return UnaryOp::UNK;
}
