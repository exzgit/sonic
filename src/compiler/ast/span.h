#pragma once

#include <string>

using namespace std;

namespace frontend {

  struct SourceSpan {
    string files;
    string sources;

    size_t line;
    size_t column;
    size_t start;
    size_t end;
  };

}
