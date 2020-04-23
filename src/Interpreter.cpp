// For the parts of the code originating from LLVM-3.5:
//===- Interpreter.cpp - Top-Level LLVM Interpreter Implementation --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LLVMLICENSE for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the top-level functionality for the LLVM interpreter.
// This interpreter is designed to be a very simple, portable, inefficient
// interpreter.
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

#include "Config.hpp"
#include "Error.hpp"
#include "Interpreter.h"
#include <llvm/CodeGen/IntrinsicLowering.h>
#if defined(HAVE_LLVM_IR_DERIVEDTYPES_H)
#include <llvm/IR/DerivedTypes.h>
#elif defined(HAVE_LLVM_DERIVEDTYPES_H)
#include <llvm/DerivedTypes.h>
#endif
#if defined(HAVE_LLVM_IR_MODULE_H)
#include <llvm/IR/Module.h>
#elif defined(HAVE_LLVM_MODULE_H)
#include <llvm/Module.h>
#endif
#include <cstring>

using namespace llvm;

extern "C" void LLVMLinkInInterpreter() { }

/// create - Create a new interpreter object.  This can never fail.
///
ExecutionEngine *Interpreter::create(Module *M, VariableInfo &&VI, DirInode &&DI,
				     GenMCDriver *driver, const Config *userConf,
				     std::string* ErrStr) {
  // Tell this Module to materialize everything and release the GVMaterializer.
#ifdef LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_ERRORCODE_BOOL
  if (std::error_code EC = M->materializeAllPermanently()) {
    if (ErrStr)
      *ErrStr = EC.message();
    // We got an error, just return 0
    return nullptr;
  }
#elif defined LLVM_MODULE_MATERIALIZE_ALL_PERMANENTLY_BOOL_STRPTR
  if (M->MaterializeAllPermanently(ErrStr)){
    // We got an error, just return 0
    return nullptr;
  }
#elif defined LLVM_MODULE_MATERIALIZE_ALL_LLVM_ERROR
  if (Error Err = M->materializeAll()) {
    std::string Msg;
    handleAllErrors(std::move(Err), [&](ErrorInfoBase &EIB) {
      Msg = EIB.message();
    });
    if (ErrStr)
      *ErrStr = Msg;
    // We got an error, just return 0
    return nullptr;
  }
#else
  if(std::error_code EC = M->materializeAll()){
    if(ErrStr)
      *ErrStr = EC.message();
    // We got an error, just return 0
    return nullptr;
  }
#endif

  return new Interpreter(M, std::move(VI), std::move(DI), driver, userConf);
}

/* Thread::seed is ODR-used -- we need to provide a definition (C++14) */
constexpr int Thread::seed;

llvm::raw_ostream& llvm::operator<<(llvm::raw_ostream &s, const Thread &thr)
{
	return s << "<" << thr.parentId << ", " << thr.id << ">"
		 << " " << thr.threadFun->getName().str();
}

/* Resets the interpreter for a new exploration */
void Interpreter::reset()
{
	/*
	 * Make sure that all execution stacks are empty since there may
	 * have been a failed assume on some thread and a join waiting on
	 * that thread (joins do not empty ECStacks)
	 */
	currentThread = 0;
	for (auto i = 0u; i < threads.size(); i++) {
		threads[i].ECStack = {};
		threads[i].tls = threadLocalVars;
		threads[i].isBlocked = false;
		threads[i].globalInstructions = 0;
		threads[i].rng.seed(Thread::seed);
		clearDeps(i);
	}
}

void Interpreter::setupRecoveryRoutine(int tid)
{
	BUG_ON(tid >= threads.size());
	currentThread = tid;

	/* Only fill the stack if a recovery routine actually exists... */
	if (recoveryRoutine)
		callFunction(recoveryRoutine, {});
	return;
}

void Interpreter::cleanupRecoveryRoutine(int tid)
{
	/* Nothing to do -- yet */
	currentThread = 0;
	return;
}

/* Creates an entry for the main() function */
Thread Interpreter::createMainThread(llvm::Function *F)
{
	Thread thr(F, 0);
	thr.tls = threadLocalVars;
	return thr;
}

