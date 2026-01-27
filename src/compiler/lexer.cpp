#include "lexer.h"
#include "source.h"
#include "token.h"
#include <cctype>
#include <cwctype>

namespace sonic::frontend {

Lexer::Lexer(const std::string& input, const std::string& filename)
    : input(input), filename(filename), index(0), line(1), column(1) {
  lines = split_lines(input);
  lines.push_back(""); // sentinel line
  this->input += " \n";
}

Token Lexer::next_token() {
  skipWhitespace();

  if (peek() == '\0')
    return Token(TokenType::ENDOFFILE, "", "\\0", SourceLocation(filename, lines[line - 1], "", line, column, index));


  if (peek() == '/') {
    advance();
    if (peek() == '/') {
      skipComment();
    } else if (peek() == '*') {
      skipMultiComment();
    } else {
      column--;
      index--;
    }
  }

  skipWhitespace();
  Token tok = Token(TokenType::INVALID, peek() + "", "\\0", SourceLocation(filename, lines[line - 1], "", line, column, index));

  if (isdigit(peek())) {
    tok = getTokenNumber();
  }
  else if (isalpha(peek()) || peek() == '_') {
    tok = getTokenKeyword();
  }
  else if (peek() == '"') {
    tok = getTokenString();
  }
  else if (peek() == '\'') {
    tok = getTokenChar();
  }
  else if (ispunct(peek())) {
    tok = getTokenPunct();
  }

  skipWhitespace();

  if (peek() == '/') {
    advance();
    if (peek() == '/') {
      skipComment();
    } else if (peek() == '*') {
      skipMultiComment();
    } else {
      column--;
      index--;
    }
  }

  skipWhitespace();

  return tok;
}

Token Lexer::getTokenNumber() {
  std::string value;
  std::string raw;
  auto location = SourceLocation(filename, lines[line - 1], raw, line, column, index);
  location.start = column;

  while (std::isdigit(peek()) || peek() == '_') {
    raw += peek();
    if (peek() != '_')
      value += peek();
    else {
      advance();

      if (!std::isdigit(peek())) {
        location.end = column;
        location.column = column;
        location.raw_value = raw;

        diag->report({
          ErrorType::INVALID,
          Severity::ERROR,
          location,
          "Invalid number format",
          "expected digit after underscore",
          "try this " + raw + "\033[32m0\033[0m"
        });

        if (peek() != '.') {
          return Token(TokenType::NUMBER, value, raw, location);
        } else break;
      }
      continue;
    }
    advance();
  }

  if (peek() == '.') {
    advance();

    if (peek() == '.') {
      index--;
      column--;
      return Token(TokenType::NUMBER, value, raw, location);
    }

    raw += '.';
    value += '.';

    if (!std::isdigit(peek())) {
      location.end = column;
      location.column = column;
      location.raw_value = raw;

      diag->report({
        ErrorType::INVALID,
        Severity::ERROR,
        location,
        "Invalid number format",
        "expected digit after dot",
        "try this " + raw + "\033[32m0\033[0m"
      });
      return Token(TokenType::NUMBER, value, raw, location);
    }

    while (std::isdigit(peek()) || peek() == '_') {
      raw += peek();
      if (peek() != '_')
        value += peek();
      else {
        advance();

        if (!std::isdigit(peek())) {
          location.end = column;
          location.column = column;
          location.raw_value = raw;

          diag->report({
            ErrorType::INVALID,
            Severity::ERROR,
            location,
            "Invalid number format",
            "expected digit after underscore",
            "try this " + raw + "\033[32m0\033[0m"
          });
          return Token(TokenType::NUMBER, value, raw, location);
        }
        continue;
      }
      advance();
    }
  }

  location.end = column;
  location.raw_value = raw;

  return Token(TokenType::NUMBER, value, raw, location);
}

Token Lexer::getTokenString() {
  std::string value;
  std::string raw = std::string(1, peek());
  auto location = SourceLocation(filename, lines[line - 1], raw, line, column, index);
  int starts = column;
  location.start = column;

  advance();

  while (peek() != '"' && peek() != '\n' && peek() != '\0') {
    if (peek() == '\\') {
      advance();
      if (peek() == 'n') {
        value += '\n';
        raw += "\\n";
      } else if (peek() == 't') {
        value += '\t';
        raw += "\\t";
      }
      #if defined(_WIN32)
        else if (peek() == 'r') {
          value += '\r';
          raw += "\\r";
        }
      #endif
      else if (peek() == '0') {
        value += '\0';
        raw += "\\0";
      } else if (peek() == '\\') {
        value += '\\';
        raw += "\\\\";
      } else if (peek() == '"') {
        value += '"';
        raw += "\\\"";
      } else {
        location.start = column;
        location.end = column;
        location.column = column;

        diag->report({
          ErrorType::INVALID,
          Severity::ERROR,
          location,
          "Invalid escape sequence",
          "",
          "try using it \"" + value + "\033[32m\\\\\"\033[0m"
        });
      }
    } else if (peek() == '\n' || peek() == '\t') {
      std::string hint;

      if (peek() == '\n') {
        hint = "try using it \"" + value + "\033[32m\"\033[0m";
      } else if (peek() == '\t') {
        hint = "try using it \"" + value + "\033[32m\\\\t\"\033[0m";
      }

      location.start = column;
      location.end = column;
      location.column = column;

      diag->report({
        ErrorType::INVALID,
        Severity::ERROR,
        location,
        "Invalid character in string literal",
        "",
        "try using it \"" + value + hint + "\"\033[0m"
      });
    }
    #if defined(_WIN32)
    else if (peek() == '\r') {
      location.start = column;
      location.end = column;
      location.column = column;

      diag->report({
        ErrorType::INVALID,
        Severity::ERROR,
        location,
        "Invalid character in string literal",
        "",
        "try using it \"" + value + "\033[32m\\\\r\"\033[0m"
      });
    }
    #endif
    else {
      value += peek();
      raw += peek();
    }
    advance();
  }

  if (peek() == '\n') {
    location.start = column;
    location.column = column;
    location.end = column;

    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "unterminated string literal",
      "missing closing '\"'",
      "try using it \"" + value + "\033[32m\"\033[0m"
    });
    return Token(TokenType::STRLIT, value, raw, location);
  } else if (peek() == '\0') {
    location.start = column;
    location.column = column;
    location.end = column;
    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "unterminated string literal",
      "missing closing '\"'",
      "try using it \"" + value + "\033[32m\"\033[0m"
    });

    return Token(TokenType::STRLIT, value, raw, location);
  } else if (peek() != '"') {
    location.start = column;
    location.column = column;
    location.end = column;
    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "unterminated string literal",
      "missing closing '\"'",
      "try using it \"" + value + "\033[32m\"\033[0m"
    });
    return Token(TokenType::STRLIT, value, raw, location);
  }

  raw += peek();
  advance();

  location.end = column;
  location.start = starts;
  return Token(TokenType::STRLIT, value, raw, location);
}

