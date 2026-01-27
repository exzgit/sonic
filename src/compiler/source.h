#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <sstream>

inline std::vector<std::string> split_lines(const std::string& input) {
  std::vector<std::string> lines;
  std::stringstream ss(input);
  std::string line;

  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  return lines;
}

struct SourceLocation {
  std::string path;
  std::string lines;
  std::string raw_value;

  uint32_t line;
  uint32_t column;
  uint32_t offset;

  uint32_t start; // 1-based column
  uint32_t end;   // exclusive

  bool is_error = false;

  // Default constructor
  SourceLocation()
    : path(""),
      lines(""),
      raw_value(""),
      line(0),
      column(0),
      offset(0),
      start(0),
      end(0) {}

  SourceLocation(
    const std::string& path,
    const std::string& lines,
    const std::string& raw_value,
    uint32_t line,
    uint32_t column,
    uint32_t offset
  )
    : path(path),
      lines(lines),
      raw_value(raw_value),
      line(line),
      column(column),
      offset(offset),
      start(column),
      end(column + 1) {}

  std::string toString() const {
    return path + ":" + std::to_string(line) + ":" + std::to_string(start);
  }
};
