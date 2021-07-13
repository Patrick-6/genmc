// For the parts of the code originating from LLVM-3.5:
//===-- Interpreter.h ------------------------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines the interpreter structure
//
//===----------------------------------------------------------------------===//

/*
 * (For the parts of the code modified from LLVM-3.5)
 *
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "config.h"

#ifndef LLI_INTERPRETER_H
#define LLI_INTERPRETER_H

#include "InterpreterEnumAPI.hpp"
#include "Config.hpp"
#include "IMMDepTracker.hpp"
#include "Library.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "View.hpp"
#include "CallInstWrapper.hpp"

#include <llvm/ADT/BitVector.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DebugInfo.h>
#if defined(HAVE_LLVM_IR_DATALAYOUT_H)
#include <llvm/IR/DataLayout.h>
#elif defined(HAVE_LLVM_DATALAYOUT_H)
#include <llvm/DataLayout.h>
#endif
#if defined(HAVE_LLVM_IR_FUNCTION_H)
#include <llvm/IR/Function.h>
#elif defined(HAVE_LLVM_FUNCTION_H)
#include <llvm/Function.h>
#endif
#if defined(HAVE_LLVM_INSTVISITOR_H)
#include <llvm/InstVisitor.h>
#elif defined(HAVE_LLVM_IR_INSTVISITOR_H)
#include <llvm/IR/InstVisitor.h>
#elif defined(HAVE_LLVM_SUPPORT_INSTVISITOR_H)
#include <llvm/Support/InstVisitor.h>
#endif
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <random>
#include <unordered_map>
#include <unordered_set>

/* Some helpers for GenericValues */
#define INT_TO_GV(typ, val)						\
({							                \
	llvm::GenericValue __ret;					\
	__ret.IntVal = llvm::APInt((typ)->getIntegerBitWidth(), (val), true); \
	__ret;								\
})

#define PTR_TO_GV(ptr)							\
({							                \
	llvm::GenericValue __ret;					\
	__ret.PointerVal = (void *) (ptr);				\
	__ret;								\
})

#define GET_ZERO_GV(typ)				\
({							\
	llvm::GenericValue __ret;			\
	if (typ->isPointerTy())				\
		__ret = PTR_TO_GV(nullptr);		\
	else						\
		__ret = INT_TO_GV(typ, 0);		\
	__ret;						\
})

class GenMCDriver;

#define INT_TO_GV(typ, val)						\
({							                \
	llvm::GenericValue __ret;					\
	__ret.IntVal = llvm::APInt((typ)->getIntegerBitWidth(), (val), true); \
	__ret;								\
})

#define PTR_TO_GV(ptr)							\
({							                \
	llvm::GenericValue __ret;					\
	__ret.PointerVal = (void *) (ptr);				\
	__ret;								\
})

namespace llvm {

class IntrinsicLowering;
struct FunctionInfo;
template<typename T> class generic_gep_type_iterator;
class ConstantExpr;
typedef generic_gep_type_iterator<User::const_op_iterator> gep_type_iterator;

typedef std::vector<GenericValue> ValuePlaneTy;

// AllocaHolder - Object to track all of the blocks of memory allocated by
// allocas in a particular stack frame. Since the driver needs to be made
// aware of the deallocs, special care needs to be taken to inform the driver
// when stack frames are popped
//
class AllocaHolder {
   std::vector<void *> allocas;

 public:
   using Allocas = std::vector<void *>;

   AllocaHolder() {}

   void add(void *mem) { allocas.push_back(mem); }
   const Allocas &get() const { return allocas; }
};

// ExecutionContext struct - This struct represents one stack frame currently
// executing.
//
struct ExecutionContext {
  Function             *CurFunction;// The currently executing function
  BasicBlock           *CurBB;      // The currently executing BB
  BasicBlock::iterator  CurInst;    // The next instruction to execute
  CallInstWrapper       Caller;     // Holds the call that called subframes.
                                    // NULL if main func or debugger invoked fn
  std::map<Value *, GenericValue> Values; // LLVM values used in this invocation
  std::vector<GenericValue>  VarArgs; // Values passed through an ellipsis
  AllocaHolder Allocas;            // Track memory allocated by alloca

