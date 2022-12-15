//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MemoryTracer.h"
#include "llvm/IR/Instructions.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BuildLibCalls.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/GlobalVariable.h"

#include <stdio.h>

using namespace llvm;

Value *pFile;
StructType *IO_FILE_ty;
PointerType *IO_FILE_PTR_ty;

static Value* emitLibCall(LibFunc TheLibFunc, Type *ReturnType,
		ArrayRef<Type*> ParamTypes, ArrayRef<Value*> Operands, IRBuilderBase &B,
		const TargetLibraryInfo *TLI, bool IsVaArgs = false) {
	Module *M = B.GetInsertBlock()->getModule();
	if (!isLibFuncEmittable(M, TLI, TheLibFunc))
		return nullptr;

	StringRef FuncName = TLI->getName(TheLibFunc);
	FunctionType *FuncType = FunctionType::get(ReturnType, ParamTypes,
			IsVaArgs);
	FunctionCallee Callee = getOrInsertLibFunc(M, *TLI, TheLibFunc, FuncType);
	inferNonMandatoryLibFuncAttrs(M, FuncName, *TLI);
	CallInst *CI = B.CreateCall(Callee, Operands, FuncName);
	if (const Function *F = dyn_cast<Function>(
			Callee.getCallee()->stripPointerCasts()))
		CI->setCallingConv(F->getCallingConv());
	return CI;
}

