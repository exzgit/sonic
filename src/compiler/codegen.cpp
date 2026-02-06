#include "codegen.h"
#include "../core/config.h"

#include <cstdint>
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
#include <string>
#include <vector>

#include "../core/io.h"
#include "ast.h"
#include "startup.h"
#include "../core/target_info.h"
#include "symbol.h"

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
    std::string output_file = cfg::project_build + "/cache/" + sonic::io::getFileNameWithoutExt(path) + ".bc";
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
    std::string output_file = cfg::project_build + "/cache/" + sonic::io::getFileNameWithoutExt(path) + ".ll";
    sonic::io::create_file_and_folder(output_file);

    std::error_code EC;
    llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_Text | llvm::sys::fs::OF_Append);

    if (EC) {
      llvm::errs() << "Could not open file: " << EC.message();
    } else {
      module->print(dest, nullptr);
    }
  }

  void SonicCodegen::generate(ast::Program* program) {
    for (auto& s : program->statements_) {
      generate_statement(s.get());
    }

    saveBitcode(sonic::io::cutPath(program->name_, "src"));
    saveLLReadable(sonic::io::cutPath(program->name_, "src"));
  }

  void SonicCodegen::generate_statement(ast::Statement* stmt) {
    if (!stmt) return;

    switch(stmt->kind_) {
      case ast::StmtKind::IMPORT: {
        for (auto& item : stmt->import_items_) {
          Symbol* fnSym = item->symbols_ ? (Symbol*)item->symbols_ : nullptr;

          if (!fnSym) {
            return;
          }

          fnSym = fnSym->ref_;

          // If no valid symbol found, skip this function
          if (!fnSym) {
            return;
          }

          // Return type
          llvm::Type* retType = llvm::Type::getVoidTy(context);
          if (fnSym->type_) retType = mapping_type(fnSym->type_);
          else {
            retType = llvm::Type::getVoidTy(context);
          }

          // Parameter types
          std::vector<llvm::Type*> paramTypes;
          for (auto& p : fnSym->params_) {
            auto t = mapping_type(p);
            if (!t) {
              t = llvm::Type::getInt64Ty(context);
            }
            paramTypes.push_back(t);
          }

          if (!retType) {
            break;
          }
          
          // Validate all parameter types before creating FunctionType
          for (size_t i = 0; i < paramTypes.size(); i++) {
            if (!paramTypes[i]) {
              paramTypes[i] = llvm::Type::getInt64Ty(context);
            }
            // Make sure it's not a FunctionType
            if (paramTypes[i]->isFunctionTy()) {
              std::cerr << "Error: parameter " << i << " cannot be a FunctionType for: " << fnSym->name_ << std::endl;
              paramTypes[i] = llvm::Type::getInt64Ty(context);
            }
          }
          
          // Return type also cannot be a FunctionType
          if (retType->isFunctionTy()) {
            retType = llvm::Type::getVoidTy(context);
          }

          auto funcType = llvm::FunctionType::get(retType, paramTypes, fnSym->variadic_);
          if (!funcType) {
            break;
          }
          
          auto func = llvm::Function::Create(funcType, (fnSym->public_ || fnSym->extern_ || fnSym->async_) ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage, fnSym->name_, module.get());
          
          if (!func) {
            break;
          }
          
          fnSym->llvm_function_= func;
          symbols->declare(fnSym);
        }
        break;
      }
      case ast::StmtKind::FUNCTION: {
        // Get symbol from AST or lookup
        Symbol* fnSym = stmt->symbols_ ? (Symbol*)stmt->symbols_ : nullptr;

        if (!fnSym) {
          fnSym = symbols->lookup(stmt->name_);
        }

        // If no valid symbol found, skip this function
        if (!fnSym) {
          return;
        }

        // Return type
        llvm::Type* retType = llvm::Type::getVoidTy(context);
        if (fnSym->type_) retType = mapping_type(fnSym->type_);
        else {
          retType = llvm::Type::getVoidTy(context);
        }

        // Parameter types
        std::vector<llvm::Type*> paramTypes;
        for (auto& p : fnSym->params_) {
          auto t = mapping_type(p);
          if (!t) {
            t = llvm::Type::getInt64Ty(context);
          }
          paramTypes.push_back(t);
        }

        if (!retType) {
          break;
        }
        
        // Validate all parameter types before creating FunctionType
        for (size_t i = 0; i < paramTypes.size(); i++) {
          if (!paramTypes[i]) {
            paramTypes[i] = llvm::Type::getInt64Ty(context);
          }
          // Make sure it's not a FunctionType
          if (paramTypes[i]->isFunctionTy()) {
            std::cerr << "Error: parameter " << i << " cannot be a FunctionType for: " << fnSym->name_ << std::endl;
            paramTypes[i] = llvm::Type::getInt64Ty(context);
          }
        }
        
        // Return type also cannot be a FunctionType
        if (retType->isFunctionTy()) {
          retType = llvm::Type::getVoidTy(context);
        }

        auto funcType = llvm::FunctionType::get(retType, paramTypes, fnSym->variadic_);
        if (!funcType) {
          break;
        }
        
        auto func = llvm::Function::Create(funcType, (fnSym->public_ || fnSym->extern_ || fnSym->async_) ? llvm::Function::ExternalLinkage : llvm::Function::InternalLinkage, fnSym->name_, module.get());
        
        if (!func) {
          break;
        }
        
        fnSym->llvm_function_= func;

        if (fnSym->decl_) break; // only declaration

        // Create entry block
        auto entry = llvm::BasicBlock::Create(context, "entry", func);
        builder->SetInsertPoint(entry);

        // Set current function symbol for local declarations
        current_function_ = fnSym;

        // Allocate and store parameters into local allocas
        size_t idx = 0;
        for (auto& arg : func->args()) {
          std::string pname = "arg" + std::to_string(idx);
          if (idx < stmt->params_.size() && stmt->params_[idx]) pname = stmt->params_[idx]->name_;

          llvm::AllocaInst* alloca = builder->CreateAlloca(arg.getType(), nullptr, pname + "_addr");
          builder->CreateStore(&arg, alloca);

          // create param symbol and declare under function symbol so expressions can find it
          auto paramSym = new Symbol(pname);
          paramSym->kind_ = SymbolKind::VARIABLE;
          paramSym->llvm_value_ = alloca;
          fnSym->declare(paramSym);

          idx++;
        }

        // Generate function body
        for (auto& b : stmt->body_) {
          generate_statement(b.get());
        }

        // Ensure function has a terminator
        if (!entry->getTerminator()) {
          if (retType->isVoidTy()) builder->CreateRetVoid();
          else builder->CreateRet(llvm::Constant::getNullValue(retType));
        }

        // clear current function
        current_function_ = nullptr;

        break;
      }
      case ast::StmtKind::VARIABLE: {
        // Top-level globals or local variable inside function
        Symbol* varSym = nullptr;
        if (stmt->symbols_) varSym = (Symbol*)stmt->symbols_;
        else varSym = symbols->lookup(stmt->name_);

        llvm::Type* ty = nullptr;
        if (stmt->type_) ty = mapping_type(stmt->type_.get());
        if (!ty && stmt->value_) ty = mapping_type(stmt->value_->type_);
        if (!ty) ty = llvm::Type::getInt64Ty(context);

        // initializer
        llvm::Constant* init = nullptr;
        if (stmt->value_) {
          auto val = generate_expression(stmt->value_.get());
          if (val && llvm::isa<llvm::Constant>(val)) init = llvm::cast<llvm::Constant>(val);
        }

        if (!current_function_) {
          // create global variable
          llvm::GlobalVariable* gv = new llvm::GlobalVariable(*module, ty, false, llvm::GlobalValue::ExternalLinkage, nullptr, stmt->name_);
          if (init) gv->setInitializer(init);
          if (varSym) varSym->llvm_value_ = gv;
        } else {
          // local variable: create alloca in current function's entry block
          llvm::Function* f = builder->GetInsertBlock()->getParent();
          llvm::IRBuilder<> tmpBuilder(&f->getEntryBlock(), f->getEntryBlock().begin());
          llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(ty, nullptr, stmt->name_ + "_addr");

          if (stmt->value_) {
            auto val = generate_expression(stmt->value_.get());
            if (val) builder->CreateStore(val, alloca);
          } else {
            // default initialize to zero
            builder->CreateStore(llvm::Constant::getNullValue(ty), alloca);
          }

          if (varSym) varSym->llvm_value_ = alloca;
          // also register symbol under current function if not present
          if (!current_function_->exists(stmt->name_)) {
            auto s = new Symbol(stmt->name_);
            s->kind_ = SymbolKind::VARIABLE;
            s->llvm_value_ = alloca;
            current_function_->declare(s);
          }
        }

        break;
      }
      case ast::StmtKind::RETURN: {
        if (stmt->value_) {
          auto retv = generate_expression(stmt->value_.get());
          if (retv) builder->CreateRet(retv);
        } else {
          builder->CreateRetVoid();
        }

        break;
      }
      case ast::StmtKind::EXPR: {
        generate_expression(stmt->value_.get());
        break;
      }
      default: return;
    }
  }

  llvm::Value* SonicCodegen::generate_expression(ast::Expression* expr) {
    if (!expr) return nullptr;

    switch (expr->kind_) {
      case ast::ExprKind::LITERAL: {
        switch (expr->literal_) {
          case ast::LiteralKind::I32: {
            int v = 0;
            std::from_chars(expr->value_.data(), expr->value_.data()+expr->value_.size(), v);
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), v, true);
          }
          case ast::LiteralKind::I64:
          case ast::LiteralKind::UNK_INT: {
            long long v = 0;
            std::from_chars(expr->value_.data(), expr->value_.data()+expr->value_.size(), v);
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), v, true);
          }
          case ast::LiteralKind::F32: {
            float f = std::stof(expr->value_);
            return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), f);
          }
          case ast::LiteralKind::F64: {
            double d = std::stod(expr->value_);
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context), d);
          }
          case ast::LiteralKind::BOOL: {
            bool b = expr->value_ == "true";
            return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), b);
          }
          case ast::LiteralKind::CHAR: {
            char c = expr->value_[0];
            return llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), c);
          }
          case ast::LiteralKind::STRING: {
            return builder->CreateGlobalStringPtr(expr->value_);
          }
          default: return nullptr;
        }
      }
      case ast::ExprKind::VARIABLE: {
        // prefer symbol pointer from semantic
        Symbol* s = expr->symbols_ ? (Symbol*)expr->symbols_ : nullptr;

        if (!s) {
          std::cout << "Warning: Variable " << expr->name_ << " has no LLVM value." << std::endl;
          return nullptr;
        }

        if (s->kind_ == SymbolKind::ALIAS) {
          s = s->ref_;
        }
        
        if (s->kind_ == SymbolKind::FUNCTION) {
          expr->symbols_ = s;
          return s->llvm_value_;
        }

        if (s->llvm_value_) {
          if (auto ai = llvm::dyn_cast<llvm::AllocaInst>(s->llvm_value_)) {
            return builder->CreateLoad(ai->getAllocatedType(), ai);
          }
          if (auto gv = llvm::dyn_cast<llvm::GlobalVariable>(s->llvm_value_)) {
            return builder->CreateLoad(gv->getValueType(), gv);
          }
          return s->llvm_value_;
        }

        std::cout << "Warning: Variable " << expr->name_ << " has no LLVM value." << std::endl;

        return nullptr;
      }
      case ast::ExprKind::CALL: {
        if (expr->callee_ == nullptr) std::cerr << "Error: CALL expression with null callee" << std::endl;
        generate_expression(expr->callee_.get());
        

        // Generate callee first
        auto fnsym = (Symbol*)expr->symbols_;
        if (fnsym == nullptr) {
          std::cerr << "Error: Unable to find function symbol for call expression" << std::endl;
          return nullptr;
        }
        llvm::Function* callee = fnsym->llvm_function_;

        if (callee == nullptr) {
          std::cerr << "Error: Unable to find function for call: " << fnsym->name_ << std::endl;
          return nullptr;
        }

        // Generate arguments
        std::vector<llvm::Value*> args;
        for (auto& a : expr->args_) {
          auto arg = generate_expression(a.get());
          if (arg) args.push_back(arg);
        }
        
        if (fnsym->variadic_) {
          args.push_back(llvm::Constant::getNullValue(llvm::Type::getInt8Ty(context)));
        }

        return builder->CreateCall(callee, args);
      }
      case ast::ExprKind::SCOPE:
      case ast::ExprKind::MEMBER: {
        if (!expr->nested_) return nullptr;
        Symbol* scopeSym = nullptr;
        if (expr->nested_->symbols_) scopeSym = (Symbol*)expr->nested_->symbols_;
        if (!scopeSym) return nullptr;
        auto child = scopeSym->lookup(expr->name_);
        if (!child) return nullptr;
        if (child->llvm_value_) {
          if (auto ai = llvm::dyn_cast<llvm::AllocaInst>(child->llvm_value_)) return builder->CreateLoad(ai->getAllocatedType(), ai);
          if (auto gv = llvm::dyn_cast<llvm::GlobalVariable>(child->llvm_value_)) return builder->CreateLoad(gv->getValueType(), gv);
          return child->llvm_value_;
        }
        return nullptr;
      }
      case ast::ExprKind::NONE: {
        return llvm::Constant::getNullValue(llvm::Type::getInt8Ty(context));
      }
      default: return nullptr;
    }
  }

  llvm::Type* SonicCodegen::mapping_type(ast::Type* type) {
    if (!type) return llvm::Type::getVoidTy(context);

    switch (type->kind_) {
      case ast::TypeKind::LITERAL: {
        switch (type->literal_) {
          case ast::LiteralKind::I32: return llvm::Type::getInt32Ty(context);
          case ast::LiteralKind::I64: return llvm::Type::getInt64Ty(context);
          case ast::LiteralKind::F32: return llvm::Type::getFloatTy(context);
          case ast::LiteralKind::F64: return llvm::Type::getDoubleTy(context);
          case ast::LiteralKind::BOOL: return llvm::Type::getInt1Ty(context);
          case ast::LiteralKind::CHAR: return llvm::Type::getInt8Ty(context);
          case ast::LiteralKind::STRING: return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
          case ast::LiteralKind::UNK_INT:
          default: return llvm::Type::getInt64Ty(context);
        }
      }
      case ast::TypeKind::VOID: return llvm::Type::getVoidTy(context);
      case ast::TypeKind::PTR:
      case ast::TypeKind::REF: return llvm::PointerType::getUnqual(mapping_type(type->nested_.get()));
      default: 
        std::cerr << "WARNING: Unknown TypeKind in mapping_type: " << (int)type->kind_ << ", defaulting to i64" << std::endl;
        return llvm::Type::getVoidTy(context);
    }
  }

};
