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
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

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
//	Constant *loadString = ConstantDataArray::getString(M.getContext(), "%p\n");
	Constant *loadString = ConstantDataArray::getString(M.getContext(),
			"Total number of executed instructions are %ld\n");

	// Defining and initializing global variables corresponding to message strings
	GlobalVariable *loadStringVariable = new GlobalVariable(M,
			loadString->getType(), true, Function::InternalLinkage, loadString,
			"load_string");

	std::vector<Constant*> beginAt;
	Constant *loadStringPointer = ConstantExpr::getGetElementPtr(
			loadString->getType(), loadStringVariable, beginAt);

	Value *charPtr = builder->CreatePointerCast(loadStringPointer,
			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

	GlobalVariable *counterVariable = new GlobalVariable(M,
			IntegerType::getInt64Ty(M.getContext()), false,
			Function::InternalLinkage,
			ConstantInt::get(IntegerType::getInt64Ty(M.getContext()),
					0/*value*/, true));

	GlobalVariable *memoryInstcounterVariable = new GlobalVariable(M,
			IntegerType::getInt64Ty(M.getContext()), false,
			Function::InternalLinkage,
			ConstantInt::get(IntegerType::getInt64Ty(M.getContext()),
					0/*value*/, true));

	Constant *memoryAccessLimit = ConstantInt::get(
			IntegerType::getInt64Ty(M.getContext()), 4294967296/*value*/, true);


	//Overhead for file name argument "result.txt"
	// Defining and initializing global variables corresponding to message strings
//	Constant *fileString = ConstantDataArray::getString(M.getContext(),
//			"results.txt");
//	GlobalVariable *fileStringVariable = new GlobalVariable(M,
//			fileString->getType(), true, Function::InternalLinkage, fileString,
//			"file_string");
//
//	std::vector<Constant*> beginAtFileString;
//	Constant *fileStringPointer = ConstantExpr::getGetElementPtr(
//			fileString->getType(), fileStringVariable, beginAtFileString);
//
//	Value *filecharPtr = builder->CreatePointerCast(fileStringPointer,
//			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

	//Overhead for file mode argument "w+"
//	Constant *modeString = ConstantDataArray::getString(M.getContext(), "w+");
//	GlobalVariable *modeStringVariable = new GlobalVariable(M,
//			modeString->getType(), true, Function::InternalLinkage, modeString,
//			"mode_string");

//	std::vector<Constant*> beginAtModeString;
//	Constant *modeStringPointer = ConstantExpr::getGetElementPtr(
//			modeString->getType(), modeStringVariable, beginAtModeString);

//	Value *modecharPtr = builder->CreatePointerCast(modeStringPointer,
//			PointerType::get(Type::getInt8Ty(M.getContext()), 0));

//	Value *pFile;
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

	GlobalVariable *arrayVariableForFWrite = new GlobalVariable(M,
			IntegerType::getInt64Ty(M.getContext()), false,
			Function::InternalLinkage,
			ConstantInt::get(IntegerType::getInt64Ty(M.getContext()),
					0/*value*/, true));

	FunctionCallee CalleeF_fprintf = M.getOrInsertFunction("fprintf",
			FunctionType::get(IntegerType::getInt32Ty(M.getContext()), {
					IO_FILE_PTR_ty, builder->getInt8PtrTy() },
					true /* this is var arg func type*/));

	FunctionCallee CalleeF_fclose = M.getOrInsertFunction("fclose",
			FunctionType::get(IntegerType::getInt32Ty(M.getContext()),
					IO_FILE_PTR_ty, false /* this is var arg func type*/));

	FunctionCallee CalleeF_exit = M.getOrInsertFunction("exit",
			FunctionType::get(IntegerType::getVoidTy(M.getContext()),
					IntegerType::getInt32Ty(M.getContext()),
					false /* this is var arg func type*/));

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

//			// Returns a pointer to the first instruction in this block
//			// that is not a PHINode instruction
//			Instruction *inst = F.getEntryBlock().getFirstNonPHI();

//
//			builder->SetInsertPoint(inst);
//
//			SmallVector<Value*, 8> Args { filecharPtr, modecharPtr };
//
//			pFile = emitLibCall(LibFunc_fopen, IO_FILE_PTR_ty,
//					{ builder->getInt8PtrTy(), builder->getInt8PtrTy() }, Args,
//					*builder, &TLI, /*IsVaArgs=*/
//					true);
//
			for (BasicBlock &B : F) {
				if (isa<ReturnInst>(B.getTerminator())) {
//					SmallVector<Value*, 8> Args { pFile };
//					builder->SetInsertPoint(B.getTerminator());
//
//					builder->CreateCall(CalleeF_fclose, Args);

					builder->SetInsertPoint(B.getTerminator());
					Value *load = builder->CreateLoad(
							Type::getInt64Ty(M.getContext()), counterVariable);
					SmallVector<Value*, 8> printArgs { charPtr };
					ArrayRef<Value*> VariadicArgs = { load };
					llvm::append_range(printArgs, VariadicArgs);
					emitLibCall(LibFunc_printf, builder->getInt32Ty(),
							{ builder->getInt8PtrTy(), }, printArgs, *builder,
							&TLI, /*IsVaArgs=*/
							true);

				}
			}
//
		}

		for (BasicBlock &B : F) {
			// Handling the constant expressions, e.g. expressions as arguments to functions etc.
			// Because some getElementPtr instrcutions are inside them
			for (Instruction &I : B) {
				GetElementPtrInst *getInstr = nullptr;

				ConstantExpr *constantExpr;

				for (auto operand = I.operands().begin();
						operand != I.operands().end(); ++operand) {
//					errs() << *(operand->get()) << "\n";
					if ((constantExpr = dyn_cast<ConstantExpr>(operand->get()))
							!= nullptr) {
						Instruction *constantEprAsInst =
								constantExpr->getAsInstruction(&I);

						if ((getInstr = dyn_cast<GetElementPtrInst>(
								constantEprAsInst)) != nullptr) {

						} else {
							constantEprAsInst->eraseFromParent();

						}
					}

				}
			}
		}

		std::vector<GetElementPtrInst*> gepInstructions;
		std::vector<Instruction*> allInstructions;
