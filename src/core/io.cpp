#include "io.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace sonic::io {

  string read_file(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
      std::cerr << "\033[31merror:\033[0m failed to open file '" << path << "'";
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();
    return content;
  }

  void write_file(const string& path, const string& content) {
    ofstream file(path);
    if (!file.is_open()) {
      std::cerr << "\033[31merror:\033[0m failed to open file '" << path << "'";
    }
    file << content;
    file.close();
  }

  void append_file(const string& path, const string& content) {
    ofstream file(path, ios::app);
    if (!file.is_open()) {
      std::cerr << "\033[31merror:\033[0m failed to open file '" << path << "'";
    }
    file << content;
    file.close();
  }

  void delete_file(const string& path) {
    if (!fs::remove(path)) {
      std::cerr << "\033[31merror:\033[0m failed to delete file '" << path << "'";
    }
  }

  void copy_file(const string& src_path, const string& dest_path) {
    ifstream src(src_path);
    ofstream dest(dest_path);
    if (!src.is_open() || !dest.is_open()) {
      std::cerr << "\033[31merror:\033[0m failed to copy file '" << src_path << "' to '" << dest_path << "'";
    }
    dest << src.rdbuf();
    src.close();
    dest.close();
  }

  bool is_file(const string& path) {
    return fs::is_regular_file(path);
  }

  bool is_directory(const string& path) {
    return fs::is_directory(path);
  }

  bool is_exists(const string& path) {
    return fs::exists(path);
  }

  std::string getPathWithoutExtension(const std::string& path) {
    size_t pos = path.find_last_of(".");
    if (pos == std::string::npos) {
      return path;
    }
    return path.substr(0, pos);
  }

  std::string getFileNameWithoutExt(const std::string& path) {
    if (path.empty()) return "";

    // ambil filename (setelah '/' atau '\')
    size_t sep = path.find_last_of("/\\");
    size_t start = (sep == std::string::npos) ? 0 : sep + 1;

    // ambil posisi dot terakhir
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || dot < start) {
        // tidak ada extension
        return path.substr(start);
    }

    return path.substr(start, dot - start);
  }

  std::string getPathWithoutFile(const std::string& path) {
    if (path.empty()) return "";

    // cari separator terakhir
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        // tidak ada path, hanya filename
        return "";
    }

    return path.substr(0, pos);
  }

}
