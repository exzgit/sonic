#pragma once

#include <iostream>

enum TokenKind {
  END_OF_FILE,
  NONE,
  UNKNOWN,
  INDENT,

  BOOL,
  STRING,
  CHAR,
  STRING_LITERAL,
  CHAR_LITERAL,
  NUMBER,
  SCNOTATION,
  HEX,
  IDENTIFIER,
  TRUE,
  FALSE,
  INT,
  LONG,
  FLOAT,
  DOUBLE,
  FUNCTION,
  RETURN,
  IF,
  ELSE,
  WHILE,
  SWITCH,
  CASE,
  DEFAULT,
  BREAK,
  CONTINUE,
  SUMMON,
  FOR,
  STRUCT,
  ENUM,
  LET,
  PUBLIC,
  STATIC,
  CONST,
  IN,

  I8,
  I16,
  I32,
  I64,
  I128,

  U8,
  U16,
  U32,
  U64,
  U128,

  F32,
  F64,

  VOID,
  AUTO,
  ANY,

  LPARENT,
  RPARENT,
  LBRACKET,
  RBRACKET,
  LBRACE,
  RBRACE,
  EQUAL,
  AMPER,
  PIPE,
  EXCLAMATION,
  TILDE,
  AT,
  DOLLAR,
  PERCENT,
  POWER,
  STAR,
  MINUS,
  PLUS,
  LESS,
  GREATER,
  COMMA,
  DOT,
  SLASH,
  COLON,
  SEMICOLON,
  QUESTION,

  PLUS_EQUAL,
  MINUS_EQUAL,
  STAR_EQUAL,
  SLASH_EQUAL,
  POWER_EQUAL,
  PERCENT_EQUAL,
  LESS_EQUAL,
  GREATER_EQUAL,
  IS_EQUAL,
  NOT_EQUAL,
  DOUBLE_COLON,
  AND,
  OR,
  ROOT,
  SUPER,
  ARROW
};

inline std::string TokenKindToValue(TokenKind kind) {
  switch (kind) {
    case END_OF_FILE: return "EOF";
    case NONE: return "none";
    case UNKNOWN: return "unknown";
    case INDENT: return "indent";

    case STRING: return "string";
    case CHAR: return "char";
    case STRING_LITERAL:      return "string";
    case CHAR_LITERAL:        return "char";
    case NUMBER: return "number";
    case BOOL: return "bool";
    case SCNOTATION: return "scientific notation";
    case HEX: return "hex";
    case IDENTIFIER: return "identifier";

    case I8:          return "i8";
    case I16:         return "i16";
    case I32:         return "i32";
    case I64:         return "i64";
    case I128:        return "i128";

    case U8:          return "u8";
    case U16:         return "u16";
    case U32:         return "u32";
    case U64:         return "u64";
    case U128:        return "u128";
    
    case F32:         return "f32";
    case F64:         return "i64";
    
    case VOID:        return "void";
    case AUTO:        return "auto";
    case ANY:         return "any";
    
    case TRUE:        return "true";
    case FALSE:       return "false";

    case INT:         return "int";
    case LONG:        return "long";
    case FLOAT:       return "float";
    case DOUBLE:      return "double";

    case FUNCTION:    return "fn";
    case RETURN:      return "return";
    case IF:          return "if";
    case ELSE:        return "else";
    case WHILE:       return "while";
    case SWITCH:      return "switch";
    case CASE:        return "case";
    case DEFAULT:     return "default";
    case BREAK:       return "break";
    case CONTINUE:    return "continue";
    case SUMMON:      return "summon";
    case FOR:         return "for";
    case STRUCT:      return "struct";
    case ENUM:        return "enum";
    case LET:         return "let";
    case PUBLIC:      return "public";
    case STATIC:      return "static";
    case CONST:       return "const";
    case IN:          return "in";
    
    case LPARENT:     return "(";
    case RPARENT:     return ")";
    case LBRACKET:    return "[";
    case RBRACKET:    return "]";
    case LBRACE:      return "{";
    case RBRACE:      return "}";
    case EQUAL:       return "=";
    case AMPER:       return "&";
    case PIPE:        return "|";
    case EXCLAMATION: return "!";
    case TILDE:       return "~";
    case AT:          return "@";
    case DOLLAR:      return "$";
    case PERCENT:     return "%";
    case POWER:       return "^";
    case STAR:        return "*";
    case MINUS:       return "-";
    case PLUS:        return "+";
    case LESS:        return "<";
    case GREATER:     return ">";
    case COMMA:       return ",";
    case DOT:         return ".";
    case SLASH:       return "/";
    case COLON:       return ":";
    case SEMICOLON:   return ";";
    case QUESTION:    return "?";

    case PLUS_EQUAL:    return "+=";
    case MINUS_EQUAL:   return "-=";
    case STAR_EQUAL:    return "*=";
    case SLASH_EQUAL:   return "/=";
    case POWER_EQUAL:   return "^=";
    case PERCENT_EQUAL: return "%=";
    case LESS_EQUAL:    return "<=";
    case GREATER_EQUAL: return ">=";
    case IS_EQUAL:      return "==";
    case NOT_EQUAL:     return "!=";
    case AND:           return "&&";
    case OR:            return "||";

    case ROOT:          return "root";
    case SUPER:         return "super";
    case ARROW:         return "->";
    case DOUBLE_COLON:  return "::";
  }

  return "unknown";
}

struct Token {
  TokenKind kind;
  std::string value;

  size_t line;
  size_t column;
  size_t start;
  size_t end;

  std::string files;
  std::string source;
};
