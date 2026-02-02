#pragma once

// c++ library
#include <string>
#include <platform.h>

inline std::string target_triple() {
#if defined(TARGET_OS_WINDOWS)
  #if defined(TARGET_ARCH_X86_64)
    return "x86_64-pc-windows-msvc";
  #elif defined(TARGET_ARCH_X86)
    return "i686-pc-windows-msvc";
  #endif

#elif defined(TARGET_OS_LINUX)
  #if defined(TARGET_ARCH_X86_64)
    return "x86_64-unknown-linux-gnu";
  #elif defined(TARGET_ARCH_ARM64)
    return "aarch64-unknown-linux-gnu";
  #endif

#elif defined(TARGET_OS_ANDROID)
  #if defined(TARGET_ARCH_ARM64)
    return "aarch64-linux-android";
  #elif defined(TARGET_ARCH_ARM)
    return "armv7a-linux-androideabi";
  #endif

#elif defined(TARGET_OS_APPLE)
  #if defined(TARGET_ARCH_ARM64)
    return "arm64-apple-darwin";
  #elif defined(TARGET_ARCH_X86_64)
    return "x86_64-apple-darwin";
  #endif
#endif

  return "unknown-unknown-unknown";
}

inline std::string target_cpu(const std::string& target_triple) {
  // Windows
  if (target_triple.find("windows") != std::string::npos) {
      if (target_triple.find("x86_64") != std::string::npos) return "x86-64";
      if (target_triple.find("i686") != std::string::npos)  return "i686";
  }

  // Linux
  if (target_triple.find("linux") != std::string::npos) {
      if (target_triple.find("x86_64") != std::string::npos) return "generic"; // atau tigerlake jika native
      if (target_triple.find("aarch64") != std::string::npos) return "generic";
  }

  // Android
  if (target_triple.find("android") != std::string::npos) {
      if (target_triple.find("aarch64") != std::string::npos) return "generic";
      if (target_triple.find("armv7") != std::string::npos) return "generic";
  }

  // Apple / macOS / iOS
  if (target_triple.find("apple") != std::string::npos) {
      if (target_triple.find("x86_64") != std::string::npos) return "core2"; // default Apple x86
      if (target_triple.find("arm64") != std::string::npos) return "generic";
  }

  // fallback
  return "generic";
}