PreservedAnalyses MemoryTracerPass::run(Module &M, ModuleAnalysisManager &MAM) {

	IRBuilder<> *builder = new IRBuilder<>(M.getContext());

//	errs() << M.getName() << "\n";

//	FunctionCallee CalleeF = M.getOrInsertFunction("printf",
//			FunctionType::get(IntegerType::getInt32Ty(M.getContext()),
//					PointerType::get(Type::getInt8Ty(M.getContext()), 0),
//					true /* this is var arg func type*/));
//
//	// Creating messages constants, prinf("%p\n", ptrValue);
	Constant *loadString = ConstantDataArray::getString(M.getContext(), "%p\n");

	// Defining and initializing global variables corresponding to message strings
	GlobalVariable *loadStringVariable = new GlobalVariable(M,
			loadString->getType(), true, Function::InternalLinkage, loadString,
			"load_string");

	std::vector<Constant*> beginAt;
	Constant *loadStringPointer = ConstantExpr::getGetElementPtr(
			loadString->getType(), loadStringVariable, beginAt);

	Value *charPtr = builder->CreatePointerCast(loadStringPointer,
			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

	// Defining and initializing global variables corresponding to message strings
	Constant *fileString = ConstantDataArray::getString(M.getContext(),
			"results.txt");
	GlobalVariable *fileStringVariable = new GlobalVariable(M,
			fileString->getType(), true, Function::InternalLinkage, fileString,
			"file_string");

	std::vector<Constant*> beginAtFileString;
	Constant *fileStringPointer = ConstantExpr::getGetElementPtr(
			fileString->getType(), fileStringVariable, beginAtFileString);

	Value *filecharPtr = builder->CreatePointerCast(fileStringPointer,
			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

	Constant *modeString = ConstantDataArray::getString(M.getContext(), "w+");
	GlobalVariable *modeStringVariable = new GlobalVariable(M,
			modeString->getType(), true, Function::InternalLinkage, modeString,
			"mode_string");

	std::vector<Constant*> beginAtModeString;
	Constant *modeStringPointer = ConstantExpr::getGetElementPtr(
			modeString->getType(), modeStringVariable, beginAtModeString);

	Value *modecharPtr = builder->CreatePointerCast(modeStringPointer,
			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

	Value *pFile;
	StructType *IO_FILE_ty;
	PointerType *IO_FILE_PTR_ty;

	IO_FILE_ty = StructType::create(M.getContext(), "struct.__sFILE");
	IO_FILE_PTR_ty = PointerType::getUnqual(IO_FILE_ty);

//	pFile = new GlobalVariable(IO_FILE_PTR_ty, false,
//			GlobalValue::ExternalLinkage , nullptr, "__stderrp",
//			GlobalValue::NotThreadLocal, 0, true);

//	pFile = M.getOrInsertGlobal("__stderrp", IO_FILE_ty);
	/// Global values are always pointers.
	GlobalVariable *stdErrorVar = dyn_cast<GlobalVariable>(
			M.getOrInsertGlobal("__stderrp", IO_FILE_PTR_ty));
	stdErrorVar->setLinkage(GlobalValue::ExternalLinkage);

	Value *indexList[1] = { ConstantInt::get(
			IntegerType::getInt32Ty(M.getContext()), 0/*value*/, true) };

//	std::vector<Value*> args;
//	args.push_back(charPtr);∫

	FunctionCallee CalleeF_fprintf = M.getOrInsertFunction("fprintf",
			FunctionType::get(IntegerType::getInt32Ty(M.getContext()), {
					IO_FILE_PTR_ty, builder->getInt8PtrTy() },
					true /* this is var arg func type*/));

	FunctionCallee CalleeF_fclose = M.getOrInsertFunction("fclose",
			FunctionType::get(IntegerType::getInt32Ty(M.getContext()),
					IO_FILE_PTR_ty, false /* this is var arg func type*/));

	auto &FAM =
			MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

	for (Function &F : M) {

		const TargetLibraryInfo &TLI = FAM.getResult<TargetLibraryAnalysis>(F);
		Type *SizeTTy = builder->getIntNTy(TLI.getSizeTSize(M));

		FunctionCallee CalleeF_fwrite = M.getOrInsertFunction("fwrite",
				FunctionType::get(SizeTTy,
						{ PointerType::getInt64PtrTy(M.getContext()),
								builder->getInt64Ty(), builder->getInt64Ty(),
								IO_FILE_PTR_ty }, false /* this is var arg func type*/
						));

		// We know we've encountered a main module∫
		// we get the entry block of it
		if ((F.getName() == "main")) {
			// Returns a pointer to the first instruction in this block
			// that is not a PHINode instruction
			Instruction *inst = F.getEntryBlock().getFirstNonPHI();

			IRBuilder<> *builder = new IRBuilder<>(M.getContext());
			builder->SetInsertPoint(inst);

			SmallVector<Value*, 8> Args { filecharPtr, modecharPtr };

			pFile = emitLibCall(LibFunc_fopen, IO_FILE_PTR_ty,
					{ builder->getInt8PtrTy(), builder->getInt8PtrTy() }, Args,
					*builder, &TLI, /*IsVaArgs=*/
					true);

			for (BasicBlock &B : F) {
				if (isa<ReturnInst>(B.getTerminator())) {
					SmallVector<Value*, 8> Args { pFile };
					builder->SetInsertPoint(B.getTerminator());

					builder->CreateCall(CalleeF_fclose, Args);

				}
			}

		}

		for (BasicBlock &B : F) {

			Instruction *inst = F.getEntryBlock().getFirstNonPHI();

			builder->SetInsertPoint(inst);

			Value *addressByteArray = builder->CreateAlloca(
					builder->getInt64Ty(),
					ConstantInt::get(IntegerType::getInt32Ty(M.getContext()),
							1/*value*/, true));
			Value *addressByteArrayPtr = builder->CreateGEP(
					builder->getInt64Ty(), addressByteArray, indexList);

			for (Instruction &I : B) {

//				errs() << I << "\n";

				LoadInst *load = nullptr;
				StoreInst *store = nullptr;

				if ((load = dyn_cast<LoadInst>(&I)) != nullptr) { // Check if the current instruction is a load

					Value *address = load->getPointerOperand(); // Getting source address

					builder->SetInsertPoint(&I);

					Value *stdErrorVarPtr = builder->CreateGEP(IO_FILE_PTR_ty,
							stdErrorVar, indexList);
					Value *stdErrorVarActualPtr = builder->CreateLoad(
							IO_FILE_PTR_ty, stdErrorVarPtr);

//					SmallVector<Value*, 8> Args { stdErrorVarActualPtr, charPtr };
//					ArrayRef<Value*> VariadicArgs = { address };
//					llvm::append_range(Args, VariadicArgs);
//
//					builder->CreateCall(CalleeF_fprintf, Args);

					Value *bitCasted = builder->CreatePtrToInt(address,
							builder->getInt64Ty());

					builder->CreateStore(bitCasted, addressByteArrayPtr);

					SmallVector<Value*, 8> Args { addressByteArrayPtr,
							ConstantInt::get(
									IntegerType::getInt64Ty(M.getContext()),
									8/*value*/, true), ConstantInt::get(
									IntegerType::getInt64Ty(M.getContext()),
									1/*value*/, true), stdErrorVarActualPtr };

					builder->CreateCall(CalleeF_fwrite, Args);

					SmallVector<Value*, 8> printArgs { charPtr };
					ArrayRef<Value*> VariadicArgs = { address };
					llvm::append_range(printArgs, VariadicArgs);
					emitLibCall(LibFunc_printf, builder->getInt32Ty(),
							{ builder->getInt8PtrTy(), }, printArgs, *builder, &TLI, /*IsVaArgs=*/
							true);

//					emitLibCall(LibFunc_strlen, SizeTTy,
//							builder->getInt8PtrTy(), charPtr, *builder, &TLI);

//					errs() << CalleeF.getFunctionType()->getNumParams() << "\n";

//					std::string type_str;
//					llvm::raw_string_ostream rso(type_str);
//					CalleeF.getFunctionType()->getParamType(0)->print(rso);
//					errs() << rso.str() << "\n";
//
//					loadStringPointer->getType()->print(rso);
//					errs() << rso.str() << "\n";
//
//					builder->CreateCall(CalleeF, args);

				} else if ((store = dyn_cast<StoreInst>(&I)) != nullptr) { // Check if the current instruction is a store

					Value *value = store->getValueOperand(); // Getting the value to be stored at given address
					Value *address = store->getPointerOperand(); // Getting source address

//					errs() << address << "\n";
				}
			}
		}
	}

	return PreservedAnalyses::none();
}
