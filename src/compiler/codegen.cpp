#include "codegen.h"
#include "../core/config.h"

#include <iostream>
#include <llvm/CodeGen/MachineBasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/raw_ostream.h>

#include "../core/io.h"
#include "startup.h"

namespace cfg = sonic::config;

namespace sonic::backend {
  SonicCodegen::SonicCodegen(Symbol* symbol) : symbols(symbol) {
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    if (cfg::target_platform.empty()) {
      cfg::target_platform = llvm::sys::getDefaultTargetTriple();
    }

    llvm::Triple triple(cfg::target_platform);

    module = std::make_unique<llvm::Module>("sonic_module", context);
    module->setTargetTriple(triple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
      llvm::errs() << "Target error: " << error << "\n";
      std::abort();
    }

    llvm::TargetOptions opt;
    auto cpu = llvm::sys::getHostCPUName();
    auto features = "";

    targetMachine = target->createTargetMachine(
      triple,
      cpu,
      features,
      opt,
      std::nullopt
    );

    module->setDataLayout(targetMachine->createDataLayout());

    builder = std::make_unique<llvm::IRBuilder<>>(context);
  }

  SonicCodegen::~SonicCodegen() {
    delete targetMachine;
  }

  void SonicCodegen::saveBitcode(const std::string& path) {
    std::string output_file = cfg::project_build + "/cache/" + sonic::startup::getClearPath(path) + ".bc";
    sonic::io::create_file_and_folder(output_file);

    std::error_code EC;
    llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);

    if (EC) {
      llvm::errs() << "Could not open file for writing bitcode: " << EC.message() << "\n";
      return;
    }

    llvm::WriteBitcodeToFile(*module, dest);
    dest.flush(); // pastikan ditulis ke disk
  }

  void SonicCodegen::saveLLReadable(const std::string& path) {
    std::string output_file = cfg::project_build + "/cache/" + sonic::startup::getClearPath(path) + ".ll";
    sonic::io::create_file_and_folder(output_file);

    std::error_code EC;
    llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);

    if (EC) {
      llvm::errs() << "Could not open file: " << EC.message();
    } else {
      module->print(dest, nullptr);
    }
  }

  void SonicCodegen::generate(SonicStmt* stmt) {
    generate_statement(stmt);
  }

  void SonicCodegen::generate_statement(SonicStmt* stmt) {
    if (!stmt) return;

    switch(stmt->kind) {
      case StmtKind::PROGRAM: {
        auto progSym = symbols->lookup_local(stmt->name);
        std::cout << "name: " << stmt->name << "\n";
        if (!progSym) return;

        auto temp = symbols;
        symbols = progSym;

        for (auto& child : stmt->body) {
          generate_statement(child.get());
        }

        symbols = temp;

        saveLLReadable(stmt->name);
        saveBitcode(stmt->name);

        break;
      }
      case StmtKind::FUNC_DECL: {
        auto funcSym = symbols->lookup_local(stmt->name);
        if (!funcSym) return;

        // auto temp = symbols;
        // symbols = funcSym;

        // symbols = temp;
        break;
      }
      default: return;
    }
  }

  void SonicCodegen::generate_expression(SonicExpr* expr) {

  }

  llvm::Type* SonicCodegen::mapping_type(SonicType* type) {
    if (!type) return llvm::Type::getVoidTy(context);

    if (type->isNullable) {
      type->isNullable = false;
      return llvm::PointerType::getUnqual(mapping_type(type));
    }

    switch(type->kind) {
      case TypeKind::I32:
        return llvm::Type::getInt32Ty(context);
      case TypeKind::I64:
        return llvm::Type::getInt64Ty(context);
      case TypeKind::I128:
        return llvm::Type::getInt128Ty(context);
      case TypeKind::F32:
        return llvm::Type::getFloatTy(context);
      case TypeKind::F64:
        return llvm::Type::getDoubleTy(context);
      case TypeKind::BOOL:
        return llvm::Type::getInt1Ty(context);
      case TypeKind::STRING:
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
      case TypeKind::CHAR:
        return llvm::Type::getInt8Ty(context);
      case TypeKind::POINTER: {
        auto pointee = mapping_type(type->elementType.get());
        return llvm::PointerType::getUnqual(pointee);
      }
      case TypeKind::REFERENCE: {
        auto pointee = mapping_type(type->elementType.get());
        return llvm::PointerType::getUnqual(pointee);
      }
      default: return llvm::Type::getVoidTy(context);
    }
  }

};
