#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>

#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "ast.h"
#include "symbol.h"

using namespace sonic::frontend;

namespace sonic::backend {
  class SonicCodegen {
    public:
    SonicCodegen(Symbol* symbol);
    ~SonicCodegen();

    void generate(ast::Program* program);
    void saveBitcode(const std::string& path);
    void saveLLReadable(const std::string& path);
    void generate_statement(ast::Statement* stmt);
    llvm::Value* generate_expression(ast::Expression* expr);
    llvm::Type* mapping_type(ast::Type* type);

    private:
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::TargetMachine* targetMachine = nullptr;

    std::string current_file_output;
    Symbol* symbols;
    Symbol* current_function_ = nullptr;
    size_t offset_entry = 0;

    inline std::string getEntryLabel() {
      std::string entry = "sn_entry_" + std::to_string(offset_entry);
      offset_entry += 1;
      return entry;
    }
  };
};
