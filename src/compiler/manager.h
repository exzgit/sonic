#pragma once

#include "ast.h"
#include <vector>

using namespace sonic::frontend;

inline std::vector<ast::Program*> astListManager;

inline void insert_ast(ast::Program* ast) {
  astListManager.push_back(ast);
}