/* Creates an entry for another thread */
Thread Interpreter::createNewThread(llvm::Function *F, int tid, int pid,
				    const llvm::ExecutionContext &SF)
{
	Thread thr(F, tid, pid, SF);
	thr.ECStack.push_back(SF);
	thr.tls = threadLocalVars;
	return thr;
}

Thread Interpreter::createRecoveryThread(int tid)
{
	Thread rec(recoveryRoutine, tid);
	rec.tls = threadLocalVars;
	return rec;
}

/* Returns the (source-code) variable name corresponding to "addr" */
std::string Interpreter::getVarName(const void *addr)
{
	if (isStack(addr))
		return varNames[static_cast<int>(AddressSpace::Stack)][addr];
	if (isStatic(addr))
		return varNames[static_cast<int>(AddressSpace::Static)][addr];
	if (isInternal(addr))
		return varNames[static_cast<int>(AddressSpace::Internal)][addr];
	return "";
}

bool Interpreter::isInternal(const void *addr)
{
	return alloctor.isAllocated(addr, AddressSpace::Internal);
}

bool Interpreter::isStatic(const void *addr)
{
	return varNames[static_cast<int>(AddressSpace::Static)].count(addr);
}

bool Interpreter::isStack(const void *addr)
{
	return alloctor.isAllocated(addr, AddressSpace::Stack);
}

bool Interpreter::isHeap(const void *addr)
{
	return alloctor.isAllocated(addr, AddressSpace::Heap);
}

bool Interpreter::isDynamic(const void *addr)
{
	return isStack(addr) || isHeap(addr);
}

bool Interpreter::isShared(const void *addr)
{
	return isStatic(addr) || isDynamic(addr);
}

/* Returns a fresh address to be used from the interpreter */
void *Interpreter::getFreshAddr(unsigned int size, AddressSpace spc)
{
	return alloctor.allocate(size, spc);
}

void Interpreter::trackAlloca(const void *addr, unsigned int size, AddressSpace spc)
{
	/* We cannot call updateVarNameInfo just yet, since we might be simply
	 * restoring a prefix, and cannot get the respective Value. The naming
	 * information will be updated from the interpreter */
	alloctor.track(addr, size, spc);
	return;
}

void Interpreter::untrackAlloca(const void *addr, unsigned int size, AddressSpace spc)
{
	alloctor.untrack(addr, size, spc);
	eraseVarNameInfo((char *) addr, size, spc);
	return;
}

#ifndef LLVM_BITVECTOR_HAS_FIND_FIRST_UNSET
int my_find_first_unset(const llvm::BitVector &bv)
{
	for (auto i = 0u; i < bv.size(); i++)
		if (bv[i] == 0)
			return i;
	return -1;
}
#endif /* LLVM_BITVECTOR_HAS_FIND_FIRST_UNSET */

int Interpreter::getFreshFd()
{
#ifndef LLVM_BITVECTOR_HAS_FIND_FIRST_UNSET
	int fd = my_find_first_unset(fds);
#else
	int fd = fds.find_first_unset();
#endif

	/* If no available descriptor found, return -1 */
	if (fd == -1)
		return -1;

	/* Otherwise, mark the file descriptor as used */
	markFdAsUsed(fd);
	return fd;
}

void Interpreter::markFdAsUsed(int fd)
{
	fds.set(fd);
}

void Interpreter::reclaimUnusedFd(int fd)
{
	fds.reset(fd);
}

void *Interpreter::getFileFromFd(int fd)
{
	return fdToFile[fd];
}

void Interpreter::setFdToFile(int fd, void *fileAddr)
{
	fdToFile[fd] = fileAddr;
}

unsigned int Interpreter::getInodeAllocSize(Type *intTyp) const
{
	/* We assume that an inode is of the following form:
	 *
	 *     struct inode {
	 *             pthread_mutex_t lock; // equivalent to int
	 *             int isize;
	 *             char data[maxFileSize];
	 *     };
	 */
	auto &ctx = intTyp->getContext();
	auto charPtrType = Type::getInt8PtrTy(ctx);

	auto intSize = getTypeSize(intTyp);
	auto charPtrSize = getTypeSize(charPtrType);

	return 2 * intSize + charPtrSize * maxFileSize;
}

