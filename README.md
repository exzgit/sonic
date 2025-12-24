# Sonic

Sonic is an **experimental programming language and compiler** designed as a *learning laboratory* to understand how a programming language is designed, implemented, and executed.

The primary focus of Sonic is not production readiness, but **clarity of compiler architecture**, **language design exploration**, and **memory management experimentation**. This project is suitable for systems learners, beginner to intermediate compiler engineers, and anyone who wants to dissect the internals of a modern programming language.

---

## Project Goals

Sonic is created to:

* Serve as a practical reference for building a compiler (lexer, parser, AST, semantic analysis, code generation)
* Provide a playground for language design experiments
* Explore various memory management models
* Understand trade-offs between high-level abstractions and low-level control
* Act as a foundation for tooling experiments such as analyzers, interpreters, or optimizers

Sonic is **not** a production-ready language and does not guarantee API or syntax stability.

---

## Language Characteristics

Some of Sonic’s design characteristics:

* Statically typed language with an explicit AST
* Support for complex expressions such as member lookup and call chaining
* Extensible type system (generics, namespaces, etc.)
* Syntax designed to be easy to parse and analyze
* Informative and localized compiler error messages

Example Sonic code:

```sonic
let a: std::collection::list<i32> = std.collection.list<i32>()
```

---

## Project Structure

Main directory structure:

```
sonic/
├── src/
│   ├── compiler/
│   │   ├── lexer/        # Lexer and tokens
│   │   ├── parser/       # Parser and grammar
│   │   ├── ast/          # AST nodes and visitors
│   │   ├── semantic/     # Semantic analysis (WIP)
│   │   └── codegen/      # Code generation (WIP)
│   ├── core/             # Error handling and utilities
│   └── main.cpp          # Entry point
├── meson.build
└── README.md
```

---

## Build Instructions (Compiling the Sonic Compiler)

Sonic uses **Meson** as the build system and **Ninja** as the backend.

### Prerequisites

* C++ compiler with C++17 or newer support
* Meson
* Ninja

On Debian/Ubuntu-based systems:

```bash
sudo apt install meson ninja-build g++
```

### Build Steps

From the repository root:

```bash
meson setup build
meson compile -C build
```

If successful, the `sonic` binary will be available in the `build/` directory.

---

## Using Sonic

### Running the Compiler

```bash
./build/sonic <file.sn>
```

Example:

```bash
./build/sonic run examples/hello.sn
```

Currently, the compiler output focuses on:

* Syntax parsing and validation
* AST construction and visualization
* Error reporting

(Code generation is still experimental.)

---

## Project Status

* Lexer: stable
* Parser: actively developed
* AST: most core nodes implemented
* Semantic analysis: WIP
* Code generation: WIP

Breaking changes are **very likely**.

---

## Contributing

Contributions are very welcome, especially in the form of:

* Compiler architecture improvements
* Additional semantic analysis passes
* Internal compiler documentation
* Example Sonic programs

Fork the repository, create a branch, and submit a pull request.

---

## License

This project is licensed under the MIT License.

---

## Notes

Sonic is an exploratory project. If you are looking for a stable production language, use Rust, C++, or Go. If you want to **understand how those languages are built**, Sonic is the playground.