  ExecutionContext() : CurFunction(nullptr), CurBB(nullptr), CurInst(nullptr) {}
};

/*
 * Thread class -- Contains information specific to each thread.
 */
class Thread {

public:
	/* Different ways a thread can be blocked */
	enum BlockageType {
		BT_NotBlocked,
		BT_ThreadJoin,
		BT_Spinloop,
		BT_FaiSpinloop,
		BT_SpinloopEnd,
		BT_LockAcq,
		BT_LockRel,
		BT_Barrier,
		BT_Cons,
		BT_Error,
		BT_User,
	};

	using MyRNG  = std::minstd_rand;
	using MyDist = std::uniform_int_distribution<MyRNG::result_type>;
	static constexpr int seed = 1995;

	int id;
	int parentId;
	llvm::Function *threadFun;
	SVal threadArg;
	std::vector<llvm::ExecutionContext> ECStack;
	llvm::ExecutionContext initSF;
	std::unordered_map<const void *, llvm::GenericValue> tls;
	unsigned int globalInstructions;
	unsigned int globalInstSnap;
	BlockageType blocked;
	MyRNG rng;
	std::vector<std::pair<int, std::string> > prefixLOC;

	void block(BlockageType t) { blocked = t; }
	void unblock() { blocked = BT_NotBlocked; }
	bool isBlocked() const { return blocked != BT_NotBlocked; }
	BlockageType getBlockageType() const { return blocked; }

	/* Useful for one-to-many instr->events correspondence */
	void takeSnapshot()   {
		globalInstSnap = globalInstructions;
	}
	void rollToSnapshot() {
		globalInstructions = globalInstSnap;
		--ECStack.back().CurInst;
	}

protected:
	friend class Interpreter;

	Thread(llvm::Function *F, int id)
		: id(id), parentId(-1), threadFun(F), initSF(), globalInstructions(0),
		  blocked(BT_NotBlocked), rng(seed) {}

	Thread(llvm::Function *F, SVal arg, int id, int pid, const llvm::ExecutionContext &SF)
		: id(id), parentId(pid), threadFun(F), threadArg(arg),
		  initSF(SF), globalInstructions(0), blocked(BT_NotBlocked), rng(seed) {}
};

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const Thread &thr);

struct ThreadInfo {
	int id;
	int parentId;
	unsigned int funId;
	SVal arg;

	ThreadInfo() = default;
	ThreadInfo(int id, int parentId, unsigned funId, SVal arg)
		: id(id), parentId(parentId), funId(funId), arg(arg) {}
};

struct EELocalState {
	SAddrAllocator alloctor;
	std::vector<Thread> threads;
	int currentThread = 0;

	EELocalState() = default;
	EELocalState(SAddrAllocator alloctor, std::vector<Thread> ts, int current)
		: alloctor(alloctor), threads(ts), currentThread(current) {}
};

struct EESharedState {
	SAddrAllocator alloctor;
	std::vector<ThreadInfo> threadInfos;

	EESharedState() = default;
	EESharedState(SAddrAllocator alloctor, std::vector<ThreadInfo> tis)
		: alloctor(alloctor), threadInfos(tis) {}
};

// Interpreter - This class represents the entirety of the interpreter.
//
class Interpreter : public ExecutionEngine, public InstVisitor<Interpreter> {
public:

  /* Pers: The state of the program -- i.e., part of the program being interpreted */
  enum ProgramState {
	  PS_Main,
	  PS_Recovery
  };

  /* The state of the current execution */
  enum ExecutionState {
	  ES_Normal,
	  ES_Replay
  };

protected:

  GenericValue ExitValue;          // The return value of the called function
  IntrinsicLowering *IL;

  /* Information about the module under test */
  std::unique_ptr<ModuleInfo> MI;

  /* List of thread-local variables, with their initializing values */
  std::unordered_map<const void *, llvm::GenericValue> threadLocalVars;

  /* Mapping between static allocation beginning and the actual addresses of
   * global variables (where their contents are stored) */
  std::unordered_map<SAddr, void *> staticValueMap;

  /* Keep all static ranges that have been allocated */
  VSet<std::pair<SAddr, SAddr> > staticAllocas;

  /* Maintain the relationship between SAddr and global variables,
   * so that we can get naming information */
  std::unordered_map<SAddr, Value *> staticNames;

  /* A tracker for dynamic allocations */
  SAddrAllocator alloctor;