unsigned int Interpreter::getFileAllocSize(Type *intTyp) const
{
	/* We assume that the file struct has the following form:
	 *
	 *     struct file {
	 *             pthread_mutex_t lock; // equivalent to int
	 *             struct inode *inode; // equivalent to int *
	 *             int offset;
	 *     };
	 */
	auto &ctx = intTyp->getContext();
	auto intPtrType = Type::getIntNPtrTy(ctx, intTyp->getIntegerBitWidth());

	auto intSize = getTypeSize(intTyp);
	auto intPtrSize = getTypeSize(intPtrType);

	return 2 * intSize + intPtrSize;
}

void collectUnnamedGlobalAddress(Value *v, char *ptr, unsigned int typeSize,
				 std::unordered_map<const void *, std::string> &vars)
{
	BUG_ON(!isa<GlobalVariable>(v));
	auto gv = static_cast<GlobalVariable *>(v);

	/* Exit if it is a private variable; it is not accessible in the program */
	if (gv->hasPrivateLinkage())
		return;

	/* Otherwise, collect the addresses anyway and use a default name */
	WARN_ONCE("name-info", ("Inadequate naming info for variable " +
				v->getName() + ".\nPlease submit a bug report to "
				PACKAGE_BUGREPORT "\n"));
	for (auto i = 0u; i < typeSize; i++) {
		vars[ptr + i] = v->getName();
	}
	return;
}

void updateVarInfoHelper(char *ptr, unsigned int typeSize,
			 std::unordered_map<const void *, std::string> &vars,
			 VariableInfo::NameInfo &vi, const std::string &baseName)
{
	/* If there are no info for the variable, just use the base name.
	 * (Except for internal types, this should normally be handled by the caller) */
	if (vi.empty()) {
		for (auto j = 0u; j < typeSize; j++)
			vars[ptr + j] = baseName;
		return;
	}

	/* If there is no name for the beginning of the block, use a default one */
	if (vi[0].first != 0) {
		WARN_ONCE("name-info", ("Inadequate naming info for variable " +
					baseName + ".\nPlease submit a bug report to "
					PACKAGE_BUGREPORT "\n"));
		for (auto j = 0u; j < vi[0].first; j++)
			vars[ptr + j] = baseName;
	}
	for (auto i = 0u; i < vi.size() - 1; i++) {
		for (auto j = 0u; j < vi[i + 1].first - vi[i].first; j++)
			vars[ptr + vi[i].first + j] = baseName + vi[i].second;
	}
	auto &last = vi.back();
	for (auto j = 0u; j < typeSize - last.first; j++)
		vars[ptr + last.first + j] = baseName + last.second;
	return;
}

void Interpreter::updateTypedVarNameInfo(char *ptr, unsigned int typeSize, AddressSpace spc,
					 Value *v, const std::string &prefix,
					 const std::string &internal)
{
	auto &vars = varNames[static_cast<int>(spc)];
	auto &vi = (spc == AddressSpace::Stack) ? VI.localInfo[v] : VI.globalInfo[v];

	if (vi.empty()) {
		/* If it is not a local value, then we should collect the address
		 * anyway, since globalVars will be used to check whether some
		 * access accesses a global variable */
		if (spc == AddressSpace::Static)
			collectUnnamedGlobalAddress(v, ptr, typeSize, vars);
		return;
	}
	updateVarInfoHelper(ptr, typeSize, vars, vi, v->getName());
	return;
}

void Interpreter::updateUntypedVarNameInfo(char *ptr, unsigned int typeSize, AddressSpace spc,
					   Value *v, const std::string &prefix,
					   const std::string &internal)
{
	/* FIXME: Does nothing now */
	return;
}

void Interpreter::updateInternalVarNameInfo(char *ptr, unsigned int typeSize, AddressSpace spc,
					    Value *v, const std::string &prefix,
					    const std::string &internal)
{
	auto &vars = varNames[static_cast<int>(spc)];
	auto &vi = VI.internalInfo[internal]; /* should be the name for internals */
	BUG_ON(spc != AddressSpace::Internal);

	updateVarInfoHelper(ptr, typeSize, vars, vi, prefix);
	return;
}

