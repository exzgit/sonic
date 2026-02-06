#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "symbol.h"

namespace sonic::frontend::symbol::io {

bool saveSymbolToFile(const Symbol& program, const std::string& path);

}
