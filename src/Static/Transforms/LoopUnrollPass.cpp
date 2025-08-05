/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */


#include "LoopUnrollPass.hpp"
#include "Support/Error.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/Cloning.h>

using TerminatorInst = llvm::Instruction;

#if LLVM_VERSION_MAJOR >= 11
#define GET_LOADINST_ARG(val)
#else
#define GET_LOADINST_ARG(val) (val)->getType()->getPointerElementType(),
#endif

#if LLVM_VERSION_MAJOR >= 11
#define GET_BOUND_ALLOCA_ALIGN_ARG(val) llvm::Align(val)
#else
#define GET_BOUND_ALLOCA_ALIGN_ARG(val) val
#endif

using namespace llvm;

/*
 * Returns a PHI node the only purpose of which is to serve as the
 * bound variable for this loop.  It does not set up the arguments for
 * the PHI node.
 */
static auto createBoundInit(Loop *l) -> PHINode *
{
	Function *parentFun = (*l->block_begin())->getParent();
	Type *int32Typ = Type::getInt32Ty(parentFun->getContext());

	/* Post-dbgrecord migration, begin() iterator should be passed directly */
#if LLVM_VERSION_MAJOR < 19
	PHINode *bound = PHINode::Create(int32Typ, 0, "bound.val", &*l->getHeader()->begin());
#else
	PHINode *bound = PHINode::Create(int32Typ, 0, "bound.val", l->getHeader()->begin());
#endif
	return bound;
}

/*
 * Returns an instruction which decrements the bound variable for this loop (BOUNDVAL).
 * The returned value should be later checked in order to bound the loop.
 */
static auto createBoundDecrement(Loop *l, PHINode *boundVal) -> BinaryOperator *
{
	Function *parentFun = (*l->block_begin())->getParent();
	Type *int32Typ = Type::getInt32Ty(parentFun->getContext());

	Value *minusOne = ConstantInt::get(int32Typ, -1, true);
#if LLVM_VERSION_MAJOR < 19
	auto *pt = &*l->getHeader()->getFirstInsertionPt();
#else
	auto pt = l->getHeader()->getFirstInsertionPt();
#endif
	return BinaryOperator::CreateNSW(Instruction::Add, boundVal, minusOne,
					 l->getName() + ".bound.dec", pt);
}

static void addBoundCmpAndSpinEndBefore(Loop *l, PHINode *val, BinaryOperator *decVal)
{
	Function *parentFun = (*l->block_begin())->getParent();
	Function *endLoopFun = parentFun->getParent()->getFunction("__VERIFIER_kill_thread");
	Type *int32Typ = Type::getInt32Ty(parentFun->getContext());

	Value *zero = ConstantInt::get(int32Typ, 0);
	Value *cmp =
		new ICmpInst(decVal, ICmpInst::ICMP_EQ, val, zero, l->getName() + ".bound.cmp");

	BUG_ON(!endLoopFun);
	CallInst::Create(endLoopFun, {cmp}, "", decVal);
}

auto LoopUnrollPass::run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR,
			 LPMUpdater &U) -> PreservedAnalyses
{
	if (!shouldUnroll(&L))
		return PreservedAnalyses::all();

	PHINode *val = createBoundInit(&L);
	BinaryOperator *dec = createBoundDecrement(&L, val);
	Type *int32Typ = Type::getInt32Ty((*L.block_begin())->getParent()->getContext());

	/* Adjust incoming values for the bound variable */
	for (BasicBlock *bb : predecessors(L.getHeader()))
		val->addIncoming(L.contains(bb) ? (Value *)dec
						: (Value *)ConstantInt::get(int32Typ, unrollDepth_),
				 bb);

	/* Finally, compare the bound and block if it reaches zero */
	addBoundCmpAndSpinEndBefore(&L, val, dec);
	return PreservedAnalyses::none();
}
