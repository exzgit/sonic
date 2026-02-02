#include "codegen.h"
#include "../core/config.h"

#include <iostream>
#include <llvm/ADT/Twine.h>
#include <llvm/CodeGen/MachineBasicBlock.h>
#include <llvm/IR/Constants.h>
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
#include <vector>

#include "../core/io.h"
#include "ast.h"
#include "startup.h"
#include "../core/target_info.h"

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

    std::string cpu;
    if (cfg::target_platform == llvm::sys::getDefaultTargetTriple()) {
        cpu = llvm::sys::getHostCPUName();
    } else {
        cpu = target_cpu(cfg::target_platform);
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
    dest.flush();
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

        auto temp = symbols;
        symbols = funcSym;

        std::vector<llvm::Type*> paramTypes;
        for (auto& param : funcSym->params_) {
          paramTypes.push_back(mapping_type(param));
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(mapping_type(funcSym->type_), paramTypes, funcSym->variadic_);
        llvm::Function* function = llvm::Function::Create(
          funcType,
          funcSym->public_ ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage,
          llvm::Twine(funcSym->extern_ ? funcSym->name_ : funcSym->mangleName_),
          module.get()
        );

        if (!stmt->isExtern) {
          llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, getEntryLabel(), function);
          builder->SetInsertPoint(entry);

          for (auto& child : stmt->body) {
            generate_statement(child.get());
          }

          llvm::Type* retType = function->getReturnType();

          if (!entry->getTerminator()) {
              if (retType->isIntegerTy()) {
                  // i1, i8, i32, i64, i128 dll.
                  builder->CreateRet(llvm::ConstantInt::get(retType, 0));
              } else if (retType->isFloatingPointTy()) {
                  // float / double / f16 / f128
                  builder->CreateRet(llvm::ConstantFP::get(retType, 0.0));
              } else if (retType->isPointerTy()) {
                  // bisa string, char*, struct*, dll.
                  builder->CreateRet(llvm::ConstantPointerNull::get(
                      llvm::cast<llvm::PointerType>(retType)
                  ));
              } else if (retType->isVoidTy()) {
                  builder->CreateRetVoid();
              } else {
                  // fallback: default void return
                  builder->CreateRetVoid();
              }
          }
        }

        symbols = temp;
        break;
      }
      case StmtKind::VAR_DECL: {
        auto varSym = symbols->lookup(stmt->name);
        if (!varSym) return;

        llvm::Type* llvmType = mapping_type(varSym->type_);

        llvm::Value* allocaVal = builder->CreateAlloca(llvmType, nullptr, varSym->name_);

        // simpan pointer ke symbol untuk digunakan di load/store
        varSym->llvmValue = allocaVal;

        if (stmt->expr) {
            llvm::Value* initVal = generate_expression(stmt->expr.get());
            builder->CreateStore(initVal, allocaVal);
        }
        break;
      }
      default: return;
    }
  }

  llvm::Value* SonicCodegen::generate_expression(SonicExpr* expr) {
    if (!expr || !expr->resolvedType) return nullptr;

    switch (expr->kind) {
      case ExprKind::INT:
        return llvm::ConstantInt::get(mapping_type(expr->resolvedType.get()), std::stoll(expr->value));
      case ExprKind::FLOAT:
        return llvm::ConstantFP::get(mapping_type(expr->resolvedType.get()), std::stod(expr->value));
      case ExprKind::STRING:
        return builder->CreateGlobalStringPtr(expr->value);
      case ExprKind::CHAR:
        return llvm::ConstantInt::get(mapping_type(expr->resolvedType.get()), std::stoll(expr->value));
      case ExprKind::BOOL:
        return llvm::ConstantInt::get(mapping_type(expr->resolvedType.get()), expr->value == "true" ? 1 : 0);
      case ExprKind::IDENT: {
        auto varSym = symbols->lookup(expr->name);
        assert(varSym && "Undefined variable in codegen");

        llvm::Value* ptr = varSym->llvmValue;
        assert(ptr && "Variable llvmValue is null");

        return builder->CreateLoad(mapping_type(varSym->type_), ptr, expr->name);
      }
      default:
        return nullptr;
    }
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
