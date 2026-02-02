#pragma once

// c++ library
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

namespace sonic::startup {
  void generate_project_folder(const string& project_name);

  std::string pathToNamespace(const fs::path& file);
  std::string getClearPath(const fs::path& file);

  void setProjectRoot(const std::string& entryFile);
};
