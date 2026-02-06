project('sonic-language', 'c', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'buildtype=debug']
)

# Get C++ compiler
cc = meson.get_compiler('cpp')

# LLVM configuration
llvm_cflags = run_command('llvm-config', '--cxxflags', check: true).stdout().strip()
llvm_ldflags = run_command('llvm-config', '--ldflags', '--libs', 'core', 'support', check: true).stdout().strip()

json_dep = dependency('nlohmann_json', required: true)

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
  'src/compiler/codegen.cpp',
  'src/compiler/ast_json.cpp',
  'src/compiler/symbol_io.cpp',
  'src/compiler/ast_io.cpp',
  'src/core/startup.cpp',
  'src/core/debugging.cpp',
  'src/core/io.cpp',
])

# Build the sonic compiler executable
executable('sonic',
  sources,
  dependencies: [json_dep],
  include_directories: inc_dirs,
  cpp_args: llvm_cflags.split(),
  link_args: llvm_ldflags.split(),
  install: true,
)