  /* (Composition) pointer to the driver */
  GenMCDriver *driver;

  /* Pointer to the dependency tracker */
  std::unique_ptr<DepTracker> depTracker = nullptr;

  /* Whether the driver should be called on system errors */
  bool stopOnSystemErrors;

  /* Where system errors return values should be stored (if required) */
  SAddr errnoAddr;
  Type *errnoTyp;

  /* Information about the interpreter's state */
  ExecutionState execState = ES_Normal;
  ProgramState programState = PS_Main; /* Pers */

  /* Pers: Whether we should run a recovery procedure after the execution finishes */
  bool checkPersistency;

  /* Pers: The recovery routine to run */
  Function *recoveryRoutine = nullptr;

  // The runtime stack of executing code.  The top of the stack is the current
  // function record.
  std::vector<ExecutionContext> mainECStack;

  // AtExitHandlers - List of functions to call when the program exits,
  // registered with the atexit() library function.
  std::vector<Function*> AtExitHandlers;

public:
  explicit Interpreter(std::unique_ptr<Module> M, std::unique_ptr<ModuleInfo> MI,
		       GenMCDriver *driver, const Config *userConf);
  virtual ~Interpreter();

  /* FIXME: Document and move to .cpp */
  std::unique_ptr<EELocalState> releaseLocalState() {
	  return LLVM_MAKE_UNIQUE<EELocalState>(alloctor, threads, currentThread);
  }
  void restoreLocalState(std::unique_ptr<EELocalState> state) {
	  alloctor = std::move(state->alloctor);
	  threads = std::move(state->threads);
	  currentThread = state->currentThread;
  }

  std::unique_ptr<EESharedState> getSharedState() const {
	  auto shared = LLVM_MAKE_UNIQUE<EESharedState>();

	  shared->alloctor = alloctor;
	  for (auto &thr : threads) {
		  shared->threadInfos.emplace_back(
			  thr.id, thr.parentId, MI->idInfo.funID.at(thr.threadFun), thr.threadArg);
	  }
	  return shared;
  }
  Thread constructThreadFromInfo(const ThreadInfo &ti) {
	  Function *calledFun = const_cast<Function *>(MI->idInfo.IDFun.at(ti.funId));
	  ExecutionContext SF;

	  SF.CurFunction = calledFun;
	  SF.CurBB = &calledFun->front();
	  SF.CurInst = SF.CurBB->begin();

	  SF.Values[&*calledFun->arg_begin()] = PTR_TO_GV(ti.arg.get());
	  return createNewThread(calledFun, ti.arg, ti.id, ti.parentId, SF);
  }
  void setSharedState(std::unique_ptr<EESharedState> state) {
	  alloctor = std::move(state->alloctor);
	  threads.clear();
	  for (auto &ti : state->threadInfos)
		  threads.push_back(constructThreadFromInfo(ti));
  }

  /* Resets the interpreter at the beginning of a new execution */
  void reset();

  /* Pers: Setups the execution context for the recovery routine
   * in thread TID. Assumes that the thread has already been added
   * to the thread list. */
  void setupRecoveryRoutine(int tid);

  /* Pers: Does cleanups after the recovery routine has run */
  void cleanupRecoveryRoutine(int tid);

  /* Information about threads as well as the currently executing thread */
  std::vector<Thread> threads;
  int currentThread = 0;

  /* Creates an entry for the main() function. More information are
   * filled from the execution engine when the exploration starts */
  Thread createMainThread(llvm::Function *F);

  /* Creates a new thread, but does _not_ add it to the thread list */
  Thread createNewThread(llvm::Function *F, SVal arg, int tid, int pid,
			 const llvm::ExecutionContext &SF);

  /* Pers: Creates a thread for the recovery routine.
   * It does _not_ add it to the thread list */
  Thread createRecoveryThread(int tid);

  /* Returns the currently executing thread */
  Thread& getCurThr() { return threads[currentThread]; };

  /* Returns the thread with the specified ID (taken from the graph) */
  Thread& getThrById(int id) { return threads[id]; };

  /* Returns the stack frame of the currently executing thread */
  std::vector<ExecutionContext> &ECStack() { return getCurThr().ECStack; }

  /* Returns the current (global) position (thread, index) interpreted */
  Event getCurrentPosition() {
	  const Thread &thr = getCurThr();
	  return Event(thr.id, thr.globalInstructions);
  };

