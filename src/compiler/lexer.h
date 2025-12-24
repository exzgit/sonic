#pragma once

#include <string>
#include <vector>

#include "../core/token.h"

using namespace std;

namespace frontend {

  inline string wspaceToString(char c) {
    if (c == '\n') return "\\n";
    if (c == '\t') return "\\t";
    if (c == '\r') return "\\r";
    if (c == '\0') return "\\0";
    return string(1, c);
  }

  class Lexer {
    public:
      Lexer(string& sources, string& files);
      ~Lexer() = default;

      Token next();

    private:
      Token getStringToken();
      Token getCharToken();
      Token getNumberToken();
      Token getIdentifierToken();
      Token getPunctuationToken();

      void advance();
      char peek();

      bool is_at_end() const {
        return index >= sources.size();
      }
    private:
      string sources;
      string files;

      vector<string> lines;

      size_t line;
      size_t column;
      size_t index;
  };
};
