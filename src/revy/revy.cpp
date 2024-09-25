#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"
#include <cxxabi.h> // Include for __cxa_demangle

using namespace llvm;

namespace {
    // Helper function to demangle names
    std::string demangleFunctionName(const std::string &mangledName) {
        int status = 0;
        char *demangled = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
        std::string demangledName = (status == 0) ? demangled : mangledName;
        free(demangled); 
        return demangledName;
    }

Function *generateReverseFunction(Function &F, Module *M) {
    // Skip if this function is already reversed
    if (F.getName().starts_with("__undo_")) {
        errs() << "Skipping undo creation for: " << F.getName() << "\n";
        return nullptr;
    }

    // Demangle the function name for readability
    std::string demangledName = demangleFunctionName(F.getName().str());
    errs() << "Processing annotated function: " << demangledName << "\n";

    // Create a new function type, same as the original function
    FunctionType *funcType = F.getFunctionType();
    std::string undoFunctionName = "__undo_" + demangledName;

    // Check if the undo function already exists
    if (M->getFunction(undoFunctionName)) {
        errs() << "Skipping undo creation for existing function: " << undoFunctionName << "\n";
        return nullptr;
    }

    // Create a new function for the reverse logic
    Function *reverseFunc = Function::Create(
        funcType, Function::ExternalLinkage, undoFunctionName, M);
    reverseFunc->setLinkage(GlobalValue::ExternalLinkage);
    reverseFunc->setVisibility(GlobalValue::DefaultVisibility);

    // Map arguments from the original function to the reverse function
    ValueToValueMapTy VMap;
    Function::arg_iterator DestI = reverseFunc->arg_begin();
    for (const Argument &Arg : F.args()) {
        DestI->setName(Arg.getName());
        VMap[&Arg] = &*DestI++;
    }

    // Create the entry block for the new reverse function
    BasicBlock *entryBlock = BasicBlock::Create(F.getContext(), "entry", reverseFunc);
    IRBuilder<> builder(entryBlock);

    // Iterate through the original function's basic blocks and instructions
    for (auto &BB : F) {
        for (auto &I : BB) {
            // Clone and map instructions
            if (isa<AllocaInst>(&I) || isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
                // Handle memory instructions
                Instruction *newInst = I.clone();
                for (unsigned idx = 0; idx < I.getNumOperands(); ++idx) {
                    Value *operand = I.getOperand(idx);
                    if (VMap.count(operand))
                        newInst->setOperand(idx, VMap[operand]);
                    else
                        newInst->setOperand(idx, operand); // Assuming operand is a constant or global
                }
                builder.Insert(newInst);
                VMap[&I] = newInst;
            } else if (auto *binOp = dyn_cast<BinaryOperator>(&I)) {
                // Handle Add and Sub instructions
                if (binOp->getOpcode() == Instruction::Add || binOp->getOpcode() == Instruction::Sub) {
                    Value *lhs = binOp->getOperand(0);
                    Value *rhs = binOp->getOperand(1);

                    // Map operands
                    if (VMap.count(lhs))
                        lhs = VMap[lhs];
                    if (VMap.count(rhs))
                        rhs = VMap[rhs];

                    // Create reverse operation
                    Value *reverseOp = nullptr;
                    if (binOp->getOpcode() == Instruction::Add) {
                        reverseOp = builder.CreateSub(lhs, rhs, "reverseSub");
                        errs() << "Reversing Add to Sub: " << *reverseOp << "\n";
                    } else if (binOp->getOpcode() == Instruction::Sub) {
                        reverseOp = builder.CreateAdd(lhs, rhs, "reverseAdd");
                        errs() << "Reversing Sub to Add: " << *reverseOp << "\n";
                    }

                    VMap[&I] = reverseOp;
                } else {
                    // For other binary operators, clone them
                    Instruction *newInst = I.clone();
                    for (unsigned idx = 0; idx < I.getNumOperands(); ++idx) {
                        Value *operand = I.getOperand(idx);
                        if (VMap.count(operand))
                            newInst->setOperand(idx, VMap[operand]);
                        else
                            newInst->setOperand(idx, operand);
                    }
                    builder.Insert(newInst);
                    VMap[&I] = newInst;
                }
            } else if (isa<ReturnInst>(&I)) {
                // We'll add a return at the end
                continue;
            } else {
                // Clone other instructions
                Instruction *newInst = I.clone();
                for (unsigned idx = 0; idx < I.getNumOperands(); ++idx) {
                    Value *operand = I.getOperand(idx);
                    if (VMap.count(operand))
                        newInst->setOperand(idx, VMap[operand]);
                    else
                        newInst->setOperand(idx, operand);
                }
                builder.Insert(newInst);
                VMap[&I] = newInst;
            }
        }
    }

    // Add a return statement to the reverse function
    builder.CreateRetVoid();

    return reverseFunc;
}
/*
    // Map arguments from the original function to the reverse function
    ValueToValueMapTy VMap;
    Function::arg_iterator DestI = reverseFunc->arg_begin();
    for (const Argument &Arg : F.args()) {
        DestI->setName(Arg.getName());  // Preserve argument names
        VMap[&Arg] = &*DestI;           // Map original arguments to new function's arguments
        DestI++;
    }

    // Create the entry block for the new reverse function
    BasicBlock *entryBlock = BasicBlock::Create(F.getContext(), "entry", reverseFunc);
    IRBuilder<> builder(entryBlock);

    // Iterate through the original function's instructions
    for (auto &BB : F) {
        for (auto &I : BB) {
            errs() << "Processing instruction: " << I << "\n";
            if (auto *binOp = dyn_cast<BinaryOperator>(&I)) {
                // Handle only Add and Sub instructions
                if (binOp->getOpcode() == Instruction::Add || binOp->getOpcode() == Instruction::Sub) {
                    Value *lhs = binOp->getOperand(0);
                    Value *rhs = binOp->getOperand(1);

                    // Map operands to the reverse function's context
                    if (VMap.count(lhs)) {
                        lhs = VMap[lhs];
                    } else if (isa<Constant>(lhs)) {
                        // Use constants directly
                    } else {
                        errs() << "Unhandled lhs operand: " << *lhs << "\n";
                        continue;
                    }

                    if (VMap.count(rhs)) {
                        rhs = VMap[rhs];
                    } else if (isa<Constant>(rhs)) {
                        // Use constants directly
                    } else {
                        errs() << "Unhandled rhs operand: " << *rhs << "\n";
                        continue;
                    }

                    // Now create new reverse instructions in the reverse function
                    Value *reverseOp = nullptr;
                    if (binOp->getOpcode() == Instruction::Add) {
                        reverseOp = builder.CreateSub(lhs, rhs, "reverseSub");
                        errs() << "Reversing Add to Sub: " << *reverseOp << "\n";
                    } else if (binOp->getOpcode() == Instruction::Sub) {
                        reverseOp = builder.CreateAdd(lhs, rhs, "reverseAdd");
                        errs() << "Reversing Sub to Add: " << *reverseOp << "\n";
                    }

                    // Optionally, map the new instruction if it will be used later
                    VMap[&I] = reverseOp;
                }
            }
        }
    }    
/*
    // Iterate through the original function's instructions
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (auto *binOp = dyn_cast<BinaryOperator>(&I)) {
                // Handle only Add and Sub instructions
                if (binOp->getOpcode() == Instruction::Add || binOp->getOpcode() == Instruction::Sub) {
                    // Ensure both operands are recreated in the reverse function
                    Value *lhs = binOp->getOperand(0);
                    Value *rhs = binOp->getOperand(1);

                    // Recreate the operands within the reverse function
                    if (!isa<Constant>(lhs) && !VMap.count(lhs)) {
                        errs() << "Recreating lhs in reverse function\n";
                        lhs = builder.CreateAdd(builder.getInt32(0), builder.getInt32(0), "lhs");
                    }
                    if (!isa<Constant>(rhs) && !VMap.count(rhs)) {
                        errs() << "Recreating rhs in reverse function\n";
                        rhs = builder.CreateAdd(builder.getInt32(0), builder.getInt32(0), "rhs");
                    }

                    // Now create new reverse instructions in the reverse function
                    if (binOp->getOpcode() == Instruction::Add) {
                        // Reverse Add -> Sub
                        Value *reverseOp = builder.CreateSub(lhs, rhs, "reverseSub");
                        llvm::errs() << "Reversing Add to Sub: " << *reverseOp << "\n";
                    } else if (binOp->getOpcode() == Instruction::Sub) {
                        // Reverse Sub -> Add
                        Value *reverseOp = builder.CreateAdd(lhs, rhs, "reverseAdd");
                        llvm::errs() << "Reversing Sub to Add: " << *reverseOp << "\n";
                    }
                }
            }
        }
    }


    // Add a return statement to the reverse function
    //builder.CreateRetVoid();
*/




