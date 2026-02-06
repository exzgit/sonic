#include <iostream>
#include <fstream>
#include "symbol_io.h"
#include "symbol_json.h"

namespace sonic::frontend::symbol::io {

bool saveSymbolToFile(const Symbol& program, const std::string& path) {
    nlohmann::json j = symbolToJson((Symbol*)&program);

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(2);
    file.close();

    return true;
}

}
