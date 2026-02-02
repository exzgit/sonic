#ifndef SONIC_CONFIG_H
#define SONIC_CONFIG_H

// c++ library
#include <string>

namespace sonic::config {

  // ===============================
  // Runtime flags (CLI overridable)
  // ===============================
  inline bool runtime_debug     = false;
  inline bool runtime_release   = false;
  inline bool runtime_optimized = true;

  inline bool is_compiled = false;

  enum OptLevel {
    NO,
    O2,
    O3,
    OFAST,
  };
  inline OptLevel optimizer_level = OptLevel::O2;

  // ===============================
  // Runtime project state
  // ===============================
  inline std::string project_name;
  inline std::string project_path;
  inline std::string project_root;
  inline std::string project_build;
  inline std::string config_file;
  inline std::string output_name;
  inline std::string target_platform;

  // ===============================
  // App metadata
  // ===============================
  constexpr const char* APP_NAME        = "sonic";
  constexpr const char* APP_VERSION     = "0.1.0";
  constexpr const char* APP_AUTHOR      = "Ezra Valenne Tofa";
  constexpr const char* APP_EMAIL       = "officialbangezz@gmail.com";
  constexpr const char* APP_LICENSE     = "MIT License";
}

#endif
