#ifndef SONIC_CONFIG_H
#define SONIC_CONFIG_H

#include <string>

namespace sonic::config {

  // ===============================
  // Compile-time build config
  // ===============================
  #ifdef NDEBUG
    constexpr bool BUILD_DEBUG    = false;
    constexpr bool BUILD_RELEASE  = true;
    constexpr bool BUILD_OPTIMIZED = true;
  #else
    constexpr bool BUILD_DEBUG    = true;
    constexpr bool BUILD_RELEASE  = false;
    constexpr bool BUILD_OPTIMIZED = false;
  #endif

  // ===============================
  // Runtime flags (CLI overridable)
  // ===============================
  inline bool runtime_debug     = BUILD_DEBUG;
  inline bool runtime_release   = BUILD_RELEASE;
  inline bool runtime_optimized = BUILD_OPTIMIZED;

  inline bool is_compiled = false;
  inline bool is_running  = false;

  // ===============================
  // Runtime project state
  // ===============================
  inline std::string project_name;
  inline std::string project_path;
  inline std::string project_root;
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
