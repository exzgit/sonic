// c++ library
#include <iostream>

// local header
#include "debugging.h"
#include "config.h"

namespace cfg = sonic::config;

namespace sonic::debug {
  void Debug::log(std::string msg) {
    if (cfg::runtime_debug) {
      std::cout << "\033[94m(debug)\033[0m " << msg << "\n";
    }
  }

  void print(const std::string& msg) {
    std::cout << "\033[92m(info)\033[0m " << msg << std::endl;
  }
};