    // Function to find annotated functions
    void findAnnotatedFunctions(Module &M) {
        if (GlobalVariable *annotations = M.getGlobalVariable("llvm.global.annotations")) {
            if (ConstantArray *arr = dyn_cast<ConstantArray>(annotations->getOperand(0))) {
                for (unsigned i = 0; i < arr->getNumOperands(); ++i) {
                    ConstantStruct *annotation = dyn_cast<ConstantStruct>(arr->getOperand(i));
                    if (annotation) {
                        if (Function *annotatedFunc = dyn_cast<Function>(annotation->getOperand(0)->stripPointerCasts())) {
                            if (ConstantDataArray *annoStr = dyn_cast<ConstantDataArray>(annotation->getOperand(1)->getOperand(0))) {
                                StringRef annotationString = annoStr->getAsCString();
                                if (annotationString == "reverse") {
                                    errs() << "Found function with reverse annotation: " << annotatedFunc->getName() << "\n";
                                    // Generate reverse function
                                    generateReverseFunction(*annotatedFunc, &M);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    struct ReversePass : public PassInfoMixin<ReversePass> {
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
            Module *M = F.getParent();

            // Find annotated functions and generate their reverse counterparts
            findAnnotatedFunctions(*M);

            return PreservedAnalyses::all();
        }

        static bool isRequired() { return true; }
    };
}

// The pass plugin entry point for dynamically loaded plugins.
extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ReversePass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "reverse-pass") {
                        FPM.addPass(ReversePass());
                        return true;
                    }
                    return false;
                });
        }};
}
