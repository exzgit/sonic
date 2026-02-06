// ast_io.cpp
#include "ast_io.h"
#include "ast_json.h"
#include <fstream>

using namespace sonic::frontend::ast::json;

namespace sonic::frontend::ast::io {
  using json = nlohmann::json;

  bool saveProgramToFile(const Program& program, const std::string& path) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
      return false;

    json j = serializeProgram(program);

    // pretty print (buat debug)
    out << j.dump(2);

    out.close();
    return true;
  }

}
