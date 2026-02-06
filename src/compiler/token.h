#pragma once

// c++ library
#include <string>
#include <unordered_map>

// local headers
#include "source.h"

enum class TokenType {
  ENDOFFILE,

  IDENT,
  NUMBER,
  TRUE,
  FALSE,
  STRLIT,
  CHARLIT,
  NONE,

  FUNCT,
  RETURN,
  LET,
  PUBLIC,
  CONST,
  STATIC,
  EXTERN,
  IF,
  ELSE,
  WHILE,
  FOR,
  BREAK,
  CONTINUE,
  DEFAULT,
  CASE,
  SWITCH,
  STRUCT,
  ENUM,
  NEW,
  I32,
  I64,
  I128,
  F32,
  F64,
  VOID,
  BOOL,
  STR,
  CHAR,
  ANY,
  IMPORT,
  USE,
  SELF,
  MODULE,
  ALIAS,
  IN,
  TRY,
  CATCH,
  FINALLY,

  LEFTPAREN,
  RIGHTPAREN,
  LEFTBRACE,
  RIGHTBRACE,
  SEMICOLON,
  COLON,
  COLON_COLON,
  COMMA,
  DOT,
  LEFTBRACKET,
  RIGHTBRACKET,
  LESS,
  GREATER,
  LESS_EQUAL,
  GREATER_EQUAL,
  EQUAL,
  NOT_EQUAL,
  IS_EQUAL,
  ARROW,
  PLUS,
  MINUS,
  STAR,
  DIV,
  PERCENT,
  POWER,
  PLUS_EQUAL,
  MINUS_EQUAL,
  STAR_EQUAL,
  DIV_EQUAL,
  PERCENT_EQUAL,
  POWER_EQUAL,
  AND,
  OR,
  XOR,
  EXCLAMATION,
  QUESTION,
  AT,
  HASH,
  DOLLAR,
  AMPERSAND,
  PIPE,
  VARIADIC,
  RANGE,

  INVALID,
  UNKNOWN
};

inline std::string tokenTypeToValue(TokenType t) {
  switch (t) {
    // special
    case TokenType::ENDOFFILE: return "<eof>";

    // literals / identifiers
    case TokenType::IDENT:   return "identifier";
    case TokenType::NUMBER:  return "digit";
    case TokenType::STRLIT:  return "string";
    case TokenType::CHARLIT: return "char";
    case TokenType::TRUE:    return "true";
    case TokenType::FALSE:   return "false";
    case TokenType::NONE:    return "none";

    // keywords
    case TokenType::FUNCT:    return "func";
    case TokenType::RETURN:   return "return";
    case TokenType::LET:      return "let";
    case TokenType::PUBLIC:   return "public";
    case TokenType::CONST:    return "const";
    case TokenType::STATIC:   return "static";
    case TokenType::EXTERN:   return "extern";
    case TokenType::IF:       return "if";
    case TokenType::ELSE:     return "else";
    case TokenType::WHILE:    return "while";
    case TokenType::FOR:      return "for";
    case TokenType::BREAK:    return "break";
    case TokenType::CONTINUE: return "continue";
    case TokenType::DEFAULT:  return "default";
    case TokenType::CASE:     return "case";
    case TokenType::SWITCH:   return "switch";
    case TokenType::STRUCT:   return "struct";
    case TokenType::ENUM:     return "enum";
    case TokenType::NEW:      return "new";
    case TokenType::SELF:     return "self";
    case TokenType::MODULE:   return "module";
    case TokenType::ALIAS:    return "as";
    case TokenType::IN:       return "in";
    case TokenType::TRY:       return "try";
    case TokenType::CATCH:       return "catch";
    case TokenType::FINALLY:       return "finally";


    // types
    case TokenType::I32:  return "i32";
    case TokenType::I64:  return "i64";
    case TokenType::I128: return "i128";
    case TokenType::F32:  return "f32";
    case TokenType::F64:  return "f64";
    case TokenType::VOID: return "void";
    case TokenType::BOOL: return "bool";
    case TokenType::STR:  return "str";
    case TokenType::CHAR: return "char";
    case TokenType::ANY:  return "any";

    // imports
    case TokenType::IMPORT: return "import";
    case TokenType::USE:    return "use";

    // punctuation / operators
    case TokenType::LEFTPAREN:    return "(";
    case TokenType::RIGHTPAREN:   return ")";
    case TokenType::LEFTBRACE:    return "{";
    case TokenType::RIGHTBRACE:   return "}";
    case TokenType::LEFTBRACKET:  return "[";
    case TokenType::RIGHTBRACKET: return "]";
    case TokenType::SEMICOLON:    return ";";
    case TokenType::COLON:        return ":";
    case TokenType::COLON_COLON:  return "::";
    case TokenType::COMMA:        return ",";
    case TokenType::DOT:          return ".";
    case TokenType::LESS:         return "<";
    case TokenType::GREATER:      return ">";
    case TokenType::LESS_EQUAL:   return "<=";
    case TokenType::GREATER_EQUAL:return ">=";
    case TokenType::EQUAL:        return "=";
    case TokenType::NOT_EQUAL:    return "!=";
    case TokenType::IS_EQUAL:     return "==";
    case TokenType::ARROW:        return "->";

    case TokenType::PLUS:          return "+";
    case TokenType::MINUS:         return "-";
    case TokenType::STAR:          return "*";
    case TokenType::DIV:           return "/";
    case TokenType::PERCENT:       return "%";
    case TokenType::POWER:         return "**";

    case TokenType::PLUS_EQUAL:    return "+=";
    case TokenType::MINUS_EQUAL:   return "-=";
    case TokenType::STAR_EQUAL:    return "*=";
    case TokenType::DIV_EQUAL:     return "/=";
    case TokenType::PERCENT_EQUAL: return "%=";
    case TokenType::POWER_EQUAL:   return "**=";

    case TokenType::AND:         return "&&";
    case TokenType::OR:          return "||";
    case TokenType::XOR:         return "^";
    case TokenType::EXCLAMATION: return "!";
    case TokenType::QUESTION:    return "?";
    case TokenType::AT:          return "@";
    case TokenType::HASH:        return "#";
    case TokenType::DOLLAR:      return "$";
    case TokenType::AMPERSAND:   return "&";
    case TokenType::PIPE:        return "|";
    case TokenType::VARIADIC:    return "...";
    case TokenType::RANGE:       return "..";

    // errors
    case TokenType::INVALID: return "<invalid>";
    case TokenType::UNKNOWN: return "<unknown>";
  }

  return "<unknown>";
}