void Interpreter::updateVarNameInfo(char *ptr, unsigned int typeSize,
				    AddressSpace spc, Value *v,
				    const std::string &prefix, const std::string &extra)
{
	switch (spc) {
	case AddressSpace::Static:
	case AddressSpace::Stack:
		updateTypedVarNameInfo(ptr, typeSize, spc, v, prefix, extra);
		return;
	case AddressSpace::Heap:
		updateUntypedVarNameInfo(ptr, typeSize, spc, v, prefix, extra);
		return;
	case AddressSpace::Internal:
		updateInternalVarNameInfo(ptr, typeSize, spc, v, prefix, extra);
		return;
	default:
		BUG();
	}
	return;
}

void Interpreter::eraseVarNameInfo(char *addr, unsigned int size, AddressSpace spc)
{
	for (auto i = 0u; i < size; i++)
		varNames[static_cast<int>(spc)].erase(addr + i);
	return;
}

/* Updates the names for all global variables, and calculates the
 * starting address of the allocation pool */
void Interpreter::collectGlobalAddresses(Module *M)
{
	/* Collect all global and thread-local variables */
	char *allocBegin = nullptr;
	for (auto &v : M->getGlobalList()) {
		char *ptr = static_cast<char *>(GVTOP(getConstantValue(&v)));
		unsigned int typeSize =
		        TD.getTypeAllocSize(v.getType()->getElementType());

		/* The allocation pool will point just after the static address */
		if (!allocBegin || ptr > allocBegin)
			allocBegin = ptr + typeSize;

		/* Record whether this is a thread local variable or not */
		if (v.isThreadLocal()) {
			for (auto i = 0u; i < typeSize; i++)
				threadLocalVars[ptr + i] = getConstantValue(v.getInitializer());
			continue;
		}

		/* Update the name for this global. We cheat a bit since we will use this
		 * to indicate whether this is an allocated static address (see isStatic()) */
		updateVarNameInfo(ptr, typeSize, AddressSpace::Static, &v);
	}
	/* The allocator will start giving out addresses greater than the maximum static address */
	if (allocBegin)
		alloctor.initPoolAddress(allocBegin);
}

const DepInfo *Interpreter::getAddrPoDeps(unsigned int tid)
{
	if (!depTracker)
		return nullptr;
	return depTracker->getAddrPoDeps(tid);
}

const DepInfo *Interpreter::getDataDeps(unsigned int tid, Value *i)
{
	if (!depTracker)
		return nullptr;
	return depTracker->getDataDeps(tid, i);
}

const DepInfo *Interpreter::getCtrlDeps(unsigned int tid)
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCtrlDeps(tid);
}

const DepInfo *Interpreter::getCurrentAddrDeps() const
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCurrentAddrDeps();
}

const DepInfo *Interpreter::getCurrentDataDeps() const
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCurrentDataDeps();
}

const DepInfo *Interpreter::getCurrentCtrlDeps() const
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCurrentCtrlDeps();
}

const DepInfo *Interpreter::getCurrentAddrPoDeps() const
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCurrentAddrPoDeps();
}

const DepInfo *Interpreter::getCurrentCasDeps() const
{
	if (!depTracker)
		return nullptr;
	return depTracker->getCurrentCasDeps();
}

void Interpreter::setCurrentDeps(const DepInfo *addr, const DepInfo *data,
				 const DepInfo *ctrl, const DepInfo *addrPo,
				 const DepInfo *cas)
{
	if (!depTracker)
		return;

	depTracker->setCurrentAddrDeps(addr);
	depTracker->setCurrentDataDeps(data);
	depTracker->setCurrentCtrlDeps(ctrl);
	depTracker->setCurrentAddrPoDeps(addrPo);
	depTracker->setCurrentCasDeps(cas);
}

void Interpreter::updateDataDeps(unsigned int tid, Value *dst, Value *src)
{
	if (depTracker)
		depTracker->updateDataDeps(tid, dst, src);
}

