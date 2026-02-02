#pragma once

// c++ library
#include <iomanip>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

// local headers
#include "source.h"

enum class ErrorType {
  INVALID,
  UNEXPECTED,
  SYNTAX,
  SEMANTIC,
  INTERNAL,
  UNIMPLEMENTED,
  UNKNOWN
};

enum class Severity {
  ERROR,
  WARNING,
  INFO
};

struct Diagnostic {
  ErrorType errorType;
  Severity severity;

  SourceLocation location;
  std::string message;
  std::string note;
  std::string hint;
};

inline const char* severityColor(Severity s) {
  switch (s) {
    case Severity::ERROR:   return "\033[31m"; // red
    case Severity::WARNING: return "\033[33m"; // yellow
    case Severity::INFO:    return "\033[34m"; // blue
  }
  return "\033[0m";
}

inline const char* severityToString(Severity s) {
  switch (s) {
    case Severity::ERROR:   return "error";
    case Severity::WARNING: return "warning";
    case Severity::INFO:    return "info";
  }
  return "unknown";
}

inline const char* errorTypeToString(ErrorType t) {
  switch (t) {
    case ErrorType::INVALID:       return "invalid";
    case ErrorType::UNEXPECTED:    return "unexpected";
    case ErrorType::SYNTAX:        return "syntax";
    case ErrorType::SEMANTIC:      return "semantic";
    case ErrorType::INTERNAL:      return "internal";
    case ErrorType::UNIMPLEMENTED: return "unimplemented";
    default:                       return "unknown";
  }
}

class DiagnosticEngine {
public:
  void report(const Diagnostic& diagnostic) {
    diagnostics.push_back(diagnostic);
  }

  void flush() const {
    for (const auto& d : diagnostics) {
      printOne(d);
    }

    if (!diagnostics.empty()) {
      std::exit(1);
    }
  }

  int size() const {
    return diagnostics.size();
  }

private:
  std::vector<Diagnostic> diagnostics;

  static void printOne(const Diagnostic& d) {
    const auto& loc = d.location;

    // ===== HEADER =====
    std::cerr
      << severityColor(d.severity)
      << severityToString(d.severity)
      << "\033[0m"
      << ": "
      << d.message
      << "\n";

    // ===== LOCATION =====
    std::cerr
      << "  --> "
      << loc.toString()
      << " \033[90m(" << errorTypeToString(d.errorType) << ")\033[0m\n";

    // ===== SOURCE LINE =====
    if (!loc.lines.empty()) {
      const size_t lineDigits = std::to_string(loc.line).length();
      const size_t gutter = std::max<size_t>(lineDigits, 2);

      // empty gutter
      std::cerr << " " << std::string(gutter, ' ') << " |\n";

      // source line
      std::cerr
        << " "
        << std::setw(gutter) << loc.line
        << " | "
        << loc.lines
        << "\n";

      // caret line
      std::cerr << " " << std::string(gutter, ' ') << " | ";

      // ---- CARET POSITION (FIXED) ----
      size_t caretPos = (loc.start > 0) ? loc.start - 2 : 0;
      caretPos = std::min(caretPos, loc.lines.size());

      // handle tabs BEFORE caret
      for (size_t i = 0; i < caretPos; ++i) {
        if (loc.lines[i] == '\t')
          std::cerr << "\t"; // TAB WIDTH = 4 (KONSISTEN)
        else
          std::cerr << " ";
      }

      size_t range = 1;
      if (loc.end > loc.start)
        range = loc.end - loc.start;

      std::cerr
        << severityColor(d.severity)
        << std::string(range, '^')
        << "\033[0m\n";
    }

    // ===== NOTE =====
    if (!d.note.empty()) {
      std::cerr
        << "\n\033[36mnote:\033[0m "
        << d.note << "\n";
    }

    // ===== HINT =====
    if (!d.hint.empty()) {
      std::cerr
        << "\033[36mhint:\033[0m "
        << d.hint << "\n";
    }

    std::cerr << "\n";
  }
};
