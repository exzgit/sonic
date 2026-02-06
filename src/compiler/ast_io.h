#pragma once

#include <string>
#include "ast.h"
#include <nlohmann/json.hpp>

using namespace sonic::frontend::ast;

namespace sonic::frontend::ast::io {

bool saveProgramToFile(const Program& program, const std::string& path);

}
