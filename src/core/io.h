#pragma once

#include <fstream>
#include <iostream>

inline bool file_reader(std::string& filepath, std::string& content) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file)
    return true;

  content.assign(
    std::istreambuf_iterator<char>(file),
    std::istreambuf_iterator<char>()
  );

  return false;
}
