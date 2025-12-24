#include "lexer.h"
#include "../core/error.h"
#include "core/token.h"

#include <sstream>

using namespace std;
using namespace errors;

namespace frontend {
  Lexer::Lexer(std::string& sources, std::string& files)
    : sources(sources),
      files(files),
      line(1),
      column(1),
      index(0)
  {
    std::stringstream ss(sources);
    std::string line;

    while(std::getline(ss, line)) {
      lines.push_back(line);
    }

    lines.push_back("\n");
  }

  Token Lexer::next() {
    while (!is_at_end() && peek() != '\0') {

      while (std::isspace(static_cast<unsigned char>(peek())))
        advance();

      if (peek() == '#') {
        while (peek() != '\n') advance();
        continue;
      }

      while (std::isspace(static_cast<unsigned char>(peek())))
        advance();

      if (std::isdigit(peek()))
        return getNumberToken();
      else if (peek() == '\'')
        return getCharToken();
      else if (peek() == '"')
        return getStringToken();
      else if (std::isalnum(peek()) || peek() == '_')
        return getIdentifierToken();
      else if (std::ispunct(peek()))
        return getPunctuationToken();
      else
        advance();
    }

    auto endToken = Token {
      TokenKind::END_OF_FILE,
      "\0",
      column,
      column + 1,
      column,
      column + 1,
      files,
      lines[line - 1]
    };

    return endToken;
  }

  Token Lexer::getStringToken() {
    size_t start = column;
    size_t cline = line;

    std::string value;

    advance();

    while (!is_at_end() && peek() != '"') {
      if (peek() == '\\') {
        advance();
        if (peek() == '\\') {
          advance();
          value += "\\";
        } else if (peek() == '"') {
          advance();
          value += "\"";
        } else if (peek() == 'n') {
          advance();
          value += "\n";
        } else if (peek() == 't') {
          advance();
          value += "\t";
        } else if (peek() == '0') {
          advance();
          value += "\0";
        } else if (peek() == 'r') {
          advance();
          value += "\r";
        }
      }

      value.push_back(peek());
      advance();
    }

    if (peek() == '"') advance();
    else {
      stringstream error;
      error << "\033[31merrors:\033[0m expected `\"`, but got `" << wspaceToString(peek()) << "`\n";
      error << "  at " << files << ":" << line << ":" << column << "\n";
      error << "  --> " << lines[line-1] << "\n";
      ErrorHandler::create(error.str());
    }

    return Token {
      TokenKind::STRING_LITERAL,
      value,
      cline,
      column,
      start,
      column,
      files,
      lines[cline - 1]
    };
  }

  Token Lexer::getCharToken() {
    size_t start = column;
    size_t cline = line;

    std::string value;
    while (!is_at_end() && peek() != '\'') {
      if (peek() == '\\') {
        advance();
        if (peek() == '\\') {
          advance();
          value += "\\";
        } else if (peek() == '\'') {
          advance();
          value += "\"";
        } else if (peek() == 'n') {
          advance();
          value += "\n";
        } else if (peek() == 't') {
          advance();
          value += "\t";
        } else if (peek() == '0') {
          advance();
          value += "\0";
        } else if (peek() == 'r') {
          advance();
          value += "\r";
        }
      }

      value += peek();
      advance();
    }

    if (peek() == '\'') advance();
    else {
      stringstream error;
      error << "\033[31merrors:\033[0m expected `'`, but got `" << wspaceToString(peek()) << "`\n";
      error << "  at " << files << ":" << line << ":" << column << "\n";
      error << "  --> " << lines[line-1] << "\n";
      ErrorHandler::create(error.str());
    }

    return Token {
      TokenKind::CHAR_LITERAL,
      value,
      cline,
      column,
      start,
      column,
      files,
      lines[cline - 1]
    };
  }

