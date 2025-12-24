#include "error.h"
#include <iostream>

using namespace std;
using namespace errors;

std::vector<string> ErrorHandler::error_stack{};

void ErrorHandler::create(string error) {
  ErrorHandler::error_stack.push_back(error);
}

void ErrorHandler::debug() {
  if (ErrorHandler::error_stack.empty()) {
    return;
  }

  for (string error : ErrorHandler::error_stack) {
    cerr << error;
  }

  std::exit(1);
}