void Interpreter::updateDataDeps(unsigned int tid, Value *dst, const DepInfo *e)
{
	if (depTracker)
		depTracker->updateDataDeps(tid, dst, *e);
}

void Interpreter::updateDataDeps(unsigned int tid, Value *dst, Event e)
{
	if (depTracker)
		depTracker->updateDataDeps(tid, dst, e);
}

void Interpreter::updateAddrPoDeps(unsigned int tid, Value *src)
{
	if (!depTracker)
		return;

	depTracker->updateAddrPoDeps(tid, src);
	depTracker->setCurrentAddrPoDeps(getAddrPoDeps(tid));
}

void Interpreter::updateCtrlDeps(unsigned int tid, Value *src)
{
	if (!depTracker)
		return;

	depTracker->updateCtrlDeps(tid, src);
	depTracker->setCurrentCtrlDeps(getCtrlDeps(tid));
}

void Interpreter::clearDeps(unsigned int tid)
{
	if (depTracker)
		depTracker->clearDeps(tid);
}


//===----------------------------------------------------------------------===//
// Interpreter ctor - Initialize stuff
//
Interpreter::Interpreter(Module *M, VariableInfo &&VI, DirInode &&DI,
			 GenMCDriver *driver, const Config *userConf)
#ifdef LLVM_EXECUTIONENGINE_MODULE_UNIQUE_PTR
  : ExecutionEngine(std::unique_ptr<Module>(M)),
#else
  : ExecutionEngine(M),
#endif
    TD(M), VI(std::move(VI)), DI(std::move(DI)), driver(driver) {

  memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
#ifdef LLVM_EXECUTIONENGINE_DATALAYOUT_PTR
  setDataLayout(&TD);
#endif
  // Initialize the "backend"
  initializeExecutionEngine();
  initializeExternalFunctions();
  emitGlobals();

  varNames.grow(static_cast<int>(AddressSpace::AddressSpaceLast));
  collectGlobalAddresses(M);

  /* Set up a dependency tracker if the model requires it */
  if (userConf->isDepTrackingModel)
	  depTracker = make_unique<IMMDepTracker>();

  /* Also run a recovery routine if it is required to do so */
  checkPersistence = userConf->checkPersistence;
  if (checkPersistence) {
	  recoveryRoutine = M->getFunction("__VERIFIER_recovery_routine");
	  fds = llvm::BitVector(userConf->maxOpenFiles);
	  fdToFile.grow(userConf->maxOpenFiles);
	  this->maxFileSize = userConf->maxFileSize;
  }

  IL = new IntrinsicLowering(TD);
}

Interpreter::~Interpreter() {
  delete IL;
}

void Interpreter::runAtExitHandlers () {
  while (!AtExitHandlers.empty()) {
    callFunction(AtExitHandlers.back(), std::vector<GenericValue>());
    AtExitHandlers.pop_back();
    run();
  }
}

/// run - Start execution with the specified function and arguments.
///
#ifdef LLVM_EXECUTION_ENGINE_RUN_FUNCTION_VECTOR
GenericValue
Interpreter::runFunction(Function *F,
                         const std::vector<GenericValue> &ArgValues) {
#else
GenericValue
Interpreter::runFunction(Function *F,
                         ArrayRef<GenericValue> ArgValues) {
#endif
  assert (F && "Function *F was null at entry to run()");

  // Try extra hard not to pass extra args to a function that isn't
  // expecting them.  C programmers frequently bend the rules and
  // declare main() with fewer parameters than it actually gets
  // passed, and the interpreter barfs if you pass a function more
  // parameters than it is declared to take. This does not attempt to
  // take into account gratuitous differences in declared types,
  // though.
  std::vector<GenericValue> ActualArgs;
  const unsigned ArgCount = F->getFunctionType()->getNumParams();
  for (unsigned i = 0; i < ArgCount; ++i)
    ActualArgs.push_back(ArgValues[i]);

  // Set up the function call.
  callFunction(F, ActualArgs);

  // Start executing the function.
  run();

  return ExitValue;
}
