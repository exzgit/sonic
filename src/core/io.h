#pragma once

// c++ library
#include <string>

using namespace std;

namespace sonic::io {

  string read_file(const string& path);
  void write_file(const string& path, const string& content);
  void append_file(const string& path, const string& content);
  void delete_file(const string& path);
  void copy_file(const string& src_path, const string& dest_path);
  bool is_file(const string& path);
  bool is_directory(const string& path);
  bool is_exists(const string& path);
  void create_folder(const string& path);
  std::string resolvePath(const std::string& path);
  void create_file_and_folder(const std::string& filepath);

  std::string getPathWithoutExtension(const std::string& path);
  std::string getFileNameWithoutExt(const std::string& path);
  std::string getPathWithoutFile(const std::string& path);
  std::string getFullPath(const std::string& path);
}
