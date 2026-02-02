#pragma once

// c++ library
#include <string>
#include <vector>

// local header
#include "token.h"
#include "diagnostics.h"

namespace sonic::frontend {

class Lexer {
public:
  Lexer(const std::string& input, const std::string& filename);
  ~Lexer() = default;

  Token next_token();

  DiagnosticEngine* diag = nullptr;

private:
  std::string input;
  std::string filename;

  std::vector<std::string> lines;

  size_t index = 0;
  int line = 1;
  int column = 1;

private:
  Token getTokenNumber();
  Token getTokenString();
  Token getTokenChar();
  Token getTokenKeyword();
  Token getTokenPunct();

  void skipComment();
  void skipMultiComment();
  void skipWhitespace();
  void advance();

  char peek();
};

}
