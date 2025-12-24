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

inline const char* TokenKindToString(TokenKind kind) {
  switch (kind) {
    case TokenKind::END_OF_FILE:   return "ENDOFFILE";
    case TokenKind::NONE:        return "NONE";
    case TokenKind::UNKNOWN:     return "UNKNOWN";
    case TokenKind::INDENT:      return "INDENT";

    case TokenKind::STRING:      return "STRING";
    case TokenKind::STRING_LITERAL:      return "STRING_LITERAL";
    case TokenKind::CHAR:        return "CHAR";
    case TokenKind::CHAR_LITERAL:        return "CHAR_LITERAL";
    case TokenKind::NUMBER:      return "NUMBER";
    case TokenKind::BOOL:        return "bool";
    case TokenKind::SCNOTATION:  return "SCIENTIFIC_NOTATION";
    case TokenKind::HEX:         return "HEX";
    case TokenKind::IDENTIFIER:  return "IDENTIFIER";

    case TokenKind::I8:          return "I8";
    case TokenKind::I16:         return "I16";
    case TokenKind::I32:         return "I32";
    case TokenKind::I64:         return "I64";
    case TokenKind::I128:        return "I128";

    case TokenKind::U8:          return "U8";
    case TokenKind::U16:         return "U16";
    case TokenKind::U32:         return "U32";
    case TokenKind::U64:         return "U64";
    case TokenKind::U128:        return "U128";

    case TokenKind::F32:         return "F32";
    case TokenKind::F64:         return "F64";
    
    case TokenKind::VOID:        return "VOID";
    case TokenKind::AUTO:        return "AUTO";
    case TokenKind::ANY:         return "ANY";

    case TokenKind::TRUE:        return "TRUE";
    case TokenKind::FALSE:       return "FALSE";

    case TokenKind::INT:         return "INT";
    case TokenKind::LONG:        return "LONG";
    case TokenKind::FLOAT:       return "FLOAT";
    case TokenKind::DOUBLE:      return "DOUBLE";

    case TokenKind::FUNCTION:    return "FUNCTION";
    case TokenKind::RETURN:      return "RETURN";
    case TokenKind::IF:          return "IF";
    case TokenKind::ELSE:        return "ELSE";
    case TokenKind::WHILE:       return "WHILE";
    case TokenKind::SWITCH:      return "SWITCH";
    case TokenKind::CASE:        return "CASE";
    case TokenKind::DEFAULT:     return "DEFAULT";
    case TokenKind::BREAK:       return "BREAK";
    case TokenKind::CONTINUE:    return "CONTINUE";
    case TokenKind::SUMMON:      return "SUMMON";
    case TokenKind::FOR:         return "FOR";
    case TokenKind::STRUCT:      return "STRUCT";
    case TokenKind::ENUM:        return "ENUM";
    case TokenKind::LET:         return "LET";
    case TokenKind::ROOT:        return "ROOT";
    case TokenKind::SUPER:       return "SUPER";
    case TokenKind::PUBLIC:      return "PUBLIC";
    case TokenKind::STATIC:      return "STATIC";
    case TokenKind::CONST:       return "CONST";

    case TokenKind::LPARENT:     return "LPARENT";
    case TokenKind::RPARENT:     return "RPARENT";
    case TokenKind::LBRACKET:    return "LBRACKET";
    case TokenKind::RBRACKET:    return "RBRACKET";
    case TokenKind::LBRACE:      return "LBRACE";
    case TokenKind::RBRACE:      return "RBRACE";
    case TokenKind::EQUAL:       return "EQUAL";
    case TokenKind::AMPER:       return "AMPER";
    case TokenKind::PIPE:        return "PIPE";
    case TokenKind::EXCLAMATION: return "EXCLAMATION";
    case TokenKind::TILDE:       return "TILDE";
    case TokenKind::AT:          return "AT";
    case TokenKind::DOLLAR:      return "DOLLAR";
    case TokenKind::PERCENT:     return "PERCENT";
    case TokenKind::POWER:       return "POWER";
    case TokenKind::STAR:        return "STAR";
    case TokenKind::MINUS:       return "MINUS";
    case TokenKind::PLUS:        return "PLUS";
    case TokenKind::LESS:        return "LESS";
    case TokenKind::GREATER:     return "GREATER";
    case TokenKind::COMMA:       return "COMMA";
    case TokenKind::DOT:         return "DOT";
    case TokenKind::SLASH:       return "SLASH";
    case TokenKind::COLON:       return "COLON";
    case TokenKind::SEMICOLON:   return "SEMICOLON";
    case TokenKind::QUESTION:    return "QUESTION";

    case TokenKind::PLUS_EQUAL:     return "PLUS EQUAL";
    case TokenKind::MINUS_EQUAL:    return "MINUS EQUAL";
    case TokenKind::STAR_EQUAL:     return "STAR EQUAL";
    case TokenKind::SLASH_EQUAL:    return "SLASH EQUAL";
    case TokenKind::POWER_EQUAL:    return "POWER EQUAL";
    case TokenKind::PERCENT_EQUAL:  return "PERCENT EQUAL";
    case TokenKind::LESS_EQUAL:     return "LESS EQUAL";
    case TokenKind::GREATER_EQUAL:  return "GREATER";
    case TokenKind::IS_EQUAL:       return "IS EQUAL";
    case TokenKind::NOT_EQUAL:      return "NOT EQUAL";
    case TokenKind::AND:            return "AND";
    case TokenKind::OR:             return "OR";
    case TokenKind::ARROW:          return "ARROW";
    case TokenKind::DOUBLE_COLON:   return "DOUBLE_COLON";
  }

  return "UNKNOWN_TOKEN_KIND";
}

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
    
    case TokenKind::VOID:        return "void";
    case TokenKind::AUTO:        return "auto";
    case TokenKind::ANY:         return "any";
    
    case TRUE: return "true";
    case FALSE: return "false";

    case INT: return "int";
    case LONG: return "long";
    case FLOAT: return "float";
    case DOUBLE: return "double";

    case FUNCTION: return "fn";
    case RETURN: return "return";
    case IF: return "if";
    case ELSE: return "else";
    case WHILE: return "while";
    case SWITCH: return "switch";
    case CASE: return "case";
    case DEFAULT: return "default";
    case BREAK: return "break";
    case CONTINUE: return "continue";
    case SUMMON: return "summon";
    case FOR: return "for";
    case STRUCT: return "struct";
    case ENUM: return "enum";
    case LET: return "let";
    case PUBLIC:      return "public";
    case STATIC:      return "static";
    case CONST:       return "const";
    
    case LPARENT: return "(";
    case RPARENT: return ")";
    case LBRACKET: return "[";
    case RBRACKET: return "]";
    case LBRACE: return "{";
    case RBRACE: return "}";
    case EQUAL: return "=";
    case AMPER: return "&";
    case PIPE: return "|";
    case EXCLAMATION: return "!";
    case TILDE: return "~";
    case AT: return "@";
    case DOLLAR: return "$";
    case PERCENT: return "%";
    case POWER: return "^";
    case STAR: return "*";
    case MINUS: return "-";
    case PLUS: return "+";
    case LESS: return "<";
    case GREATER: return ">";
    case COMMA: return ",";
    case DOT: return ".";
    case SLASH: return "/";
    case COLON: return ":";
    case SEMICOLON: return ";";
    case QUESTION: return "?";

    case PLUS_EQUAL: return "+=";
    case MINUS_EQUAL: return "-=";
    case STAR_EQUAL: return "*=";
    case SLASH_EQUAL: return "/=";
    case POWER_EQUAL: return "^=";
    case PERCENT_EQUAL: return "%=";
    case LESS_EQUAL: return "<=";
    case GREATER_EQUAL: return ">=";
    case IS_EQUAL: return "==";
    case NOT_EQUAL: return "!=";
    case AND: return "&&";
    case OR: return "||";

    case ROOT: return "root";
    case SUPER: return "super";
    case ARROW: return "->";
    case TokenKind::DOUBLE_COLON:   return "::";
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

  void debug() {
    std::cout << "Kind: " << TokenKindToString(kind);
    std::cout << ", Value: " << value << "\n";
  }
};
