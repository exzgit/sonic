#pragma once
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