//			std::vector<LoadInst*> loadInstructions;
//			std::vector<StoreInst*> storeInstructions;
		for (BasicBlock &B : F) {
			for (Instruction &I : B) {

				LoadInst *load = nullptr;
				StoreInst *store = nullptr;

				GetElementPtrInst *getInstr = nullptr;

				if ((getInstr = dyn_cast<GetElementPtrInst>(&I)) != nullptr) {
					gepInstructions.push_back(getInstr);
				}

				allInstructions.push_back(&I);

			}

		}

//		//Instruction counter
		for (auto I : allInstructions) {

			if (I->getNextNode() && !isa<PHINode>(I)) { // For some reasons PHI nodes create problems
				builder->SetInsertPoint(I->getNextNode());
				Value *load = builder->CreateLoad(
						Type::getInt64Ty(M.getContext()), counterVariable);
				Value *inc = builder->CreateAdd(load,
						ConstantInt::get(Type::getInt64Ty(M.getContext()), 1));
				builder->CreateStore(inc, counterVariable);
			}

		}

		for (auto I : gepInstructions) {
			Instruction *insertionPoint = I->getNextNode();
			builder->SetInsertPoint(insertionPoint);

			// Increment the counter
			Value *load = builder->CreateLoad(Type::getInt64Ty(M.getContext()),
					memoryInstcounterVariable);

			Value *CompareMemoryAccesses = builder->CreateICmpULT(load,
					memoryAccessLimit);
			BasicBlock *IfCompareMemoryAccesses = SplitBlock(I->getParent(),
					insertionPoint);
			BasicBlock *IfNotCompareMemoryAccesses = SplitBlock(
					IfCompareMemoryAccesses, insertionPoint);
			BasicBlock *RestCompareMemoryAccesses = SplitBlock(
					IfNotCompareMemoryAccesses, insertionPoint);

			builder->SetInsertPoint(
					cast<Instruction>(CompareMemoryAccesses)->getParent()->getTerminator());
			builder->CreateCondBr(CompareMemoryAccesses,
					IfCompareMemoryAccesses, IfNotCompareMemoryAccesses);
			cast<Instruction>(CompareMemoryAccesses)->getParent()->getTerminator()->eraseFromParent();

			builder->SetInsertPoint(IfCompareMemoryAccesses->getTerminator());

			Value *inc = builder->CreateAdd(load,
					ConstantInt::get(Type::getInt64Ty(M.getContext()), 1));
			builder->CreateStore(inc, memoryInstcounterVariable);
			// Trace the address
			Value *address = I;

			Value *stdErrorVarPtr = builder->CreateGEP(IO_FILE_PTR_ty,
					stdErrorVar, indexList);
			Value *stdErrorVarActualPtr = builder->CreateLoad(IO_FILE_PTR_ty,
					stdErrorVarPtr);

			Value *bitCasted = builder->CreatePtrToInt(address,
					builder->getInt64Ty());

			builder->CreateStore(bitCasted, arrayVariableForFWrite);

			SmallVector<Value*, 8> Args { arrayVariableForFWrite,
					ConstantInt::get(IntegerType::getInt64Ty(M.getContext()),
							8/*value*/, true), ConstantInt::get(
							IntegerType::getInt64Ty(M.getContext()), 1/*value*/,
							true), stdErrorVarActualPtr };

			builder->CreateCall(CalleeF_fwrite, Args);

//				SmallVector<Value*, 8> printArgs { charPtr };
//				ArrayRef<Value*> VariadicArgs = { address };
//				llvm::append_range(printArgs, VariadicArgs);
//				emitLibCall(LibFunc_printf, builder->getInt32Ty(),
//						{ builder->getInt8PtrTy(), }, printArgs, *builder, &TLI, /*IsVaArgs=*/
//						true);

			builder->CreateBr(RestCompareMemoryAccesses);
			IfCompareMemoryAccesses->getTerminator()->eraseFromParent();

			builder->SetInsertPoint(
					IfNotCompareMemoryAccesses->getTerminator());

			Value *instLoad = builder->CreateLoad(
					Type::getInt64Ty(M.getContext()), counterVariable);
			SmallVector<Value*, 8> printArgs { charPtr };
			ArrayRef<Value*> VariadicArgs = { instLoad };
			llvm::append_range(printArgs, VariadicArgs);
			emitLibCall(LibFunc_printf, builder->getInt32Ty(),
					{ builder->getInt8PtrTy(), }, printArgs, *builder, &TLI, /*IsVaArgs=*/
					true);

			builder->CreateCall(CalleeF_exit,
					{ ConstantInt::get(Type::getInt32Ty(M.getContext()), 1) });
		}

//			for (auto I : loadInstructions) {
//
//				builder->SetInsertPoint(I);
//
////				Value *addressByteArrayPtr = builder->CreateGEP(
////						builder->getInt64Ty(), addressByteArray, indexList);
//
//				Value *address = I->getPointerOperand();
//
//				SmallVector<Value*, 8> printArgs { charPtr };
//				ArrayRef<Value*> VariadicArgs = { address };
//				llvm::append_range(printArgs, VariadicArgs);
//				emitLibCall(LibFunc_printf, builder->getInt32Ty(),
//						{ builder->getInt8PtrTy(), }, printArgs, *builder, &TLI, /*IsVaArgs=*/
//						true);
//
//			}

	}

	return PreservedAnalyses::none();
}
