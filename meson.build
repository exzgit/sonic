project('sonic-language', 'c', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17']
)

# Get C++ compiler
cc = meson.get_compiler('cpp')

# LLVM configuration
llvm_cflags = run_command('llvm-config', '--cxxflags', check: true).stdout().strip()
llvm_ldflags = run_command('llvm-config', '--ldflags', '--libs', 'core', 'support', check: true).stdout().strip()

# Include directories
inc_dirs = include_directories([
  'src/driver',
  'src/core',
  'src/compiler',
])

# Source files
sources = files([
  'src/driver/cli.cpp',
  'src/compiler/lexer.cpp',
  'src/compiler/parser.cpp',
  'src/compiler/semantic.cpp',
  'src/core/startup.cpp',
  'src/core/debugging.cpp',
  'src/core/io.cpp',
])

# Build the sonic compiler executable
executable('sonic',
  sources,
  include_directories: inc_dirs,
  cpp_args: llvm_cflags.split(),
  link_args: llvm_ldflags.split(),
  install: true,
)
