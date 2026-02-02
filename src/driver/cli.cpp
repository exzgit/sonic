// c++ library
#include <iostream>
#include <string>

// local header
#include "../core/config.h"
#include "../core/startup.h"
#include "io.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../compiler/diagnostics.h"
#include "../compiler/semantic.h"
#include "../compiler/symbol.h"

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
        std::exit(0);
      }

      sonic::startup::generate_project_folder(argv[i + 1]);
      std::exit(0);
    }
    // ===== BUILD FLAGS =====
    else if (arg == "compile") {
      cfg::is_compiled = true;
      sonic::config::project_path = "src/main.sn";
      continue;
    }
    else if (arg == "--debug") {
      cfg::runtime_debug = true;   // override compile-time
      continue;
    }
    else if (arg == "--release") {
      cfg::runtime_release = true;
      continue;
    }
    else if (arg == "--no-opt") {
      cfg::runtime_optimized = false;
      cfg::optimizer_level = sonic::config::OptLevel::NO;
      continue;
    }
    else if (arg == "-O2") cfg::optimizer_level = sonic::config::OptLevel::O2;
    else if (arg == "-O3") cfg::optimizer_level = sonic::config::OptLevel::O3;
    else if (arg == "-Ofast") cfg::optimizer_level = sonic::config::OptLevel::OFAST;
    else {
      if (i < 2) {
        std::cerr << "\033[31m(error)\033[0m " << "unknown arguments '" << arg << "'\n";
        std::exit(0);
      }

      if (sonic::io::is_exists(arg) && sonic::io::is_directory(arg)) {
        sonic::config::project_path = arg + "/src/main.sn";
      } else if (sonic::io::is_exists(arg) && sonic::io::is_file(arg)) {
        sonic::config::project_path = arg;
      } else {
        std::cerr << "\033[31m(error)\033[0m " << "unknown arguments '" << arg << "'\n";
        std::exit(0);
      }
    }
  }
}

using namespace sonic::io;

void compile_project();
void run_project();

int main(int argc, char* argv[]) {
  check_arguments(argc, argv);

  if (cfg::is_compiled) compile_project();

  return 0;
}

void compile_project() {
  std::string f = sonic::config::project_path;

  std::string content(read_file(f));

  if (content.empty()) {
    std::cerr << "\033[31m(error)\033[0m file '" << f << "' is empty or cannot be read.\n";
    return;
  }

  sonic::startup::setProjectRoot(f);

  DiagnosticEngine diag;
  sonic::frontend::Lexer lexer(content, f);
  lexer.diag = &diag;
  sonic::frontend::Parser parser(f, &lexer);
  parser.diag = &diag;
  auto program = parser.parse();

  if (program) {
    std::cout << "|--| DEBUG AST |--|\n";
    std::cout << "===================\n";
    std::cout << program->to_string(0) << "\n";
  }

  auto symbols = std::make_unique<Symbol>();
  SemanticAnalyzer analyzer(symbols.get());
  analyzer.diag = &diag;
  analyzer.analyze(program->clone());
  diag.flush();
}

void run_project() {

}
