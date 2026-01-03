#include <iostream>

#include "core/io.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "core/error.h"
#include "compiler/ast/visitor.h"
#include "compiler/ast/debug.h"
#include "compiler/semantic.h"

using namespace std;
using namespace errors;
using namespace frontend;

int main(int argc, char** argv) {
  if (argc == 1) {
    cout << "sonic version 0.1\nusage: sonic run <filepath>\n";
    return 0;
  }

  if (argc > 2) {
    string cmd1 = argv[1];
    if (cmd1 == "run") {
      string filepath = argv[2];
      string content;
      if (file_reader(filepath, content)) {
        cerr << "\033[31merrors:\033[0m failed to open file!\n  --> " << filepath << "\n";
        return 1;
      }

      Lexer lexer = Lexer(content, filepath);
      Parser parser = Parser(filepath, &lexer);
      auto nodes = parser.parse();

      ErrorHandler::debug();

      ASTDebugger debug = ASTDebugger();
      nodes->accept(debug);

      SemanticAnalyzer semantic;
      nodes->accept(semantic);
    }
  } else
    cout << "usage: sonic run <filepath>\n";

  return 0;
}