Token Lexer::getTokenChar() {
  std::string value;
  std::string raw = std::string(1, peek());
  auto location = SourceLocation(filename, lines[line - 1], raw, line, column, index);
  int starts = column;
  location.start = column;

  advance();

  if (peek() == '\'') {
    location.start = column;
    location.end = column;
    location.column = column;

    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "invalid character literal",
      "expected alphabet, numeric, escape sequence, or punctuation",
      "example \'\033[32mC\033[0m\'"
    });

    advance();

    return Token(TokenType::CHARLIT, value, raw, location);
  } else if (peek() == '\n' || peek() == '\t' || peek() == '\0'
    #if defined(_WIN32)
    || peek() == '\r'
    #endif
  ) {
    location.start = column;
    location.end = column;
    location.column = column;

    std::string hint;
    if (peek() == '\n') {
      hint = "try using it '\033[32m\\\\n\033[0m'";
    } else if (peek() == '\t') {
      hint = "try using it '\033[32m\\\\t\033[0m'";
    } else if (peek() == '\0') {
      hint = "try using it '\033[32m\\\\0\033[0m'";
    } else if (peek() == '\\') {
      hint = "try using it '\033[32m\\\\\\\\\033[0m'";
    }
    #if defined(_WIN32)
    else if (peek() == '\r') {
      hint = "try using it '\033[32m\\\\r\033[0m'";
    }
    #endif

    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "invalid escape sequence",
      "",
      hint
    });

    advance();

    return Token(TokenType::CHARLIT, value, raw, location);
  }

  if (peek() == '\\') {
    advance();
    if (peek() == 'n') {
      value += '\n';
      raw += "\\n";
      advance();
    } else if (peek() == 't') {
      value += '\t';
      raw += "\\t";
      advance();
    }
    #if defined(_WIN32)
    else if (peek() == 'r') {
      value += '\r';
      raw += "\\r";
      advance();
    }
    #endif
    else if (peek() == '0') {
      value += '\0';
      raw += "\\0";
      advance();
    } else if (peek() == '\\') {
      value += '\\';
      raw += "\\\\";
      advance();
    } else if (peek() == '\'') {
      value += '\'';
      raw += "\\'";
      advance();
    } else {
      location.start = column;
      location.end = column;
      location.column = column;

      diag->report({
        ErrorType::INVALID,
        Severity::ERROR,
        location,
        "invalid escape sequence",
        "",
        "try using it \'\033[32m\\\\\033[0m\'"
      });
      return Token(TokenType::INVALID, value, raw, location);
    }
  } else {
    value += peek();
    raw += peek();
    advance();
  }

  if (peek() != '\'') {
    location.start = column;
    location.end = column;
    location.column = column;

    diag->report({
      ErrorType::INVALID,
      Severity::ERROR,
      location,
      "unterminated character literal",
      "missing closing `'`",
      "try using it '" + raw + "\033[32m'\033[0m"
    });
    return Token(TokenType::INVALID, value, raw, location);
  }

  raw += peek();
  advance();

  location.end = column;
  location.start = starts;

  return Token(TokenType::CHARLIT, value, raw, location);
}