  /* Set and query interpreter's state */
  ProgramState getProgramState() const { return programState; }
  ExecutionState getExecState() const { return execState; }
  void setProgramState(ProgramState s) { programState = s; }
  void setExecState(ExecutionState s) { execState = s; }

  /* Dependency tracking */

  const DepInfo *getAddrPoDeps(unsigned int tid);
  const DepInfo *getDataDeps(unsigned int tid, Value *i);
  const DepInfo *getCtrlDeps(unsigned int tid);

  const DepInfo *getCurrentAddrDeps() const;
  const DepInfo *getCurrentDataDeps() const;
  const DepInfo *getCurrentCtrlDeps() const;
  const DepInfo *getCurrentAddrPoDeps() const;
  const DepInfo *getCurrentCasDeps() const;

  void setCurrentDeps(const DepInfo *addrDeps, const DepInfo *dataDeps,
		      const DepInfo *ctrlDeps, const DepInfo *addrPoDeps,
		      const DepInfo *casDeps);

  void updateDataDeps(unsigned int tid, Value *dst, Value *src);
  void updateDataDeps(unsigned int tid, Value *dst, const DepInfo *e);
  void updateDataDeps(unsigned int tid, Value *dst, Event e);
  void updateAddrPoDeps(unsigned int tid, Value *src);
  void updateCtrlDeps(unsigned int tid, Value *src);
  void updateFunArgDeps(unsigned int tid, Function *F);

  void clearDeps(unsigned int tid);

  /* Annotation information */

  /* Returns annotation information for the instruction I */
  const SExpr *getAnnotation(Instruction *I) const {
	  auto id = MI->idInfo.instID[I];
	  return MI->annotInfo.annotMap.count(id) ? MI->annotInfo.annotMap.at(id).get() : nullptr;
  }

  /* Returns (concretized) annotation information for the
   * current instruction (assuming we're executing it) */
  std::unique_ptr<SExpr> getCurrentAnnotConcretized();

  /* Memory pools checks */

  /* Returns true if the interpreter has allocated space for the specified static */
  bool isStaticallyAllocated(SAddr addr) const;
  void *getStaticAddr(SAddr addr) const;
  std::string getStaticName(SAddr addr) const;

  /* Returns a fresh address for a new allocation */
  SAddr getFreshAddr(unsigned int size, int alignment, Storage s, AddressSpace spc);

  /* Pers: Returns a fresh file descriptor for a new open() call (marks it as in use) */
  int getFreshFd();

  /* Pers: Marks that the file descriptor fd is in use */
  void markFdAsUsed(int fd);

  /* Pers: The interpreter reclaims a file descriptor that is no longer in use */
  void reclaimUnusedFd(int fd);

  /// runAtExitHandlers - Run any functions registered by the program's calls to
  /// atexit(3), which we intercept and store in AtExitHandlers.
  ///
  void runAtExitHandlers();

  /// create - Create an interpreter ExecutionEngine. This can never fail.
  ///
  static ExecutionEngine *create(std::unique_ptr<Module> M, std::unique_ptr<ModuleInfo> MI,
				 GenMCDriver *driver, const Config *userConf,
				 std::string *ErrorStr = nullptr);

  /// run - Start execution with the specified function and arguments.
  ///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
  virtual GenericValue runFunction(Function *F,
				   const std::vector<GenericValue> &ArgValues);
#else
  virtual GenericValue runFunction(Function *F,
				   llvm::ArrayRef<GenericValue> ArgValues);
#endif

  void *getPointerToNamedFunction(const std::string &Name,
                                  bool AbortOnFailure = true) {
    // FIXME: not implemented.
    return nullptr;
  }

  void *getPointerToNamedFunction(llvm::StringRef Name,
                                  bool AbortOnFailure = true) {
    // FIXME: not implemented.
    return nullptr;
  };

  /// recompileAndRelinkFunction - For the interpreter, functions are always
  /// up-to-date.
  ///
  void *recompileAndRelinkFunction(Function *F) {
    return getPointerToFunction(F);
  }

  /// freeMachineCodeForFunction - The interpreter does not generate any code.
  ///
  void freeMachineCodeForFunction(Function *F) { }