  Token Lexer::getNumberToken() {
    size_t start = column;
    size_t cline = line;

    std::string value;
    TokenKind kind = TokenKind::NUMBER;

    while(std::isdigit(peek())) {
      value += peek();
      advance();
    }

    if (value == "0" && peek() == 'x') {
      value += peek();
      advance();

      kind = TokenKind::HEX;

      if (std::isalnum(peek())) {
        while(std::isalnum(peek())) {
          value += peek();
          advance();
        }
      } else {
        stringstream error;
        error << "\033[31merrors:\033[0m expected hexadecimal digit after `" << value << "`, but found `" << peek() << "`\n";
        error << "  at " << files << ":" << line << ":" << column << "\n";
        error << "  --> " << lines[line-1] << "\n";
        ErrorHandler::create(error.str());
        value += '0';
      }
    }
    else if (peek() == '.') {
      value += peek();
      advance();
      if (std::isdigit(peek())) {

        while(std::isdigit(peek())) {
          value += peek();
          advance();
        }
      } else {
        stringstream error;
        error << "\033[31merrors:\033[0m expected digit, but found `" << wspaceToString(peek()) << "`\n";
        error << "  at " << files << ":" << line << ":" << column << "\n";
        error << "  --> " << lines[line-1] << "\n";
        ErrorHandler::create(error.str());
        value += '0';
      }
    }

    if (peek() == 'e' || peek() == 'E') {
      value += peek();
      advance();
      kind = TokenKind::SCNOTATION;

      if (peek() == '+' || peek() == '-') {
        value += peek();
        advance();

        if (std::isdigit(peek())) {
          while(std::isdigit(peek())) {
            value += peek();
            advance();
          }
        } else {
          stringstream error;
          error << "\033[31merrors:\033[0m expected digit, but got `" << wspaceToString(peek()) << "`\n";
          error << "  at " << files << ":" << line << ":" << column << "\n";
          error << "  --> " << lines[line-1] << "\n";
          ErrorHandler::create(error.str());
          value += '0';
        }
      } else {
        stringstream error;
        error << "\033[31merrors:\033[0m expected `+` / `-`, but got `" << wspaceToString(peek()) << "`\n";
        error << "  at " << files << ":" << line << ":" << column << "\n";
        error << "  --> " << lines[line-1] << "\n";
        ErrorHandler::create(error.str());
        value += '0';
      }
    } else if (peek() == '+' || peek() == '-') {
      value += peek();
      advance();
      kind = TokenKind::SCNOTATION;

      if (std::isdigit(peek())) {
        while(std::isdigit(peek())) {
          value += peek();
          advance();
        }
      } else {
        stringstream error;
        error << "\033[31merrors:\033[0m expected digit, but got `" << wspaceToString(peek()) << "`\n";
        error << "  at " << files << ":" << line << ":" << column << "\n";
        error << "  --> " << lines[line-1] << "\n";
        ErrorHandler::create(error.str());
        value += '0';
      }
    }

    return Token {
      kind,
      value,
      cline,
      column,
      start,
      column,
      files,
      lines[cline - 1]
    };
  }

  Token Lexer::getIdentifierToken() {
    size_t start = column;
    size_t cline = line;

    std::string value;
    while (std::isalnum(peek()) || peek() == '_') {
      value += peek();
      advance();
    }

    TokenKind kind = TokenKind::IDENTIFIER;

    if (value == "fn")
      kind = TokenKind::FUNCTION;
    else if (value == "return")
      kind = TokenKind::RETURN;
    else if (value == "if")
      kind = TokenKind::IF;
    else if (value == "else")
      kind = TokenKind::ELSE;
    else if (value == "while")
      kind = TokenKind::WHILE;
    else if (value == "switch")
      kind = TokenKind::SWITCH;
    else if (value == "case")
      kind = TokenKind::CASE;
    else if (value == "default")
      kind = TokenKind::DEFAULT;
    else if (value == "break")
      kind = TokenKind::BREAK;
    else if (value == "continue")
      kind = TokenKind::CONTINUE;
    else if (value == "summon")
      kind = TokenKind::SUMMON;
    else if (value == "for")
      kind = TokenKind::FOR;
    else if (value == "struct")
      kind = TokenKind::STRUCT;
    else if (value == "enum")
      kind = TokenKind::ENUM;
    else if (value == "let")
      kind = TokenKind::LET;
    else if (value == "root")
      kind = TokenKind::ROOT;
    else if (value == "super")
      kind = TokenKind::SUPER;
    else if (value == "true")
      kind = TokenKind::TRUE;
    else if (value == "false")
      kind = TokenKind::FALSE;
    else if (value == "none")
      kind = TokenKind::NONE;
    else if (value == "i8")
      kind = TokenKind::I8;
    else if (value == "i16")
      kind = TokenKind::I16;
    else if (value == "i32")
      kind = TokenKind::I32;
    else if (value == "i64")
      kind = TokenKind::I64;
    else if (value == "i128")
      kind = TokenKind::I128;
    else if (value == "u8")
      kind = TokenKind::U8;
    else if (value == "u16")
      kind = TokenKind::U16;
    else if (value == "u32")
      kind = TokenKind::U32;
    else if (value == "u64")
      kind = TokenKind::U64;
    else if (value == "u128")
      kind = TokenKind::U128;
    else if (value == "f32")
      kind = TokenKind::F32;
    else if (value == "f64")
      kind = TokenKind::F64;
    else if (value == "str")
      kind = TokenKind::STRING;
    else if (value == "bool")
      kind = TokenKind::BOOL;
    else if (value == "void")
      kind = TokenKind::VOID;
    else if (value == "auto")
      kind = TokenKind::AUTO;
    else if (value == "any")
      kind = TokenKind::ANY;

    return Token {
      kind,
      value,
      cline,
      column,
      start,
      column,
      files,
      lines[cline - 1]
    };
  }

