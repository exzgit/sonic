#include <iostream>
#include <string>

#include "../core/config.h"
#include "../core/startup.h"
#include "io.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "semantic.h"
#include "../compiler/ast/debug.h"
#include "../compiler/symbol.h"
#include "../compiler/diagnostics.h"

namespace cfg = sonic::config;
using namespace sonic::frontend;

static void print_help() {
  std::cout <<
R"(sonic - Simple Programming Language

Usage:
  sonic new <project_name>
  sonic compile [options]
  sonic run
  sonic --version
  sonic --author
  sonic --license
  sonic --help

Options:
  --debug        Enable debug mode
  --release      Enable release mode
  --no-opt       Disable optimization
)";
}

static void print_version() {
  std::cout << cfg::APP_NAME << " v" << cfg::APP_VERSION << "\n";
}

static void print_author() {
  std::cout << cfg::APP_AUTHOR << " <" << cfg::APP_EMAIL << ">\n";
}

static void print_license() {
  std::cout << cfg::APP_LICENSE << "\n";
}

void check_arguments(int argc, char* argv[]) {
  if (argc < 2) {
    print_help();
    std::exit(1);
  }

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    // ===== HELP =====
    if (arg == "-h" || arg == "--help") {
      print_help();
      std::exit(0);
    }

    // ===== METADATA =====
    if (arg == "--version") {
      print_version();
      std::exit(0);
    }

    if (arg == "--author") {
      print_author();
      std::exit(0);
    }

    if (arg == "--license") {
      print_license();
      std::exit(0);
    }

    // ===== PROJECT INIT =====
    if (arg == "new") {
      if (i + 1 >= argc) {
        std::cerr << "Missing project name\n";
        std::exit(1);
      }

      sonic::startup::generate_project_folder(argv[i + 1]);
      std::exit(0);
    }

    // ===== BUILD FLAGS =====
    if (arg == "compile") {
      cfg::is_compiled = true;
      continue;
    }

    if (arg == "run") {
      cfg::is_running = true;
      continue;
    }

    if (arg == "--debug") {
      cfg::runtime_debug = true;   // override compile-time
      continue;
    }

    if (arg == "--release") {
      cfg::runtime_release = true;
      continue;
    }

    if (arg == "--no-opt") {
      cfg::runtime_optimized = false;
      continue;
    }
  }
}

using namespace sonic::io;

int main(int argc, char* argv[]) {
  // check_arguments(argc, argv);

  // if (cfg::is_compiled) compile_project();
  // if (cfg::is_running)  run_project();

  std::string f = argv[1];

  std::cout << "File: " << f << "\n";

  std::string content(read_file(f));

  if (content.empty()) {
    std::cerr << "\033[31m(error)\033[0m file '" << f << "' is empty or cannot be read.\n";
    return 1;
  }

  sonic::startup::setProjectRoot(f);

  DiagnosticEngine diag;
  sonic::frontend::Lexer lexer(content, cfg::project_root + "/" + f);
  lexer.diag = &diag;
  sonic::frontend::Parser parser(cfg::project_root + "/" + f, &lexer);
  parser.diag = &diag;
  auto* program = parser.parse();

  // sonic::frontend::ASTPrinter printer;
  // printer.print_stmt(program);

  Symbol* universeSymbol = new Symbol();
  universeSymbol->name = "__universe__";
  universeSymbol->scopeLevel = GLOBAL_SCOPE;
  universeSymbol->table = new SymbolTable();

  sonic::frontend::SemanticAnalyzer analyzer(universeSymbol);
  analyzer.diag = &diag;
  analyzer.analyze(program);
  std::cout << "DIAG SIZE: " << analyzer.diag->size() << "\n";
  analyzer.diag->flush();

  // printSymbol(universeSymbol, 0);

  return 0;
}