Token Lexer::getTokenKeyword() {
  std::string value;
  std::string raw;
  auto location = SourceLocation(filename, lines[line - 1], value, line, column, index);
  location.start = column;

  while (isalnum(peek()) || peek() == '_') {
    value += peek();
    advance();
  }

  location.end = column;

  raw = value;

  if (keywords.find(value) != keywords.end()) {
    return Token(keywords.at(value), value, raw, location);
  }

  return Token(TokenType::IDENT, value, raw, location);
}

Token Lexer::getTokenPunct() {
  std::string value = std::string(1, peek());
  std::string raw = value;

  auto location = SourceLocation(filename, lines[line - 1], value, line, column, index);
  location.start = column;

  advance();
  char c2 = peek();

  TokenType tokenType = TokenType::INVALID;

  if (punctuation.find(value + c2) != punctuation.end()) {
    value += c2;
    advance();
    tokenType = punctuation.at(value);

    char c3 = peek();
    if (punctuation.find(value + c3) != punctuation.end()) {
      value += c3;
      advance();
      tokenType = punctuation.at(value);
    }
  } else if (punctuation.find(value) != punctuation.end()) {
    tokenType = punctuation.at(value);
  } else {
    location.start = column;
    location.end = column;
    location.column = column;

    diag->report({
      ErrorType::UNKNOWN,
      Severity::ERROR,
      location,
      "unknown token `" + value + "`",
      "",
      ""
    });
  }

  location.end = column;

  raw = value;

  return Token(tokenType, value, raw, location);
}

void Lexer::skipComment() {
  while (peek() != '\n')
    advance();

  skipWhitespace();
}

void Lexer::skipMultiComment() {
  for (;;) {
    advance();
    if (peek() == '*') {
      advance();
      if (peek() == '/') {
        advance();
        return;
      }
    }
  }

  skipWhitespace();
}

void Lexer::skipWhitespace() {
  while (iswspace(peek()))
    advance();
}

void Lexer::advance() {
  if (peek() == '\n') {
    line++;
    column = 1;
  }

  index++;
  column++;
}

char Lexer::peek() {
  if (index < input.size() - 1)
    return input[index];
  return '\0';
}

}