  /* Helper functions */
  void replayExecutionBefore(const VectorClock &before);
  bool compareValues(SSize size, SVal val1, SVal val2);
  SVal getLocInitVal(SAddr addr, SSize size);
  unsigned int getTypeSize(Type *typ) const;
  SVal executeAtomicRMWOperation(SVal val1, SVal val2, AtomicRMWInst::BinOp op);


  // Methods used to execute code:
  // Place a call on the stack
  void callFunction(Function *F, const std::vector<GenericValue> &ArgVals);
  void run();                // Execute instructions until nothing left to do

  /* Pers: Run the specified recovery routine */
  void runRecoveryRoutine();

  // Opcode Implementations
  void visitReturnInst(ReturnInst &I);
  void visitBranchInst(BranchInst &I);
  void visitSwitchInst(SwitchInst &I);
  void visitIndirectBrInst(IndirectBrInst &I);

  void visitBinaryOperator(BinaryOperator &I);
  void visitICmpInst(ICmpInst &I);
  void visitFCmpInst(FCmpInst &I);
  void visitAllocaInst(AllocaInst &I);
  void visitLoadInst(LoadInst &I);
  void visitStoreInst(StoreInst &I);
  void visitGetElementPtrInst(GetElementPtrInst &I);
  void visitPHINode(PHINode &PN) {
    llvm_unreachable("PHI nodes already handled!");
  }
  void visitTruncInst(TruncInst &I);
  void visitZExtInst(ZExtInst &I);
  void visitSExtInst(SExtInst &I);
  void visitFPTruncInst(FPTruncInst &I);
  void visitFPExtInst(FPExtInst &I);
  void visitUIToFPInst(UIToFPInst &I);
  void visitSIToFPInst(SIToFPInst &I);
  void visitFPToUIInst(FPToUIInst &I);
  void visitFPToSIInst(FPToSIInst &I);
  void visitPtrToIntInst(PtrToIntInst &I);
  void visitIntToPtrInst(IntToPtrInst &I);
  void visitBitCastInst(BitCastInst &I);
  void visitSelectInst(SelectInst &I);

  void visitCallInstWrapper(CallInstWrapper CIW);
#ifdef LLVM_HAS_CALLSITE
  void visitCallSite(CallSite  CS) { visitCallInstWrapper(CallInstWrapper(CS)); }
#else
  void visitCallBase(CallBase &CB) { visitCallInstWrapper(CallInstWrapper(CB)); }
#endif
  void visitUnreachableInst(UnreachableInst &I);

  void visitShl(BinaryOperator &I);
  void visitLShr(BinaryOperator &I);
  void visitAShr(BinaryOperator &I);

  void visitVAArgInst(VAArgInst &I);
  void visitExtractElementInst(ExtractElementInst &I);
  void visitInsertElementInst(InsertElementInst &I);
  void visitShuffleVectorInst(ShuffleVectorInst &I);

  void visitExtractValueInst(ExtractValueInst &I);
  void visitInsertValueInst(InsertValueInst &I);

  void visitAtomicCmpXchgInst(AtomicCmpXchgInst &I);
  void visitAtomicRMWInst(AtomicRMWInst &I);
  void visitFenceInst(FenceInst &I);

  bool isInlineAsm(CallInstWrapper CIW, std::string *asmStr);
  void visitInlineAsm(CallInstWrapper CIW, const std::string &asmString);

  void visitInstruction(Instruction &I) {
    errs() << I << "\n";
    llvm_unreachable("Instruction not interpretable yet!");
  }

  GenericValue callExternalFunction(Function *F,
                                    const std::vector<GenericValue> &ArgVals);
  void exitCalled(GenericValue GV);

  void addAtExitHandler(Function *F) {
    AtExitHandlers.push_back(F);
  }

