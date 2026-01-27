#pragma once

// OS
#if defined(_WIN32)
  #define TARGET_OS_WINDOWS
#elif defined(__ANDROID__)
  #define TARGET_OS_ANDROID
#elif defined(__APPLE__)
  #define TARGET_OS_APPLE
#elif defined(__linux__)
  #define TARGET_OS_LINUX
#else
  #define TARGET_OS_UNKNOWN
#endif

// Architecture
#if defined(__x86_64__) || defined(_M_X64)
  #define TARGET_ARCH_X86_64
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define TARGET_ARCH_ARM64
#elif defined(__i386__) || defined(_M_IX86)
  #define TARGET_ARCH_X86
#elif defined(__arm__) || defined(_M_ARM)
  #define TARGET_ARCH_ARM
#else
  #define TARGET_ARCH_UNKNOWN
#endif
