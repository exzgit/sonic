#pragma once

#include <vector>
#include <string>

using namespace std;

namespace errors {
    class ErrorHandler {
      public:
        static void create(string err);
        static void debug();
      private:
        static std::vector<string> error_stack;
    };
}