  GenericValue *getFirstVarArg () {
    return &(ECStack().back ().VarArgs[0]);
  }

private:  // Helper functions
  GenericValue executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                   gep_type_iterator E, ExecutionContext &SF);

  // SwitchToNewBasicBlock - Start execution in a new basic block and run any
  // PHI nodes in the top of the block.  This is used for intraprocedural
  // control flow.
  //
  void SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF);

  void *getPointerToFunction(Function *F) { return (void*)F; }
  void *getPointerToBasicBlock(BasicBlock *BB) { return (void*)BB; }

  void initializeExecutionEngine() { }
  void initializeExternalFunctions();
  GenericValue getConstantExprValue(ConstantExpr *CE, ExecutionContext &SF);
  GenericValue getOperandValue(Value *V, ExecutionContext &SF);
  GenericValue executeTruncInst(Value *SrcVal, Type *DstTy,
                                ExecutionContext &SF);
  GenericValue executeSExtInst(Value *SrcVal, Type *DstTy,
                               ExecutionContext &SF);
  GenericValue executeZExtInst(Value *SrcVal, Type *DstTy,
                               ExecutionContext &SF);
  GenericValue executeFPTruncInst(Value *SrcVal, Type *DstTy,
                                  ExecutionContext &SF);
  GenericValue executeFPExtInst(Value *SrcVal, Type *DstTy,
                                ExecutionContext &SF);
  GenericValue executeFPToUIInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeFPToSIInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeUIToFPInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executeSIToFPInst(Value *SrcVal, Type *DstTy,
                                 ExecutionContext &SF);
  GenericValue executePtrToIntInst(Value *SrcVal, Type *DstTy,
                                   ExecutionContext &SF);
  GenericValue executeIntToPtrInst(Value *SrcVal, Type *DstTy,
                                   ExecutionContext &SF);
  GenericValue executeBitCastInst(Value *SrcVal, Type *DstTy,
                                  ExecutionContext &SF);
  GenericValue executeCastOperation(Instruction::CastOps opcode, Value *SrcVal,
                                    Type *Ty, ExecutionContext &SF);
  void returnValueToCaller(Type *RetTy, GenericValue Result);
  void popStackAndReturnValueToCaller(Type *RetTy, GenericValue Result);

  void handleSystemError(SystemError code, const std::string &msg);

  SVal getInodeTransStatus(void *inode, Type *intTyp);
  void setInodeTransStatus(void *inode, Type *intTyp, SVal status);
  SVal readInodeSizeFS(void *inode, Type *intTyp);
  void updateInodeSizeFS(void *inode, Type *intTyp, SVal newSize);
  void updateInodeDisksizeFS(void *inode, Type *intTyp, SVal newSize,
			     SVal ordDataBegin, SVal ordDataEnd);
  void writeDataToDisk(void *buf, int bufOffset, void *inode, int inodeOffset,
		       int count, Type *dataTyp);
  void readDataFromDisk(void *inode, int inodeOffset, void *buf, int bufOffset,
			int count, Type *dataTyp);
  void updateDirNameInode(const char *name, Type *intTyp, SVal inode);

  SVal checkOpenFlagsFS(SVal &flags, Type *intTyp);
  SVal executeInodeLookupFS(const char *name, Type *intTyp);
  SVal executeInodeCreateFS(const char *name, Type *intTyp);
  SVal executeLookupOpenFS(const char *file, SVal &flags, Type *intTyp);
  SVal executeOpenFS(const char *file, SVal flags, SVal inode, Type *intTyp);

  void executeReleaseFileFS(void *fileDesc, Type *intTyp);
  SVal executeCloseFS(SVal fd, Type *intTyp);
  SVal executeRenameFS(const char *oldpath, SVal oldInode, const char *newpath,
		       SVal newInode, Type *intTyp);
  SVal executeLinkFS(const char *newpath, SVal oldInode, Type *intTyp);
  SVal executeUnlinkFS(const char *pathname, Type *intTyp);


  SVal executeTruncateFS(SVal inode, SVal length, Type *intTyp);
  SVal executeReadFS(void *file, Type *intTyp, void *buf,
		     Type *bufElemTyp, SVal offset, SVal count);
  void zeroDskRangeFS(void *inode, SVal start, SVal end, Type *writeIntTyp);
  SVal executeWriteChecksFS(void *inode, Type *intTyp, SVal flags,
			    SVal offset, SVal count, SVal &wOffset);
  bool shouldUpdateInodeDisksizeFS(void *inode, Type *intTyp, SVal size,
				   SVal offset, SVal count, SVal &dSize);
  SVal executeBufferedWriteFS(void *inode, Type *intTyp, void *buf,
			      Type *bufElemTyp, SVal wOffset, SVal count);
  SVal executeWriteFS(void *file, Type *intTyp, void *buf, Type *bufElemTyp,
		      SVal offset, SVal count);
  SVal executeLseekFS(void *file, Type *intTyp, SVal offset, SVal whence);
  void executeFsyncFS(void *inode, Type *intTyp);


  /* Custom Opcode Implementations */
  void callAssertFail(Function *F, const std::vector<GenericValue> &ArgVals);
  void callRecAssertFail(Function *F, const std::vector<GenericValue> &ArgVals);
  void callSpinStart(Function *F, const std::vector<GenericValue> &ArgVals);
  void callSpinEnd(Function *F, const std::vector<GenericValue> &ArgVals);
  void callPotentialSpinEnd(Function *F, const std::vector<GenericValue> &ArgVals);
  void callEndLoop(Function *F, const std::vector<GenericValue> &ArgVals);
  void callAssume(Function *F, const std::vector<GenericValue> &ArgVals);
  void callNondetInt(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMalloc(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMallocAligned(Function *F, const std::vector<GenericValue> &ArgVals);
  void callFree(Function *F, const std::vector<GenericValue> &ArgVals);
  void callThreadSelf(Function *F, const std::vector<GenericValue> &ArgVals);
  void callThreadCreate(Function *F, const std::vector<GenericValue> &ArgVals);
  void callThreadJoin(Function *F, const std::vector<GenericValue> &ArgVals);
  void callThreadExit(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMutexInit(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMutexLock(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMutexUnlock(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMutexTrylock(Function *F, const std::vector<GenericValue> &ArgVals);
  void callMutexDestroy(Function *F, const std::vector<GenericValue> &ArgVals);
  void callBarrierInit(Function *F, const std::vector<GenericValue> &ArgVals);
  void callBarrierWait(Function *F, const std::vector<GenericValue> &ArgVals);
  void callBarrierDestroy(Function *F, const std::vector<GenericValue> &ArgVals);
  void callReadFunction(const Library &lib, const LibMem &m, Function *F,
			const std::vector<GenericValue> &ArgVals);
  void callWriteFunction(const Library &lib, const LibMem &m, Function *F,
			 const std::vector<GenericValue> &ArgVals);
  void callOpenFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callCreatFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callCloseFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callRenameFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callLinkFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callUnlinkFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callTruncateFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callReadFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callWriteFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callSyncFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callFsyncFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callPreadFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callPwriteFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callLseekFS(Function *F, const std::vector<GenericValue> &ArgVals);
  void callPersBarrierFS(Function *F, const std::vector<GenericValue> &ArgVals);

  const Library *isUserLibCall(Function *F);
  void callUserLibFunction(const Library *lib, Function *F, const std::vector<GenericValue> &ArgVals);
  void callInternalFunction(Function *F, const std::vector<GenericValue> &ArgVals);

  void freeAllocas(const AllocaHolder &allocas) const;

  /* Collects the addresses (and some naming information) for all variables with
   * static storage. Also calculates the starting address of the allocation pool */
  void collectStaticAddresses(Module *M);

  /* Sets up how some errors will be reported to the user */
  void setupErrorPolicy(Module *M, const Config *userConf);

  /* Pers: Sets up information about the modeled filesystem */
  void setupFsInfo(Module *M, const Config *userConf);

  /* Update naming information */

  void updateUserTypedVarName(char *ptr, unsigned int typeSize, Storage s,
			      Value *v, const std::string &prefix,
			      const std::string &internal);
  void updateUserUntypedVarName(char *ptr, unsigned int typeSize, Storage s,
				Value *v, const std::string &prefix,
				const std::string &internal);
  void updateInternalVarName(char *ptr, unsigned int typeSize, Storage s,
			     Value *v, const std::string &prefix,
			     const std::string &internal);

  /* Gets naming information for value V (or value with key KEY), if it is
   * an internal variable with no value correspondence */
  NameInfo *getVarNameInfo(Value *v, Storage s, AddressSpace spc,
			   const VariableInfo::InternalKey &key = {});

  /* Pers: Returns the address of the file description referenced by FD */
  void *getFileFromFd(int fd) const;

  /* Pers: Tracks that the address of the file description of FD is FILEADDR */
  void setFdToFile(int fd, void *fileAddr);

  /* Pers: Directory operations */
  void *getDirInode() const;
  void *getInodeAddrFromName(const char *filename) const;

};

} // End llvm namespace

#endif