  Token Lexer::getPunctuationToken() {

    size_t start = column;
    size_t cline = line;

    char c1 = peek();
    advance();
    string c2 = string(1, c1) + peek();

    TokenKind kind = TokenKind::UNKNOWN;

    if (c2 == "==") {
      kind = TokenKind::IS_EQUAL;
      advance();
    }
    else if (c2 == "!=") {
      kind = TokenKind::NOT_EQUAL;
      advance();
    }
    else if (c2 == "+=") {
      kind = TokenKind::PLUS_EQUAL;
      advancehttps://www.google.com/search?q=apa+itu+AI+OCR&sca_esv=49d481c5a8cd6c1b&sxsrf=AE3TifODvupBdMZ1Ltb6sbpb9VXaetasaw%3A1766588140226&ei=7P5LaYm9Daey4-EPsZrWmAY&ved=0ahUKEwiJ47_UvdaRAxUn2TgGHTGNFWMQ4dUDCBE&uact=5&oq=apa+itu+AI+OCR&gs_lp=Egxnd3Mtd2l6LXNlcnAiDmFwYSBpdHUgQUkgT0NSMgQQABgeMggQABiABBiiBDIIEAAYgAQYogRIpBNQjgxYixJwAXgBkAEAmAFeoAH8BKoBATi4AQPIAQD4AQGYAgigAvEEwgIKEAAYsAMY1gQYR8ICDRAAGIAEGLADGEMYigXCAgcQIxiwAhgnwgIGEAAYBxgewgIHEAAYgAQYDcICBhAAGA0YHsICCRAAGIAEGAoYDZgDAIgGAZAGCpIHATigB9cusgcBN7gH4wTCBwUyLTMuNcgHPYAIAA&sclient=gws-wiz-serp();
    }
    else if (c2 == "-=") {
      kind = TokenKind::MINUS_EQUAL;
      advance();
    }
    else if (c2 == "/=") {
      kind = TokenKind::SLASH_EQUAL;
      advance();
    }
    else if (c2 == "*=") {
      kind = TokenKind::STAR_EQUAL;
      advance();
    }
    else if (c2 == "%=") {
      kind = TokenKind::PERCENT_EQUAL;
      advance();
    }
    else if (c2 == "^=") {
      kind = TokenKind::POWER_EQUAL;
      advance();
    }
    else if (c2 == "<=") {
      kind = TokenKind::LESS_EQUAL;
      advance();
    }
    else if (c2 == ">=") {
      kind = TokenKind::GREATER_EQUAL;
      advance();
    }
    else if (c2 == "&&") {
      kind = TokenKind::AND;
      advance();
    }
    else if (c2 == "->") {
      kind = TokenKind::ARROW;
      advance();
    }
    else if (c2 == "||") {
      kind = TokenKind::OR;
      advance();
    }
    else if (c2 == "::") {
      kind = TokenKind::DOUBLE_COLON;
      advance();
    }
    else {
      c2 = std::string(1, c1);

      if (c1 == '(')
        kind = TokenKind::LPARENT;
      else if (c1 == ')')
        kind = TokenKind::RPARENT;
      else if (c1 == '[')
        kind = TokenKind::LBRACKET;
      else if (c1 == ']')
        kind = TokenKind::RBRACKET;
      else if (c1 == '{')
        kind = TokenKind::LBRACE;
      else if (c1 == '}')
        kind = TokenKind::RBRACE;
      else if (c1 == '=')
        kind = TokenKind::EQUAL;
      else if (c1 == '&')
        kind = TokenKind::AMPER;
      else if (c1 == '|')
        kind = TokenKind::PIPE;
      else if (c1 == '!')
        kind = TokenKind::EXCLAMATION;
      else if (c1 == '~')
        kind = TokenKind::TILDE;
      else if (c1 == '@')
        kind = TokenKind::AT;
      else if (c1 == '$')
        kind = TokenKind::DOLLAR;
      else if (c1 == '%')
        kind = TokenKind::PERCENT;
      else if (c1 == '^')
        kind = TokenKind::POWER;
      else if (c1 == '*')
        kind = TokenKind::STAR;
      else if (c1 == '-')
        kind = TokenKind::MINUS;
      else if (c1 == '+')
        kind = TokenKind::PLUS;
      else if (c1 == '<')
        kind = TokenKind::LESS;
      else if (c1 == '>')
        kind = TokenKind::GREATER;
      else if (c1 == ',')
        kind = TokenKind::COMMA;
      else if (c1 == '.')
        kind = TokenKind::DOT;
      else if (c1 == '/')
        kind = TokenKind::SLASH;
      else if (c1 == ':')
        kind = TokenKind::COLON;
      else if (c1 == ';')
        kind = TokenKind::SEMICOLON;
      else if (c1 == '?')
        kind = TokenKind::QUESTION;
    }

    return Token {
      kind,
      c2,
      cline,
      column,
      start,
      column,
      files,
      lines[cline - 1]
    };
  }

  void Lexer::advance()
  {
    if (peek() == '\n') {
      line++;
      column = 0;
    }

    column++;
    index++;
  }

  char Lexer::peek()
  {
    if (is_at_end()) return '\0';
    if (sources[index] == '\0') return '\0';
    return sources[index];
  }
}
