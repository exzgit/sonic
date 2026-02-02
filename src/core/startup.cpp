// c++ library
#include <iostream>
#include <fstream>
#include <filesystem>

// local header
#include "startup.h"
#include "config.h"
#include "target_info.h"

namespace fs = std::filesystem;

namespace sonic::startup {
  void generate_project_folder(const string& project_name) {
    config::project_name = project_name;
    config::target_platform = target_triple();

    if (fs::exists(project_name)) {
      std::cerr
        << "\033[31m==> error:\033[0m "
        << "failed to initialize project: folder already exists\n";
      exit(1);
    }

    fs::create_directory(project_name);

    string src_folder = project_name + "/src";
    fs::create_directory(src_folder);

    string config_file = project_name + "/config.snc";
    ofstream config_file_stream(config_file);
    config_file_stream
    << "// APP CONFIGURATION\n"
    << "@name " << project_name << "\n"
    << "@version " << "1.0.0" << "\n"
    << "@author " << "..." << "\n"
    << "@description " << "..." << "\n"
    << "@license " << "MIT License" << "\n\n"
    << "// TARGET PLATFORM\n"
    << "@target " << config::target_platform << "\n\n"
    << "// DEPENDENCIES\n"
    << "@use stdlib@latest\n"
    << "@use stdint@1.2.0\n"
    << "@use collections@^4.2.0";
    config_file_stream.close();

    string main_file = project_name + "/src/main.sn";
    ofstream main_file_stream(main_file);
    main_file_stream
    << "// @file    main.sn\n\nfunc main() {\n\tprintln(\"Hello, World!\");\n}";
    main_file_stream.close();
  }

  std::string pathToNamespace(const fs::path& file) {
    fs::path rel = fs::relative(file, fs::path(sonic::config::project_root));

    std::string out = "sn" + sonic::config::project_name;
    for (auto& part : rel) {
      if (part.extension() == ".sn") {
        out += "_" + part.stem().string();
      } else {
        out += "_" + part.string();
      }
    }
    return out;
  }

  void setProjectRoot(const std::string& entryFile) {
    sonic::config::project_root = fs::absolute(entryFile).parent_path();
  }
}