const std::unordered_map<std::string, TokenType> keywords = {
  {"func", TokenType::FUNCT},
  {"return", TokenType::RETURN},
  {"let", TokenType::LET},
  {"public", TokenType::PUBLIC},
  {"const", TokenType::CONST},
  {"static", TokenType::STATIC},
  {"extern", TokenType::EXTERN},
  {"if", TokenType::IF},
  {"else", TokenType::ELSE},
  {"while", TokenType::WHILE},
  {"for", TokenType::FOR},
  {"break", TokenType::BREAK},
  {"continue", TokenType::CONTINUE},
  {"default", TokenType::DEFAULT},
  {"case", TokenType::CASE},
  {"switch", TokenType::SWITCH},
  {"struct", TokenType::STRUCT},
  {"enum", TokenType::ENUM},
  {"new", TokenType::NEW},
  {"i32", TokenType::I32},
  {"i64", TokenType::I64},
  {"i128", TokenType::I128},
  {"f32", TokenType::F32},
  {"f64", TokenType::F64},
  {"void", TokenType::VOID},
  {"bool", TokenType::BOOL},
  {"true", TokenType::TRUE},
  {"false", TokenType::FALSE},
  {"none", TokenType::NONE},
  {"str", TokenType::STR},
  {"char", TokenType::CHAR},
  {"any", TokenType::ANY},
  {"import", TokenType::IMPORT},
  {"use", TokenType::USE},
  {"self", TokenType::SELF},
  {"module", TokenType::MODULE},
  {"as", TokenType::ALIAS},
  {"in", TokenType::IN},
  {"try", TokenType::TRY},
  {"catch", TokenType::CATCH},
  {"finally", TokenType::FINALLY},
};

const std::unordered_map<std::string, TokenType> punctuation = {
  // grouping
  {"(",  TokenType::LEFTPAREN},
  {")",  TokenType::RIGHTPAREN},
  {"{",  TokenType::LEFTBRACE},
  {"}",  TokenType::RIGHTBRACE},
  {"[",  TokenType::LEFTBRACKET},
  {"]",  TokenType::RIGHTBRACKET},

  // separators
  {";",  TokenType::SEMICOLON},
  {",",  TokenType::COMMA},
  {".",  TokenType::DOT},
  {"..", TokenType::RANGE},
  {"...",TokenType::VARIADIC},

  // comparison
  {"<=", TokenType::LESS_EQUAL},
  {">=", TokenType::GREATER_EQUAL},
  {"<",  TokenType::LESS},
  {">",  TokenType::GREATER},
  {"==", TokenType::IS_EQUAL},
  {"!=", TokenType::NOT_EQUAL},
  {"=",  TokenType::EQUAL},

  // arrows
  {"->", TokenType::ARROW},

  // math
  {"^=",TokenType::POWER_EQUAL},
  {"^", TokenType::POWER},
  {"+=", TokenType::PLUS_EQUAL},
  {"-=", TokenType::MINUS_EQUAL},
  {"*=", TokenType::STAR_EQUAL},
  {"/=", TokenType::DIV_EQUAL},
  {"%=", TokenType::PERCENT_EQUAL},
  {"+",  TokenType::PLUS},
  {"-",  TokenType::MINUS},
  {"*",  TokenType::STAR},
  {"/",  TokenType::DIV},
  {"%",  TokenType::PERCENT},

  // logical / bitwise
  {"&&", TokenType::AND},
  {"||", TokenType::OR},
  {"^",  TokenType::XOR},
  {"&",  TokenType::AMPERSAND},
  {"|",  TokenType::PIPE},

  // misc
  {"!",  TokenType::EXCLAMATION},
  {"?",  TokenType::QUESTION},
  {"@",  TokenType::AT},
  {"#",  TokenType::HASH},
  {"$",  TokenType::DOLLAR},
  {"::", TokenType::COLON_COLON},
  {":",  TokenType::COLON},
};


struct Token {
  TokenType type = TokenType::UNKNOWN;
  std::string value;
  std::string raw;

  SourceLocation location = {"", "", 0, 0, 0, 0};

  Token() = default;
  explicit Token(TokenType type, const std::string& value, const std::string& raw, const SourceLocation& location)
    : type(type), value(value), raw(raw), location(location) {}

  Token clone() const {
    return Token(type, value, raw, location);
  }
};
