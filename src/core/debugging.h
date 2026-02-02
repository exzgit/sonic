#pragma once

// c++ library
#include <string>

namespace sonic::debug {

  class Debug {
    public:
      static void log(std::string msg);
  };


  void print(const std::string& msg);
};
