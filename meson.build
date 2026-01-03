project('sonic-language', 'c', 'cpp')

executable('sonic',
  [
    'src/main.cpp',
    'src/core/error.cpp',
    'src/compiler/lexer.cpp',
    'src/compiler/parser.cpp',
    'src/compiler/ast/accepts.cpp',
    'src/compiler/symboltable.cpp'
  ],
  include_directories: include_directories('src/')
)
