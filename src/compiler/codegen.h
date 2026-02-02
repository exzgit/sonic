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

    void generate(SonicStmt* stmt);
    void saveBitcode(const std::string& path);
    void saveLLReadable(const std::string& path);
    void generate_statement(SonicStmt* stmt);
    void generate_expression(SonicExpr* expr);
    llvm::Type* mapping_type(SonicType* type);

    private:
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::TargetMachine* targetMachine = nullptr;

    std::string current_file_output;
    Symbol* symbols;
  };
};
