//===-- Execution.cpp - Implement code to simulate the program ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the actual instruction interpreter.
//
//===----------------------------------------------------------------------===//

#include "Error.hpp"
#include "Event.hpp"
#include "Thread.hpp"
#include "ExecutionGraph.hpp"
#include "Interpreter.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <algorithm>
#include <cmath>
using namespace llvm;

#define DEBUG_TYPE "interpreter"

STATISTIC(NumDynamicInsts, "Number of dynamic instructions executed");

static cl::opt<bool> PrintVolatile("interpreter-print-volatile", cl::Hidden,
          cl::desc("make the interpreter print every volatile load and store"));

/* Types and variables */
/* TODO: integrate this into EventLabel?? */
struct RevisitPair {
	EventType type; /* Only {R,W} are valid */
	Event e;
	Event rf;
	std::list<Event> ls;
	std::vector<int> preds;

	RevisitPair(EventType t, Event e, Event rf, std::vector<int> preds)
		: type(t), e(e), rf(rf), preds(preds) {} ;
	RevisitPair(EventType t, Event e, std::list<Event> ls, std::vector<int> preds)
		: type(t), e(e), ls(ls), preds(preds) {};
};

std::vector<RevisitPair> *currentStack;
std::vector<std::vector<ExecutionContext> > initStacks;
int explored;

bool shouldContinue;

//===----------------------------------------------------------------------===//
//                     Various Helper Functions
//===----------------------------------------------------------------------===//

static void SetValue(Value *V, GenericValue Val, ExecutionContext &SF) {
  SF.Values[V] = Val;
}

static void printStarLine(void)
{
	for (int i = 0; i < 80; i++)
		std::cerr << "=";
	std::cerr << std::endl;
}

static void printExecGraph(ExecutionGraph &g)
{
	std::cerr << std::endl;
	printStarLine();
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		std::cerr << thr << std::endl;
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			std::cerr << "\t" << lab;
			if (lab.type == R)
				std::cerr << "\n\t\treads from: " << lab.rf;
			else if (lab.type == W) {
				for (auto &r : lab.rfm1)
					std::cerr << "\n\t\t is read from: " << r;
			}
			std::cerr << std::endl;
		}
	}
	printStarLine();
	std::cerr << "Revisit Set:" << std::endl;
	for (auto &l : g.revisit)
		std::cerr << "\t" << l << std::endl;
	printStarLine();
	std::cerr << "Max Events:" << std::endl;
	for (unsigned int i = 0; i < g.maxEvents.size(); i++)
		std::cerr << "\t" << g.maxEvents[i] << std::endl;
	printStarLine();
	std::cerr << std::endl;
}

static void printRevisitPair(RevisitPair &p)
{
	if (p.type == R)
		std::cerr << p.e << " needs to read from " << p.rf
			  << std::endl;
	else {
		std::cerr << "The following list has to read from "
			  << p.e << std::endl;
		for (auto &l : p.ls)
			std::cerr << "\t" << l << std::endl;
	}
}

static void printRevisitStack(std::vector<RevisitPair> &stack)
{
	for (unsigned int i = 0; i < stack.size(); i++) 
		printRevisitPair(stack[i]);
	return;
}

static EventLabel& getEventLabel(ExecutionGraph &g, Event &e)
{
	Thread &thr = g.threads[e.threadIndex];
	return thr.eventList[e.eventIndex];
}

/* TODO: Check if pass by reference is appropriate here */
static Event getLastThreadEvent(ExecutionGraph &g, int threadIndex)
{
	Thread &thr = g.threads[threadIndex];
	int last = g.maxEvents[threadIndex];

	return thr.eventList[last - 1].pos;
}

void __calcPorfBefore(ExecutionGraph &g, const Event &e, std::vector<int> &a)
{
	int ai = a[e.threadIndex];
	if (e.eventIndex <= ai)
		return;

	a[e.threadIndex] = e.eventIndex;
	Thread &thr = g.threads[e.threadIndex];
	for (int i = ai; i <= e.eventIndex; i++) {
		EventLabel &lab = thr.eventList[i];
		if (lab.type == R && !lab.rf.isInitializer())
			__calcPorfBefore(g, lab.rf, a);
	}
	return;			
}

std::vector<int> calcPorfBefore(ExecutionGraph &g, const Event &e)
{
	std::vector<int> a(g.threads.size(), 0);
	
	__calcPorfBefore(g, e, a);
	return a;
}

std::vector<int> calcPorfBefore(ExecutionGraph &g, const std::vector<Event> &es)
{
	std::vector<int> a(g.threads.size(), 0);

	for (auto &e : es)
		__calcPorfBefore(g, e, a);
	return a;
}

std::vector<int> calcPorfBefore(ExecutionGraph &g, const std::list<Event> &es)
{
	std::vector<int> a(g.threads.size(), 0);

	for (auto &e : es)
		__calcPorfBefore(g, e, a);
	return a;
}

std::vector<int> calcPorfBeforeNoRfs(ExecutionGraph &g, const std::list<Event> &es)
{
	std::vector<int> a(g.threads.size(), 0);

	for (auto &e : es)
		__calcPorfBefore(g, Event(e.threadIndex, e.eventIndex - 1), a);
	for (auto &e : es)
		if (a[e.threadIndex] < e.eventIndex)
			a[e.threadIndex] = e.eventIndex;
	return a;
}

void __calcPorfAfter(ExecutionGraph &g, const Event &e, std::vector<int> &a)
{
	int ai = a[e.threadIndex];
	if (e.eventIndex >= ai)
		return;

	a[e.threadIndex] = e.eventIndex;
	Thread &thr = g.threads[e.threadIndex];
	for (int i = e.eventIndex; i <= ai; i++) {
		if (i >= g.maxEvents[e.threadIndex])
			break;
		EventLabel &lab = thr.eventList[i];
		if (lab.type == W) /* TODO: Maybe reference before r? */
			for (auto r : lab.rfm1) {
				__calcPorfAfter(g, r, a);
			}
	}
	return;
}

std::vector<int> calcPorfAfter(ExecutionGraph &g, const std::list<Event> &es)
{
	std::vector<int> a(g.threads.size());

	for (unsigned int i = 0; i < a.size(); i++)
		a[i] = g.maxEvents[i];
	
	for (auto e : es)
		__calcPorfAfter(g, Event(e.threadIndex, e.eventIndex + 1), a);
	return a;
}

std::vector<int> calcPorfAfter(ExecutionGraph &g, const Event &e)
{
	std::vector<int> a(g.threads.size());

	for (int i = 0; i < a.size(); i++)
		a[i] = g.maxEvents[i];
	__calcPorfAfter(g, Event(e.threadIndex, e.eventIndex + 1), a);
	return a;
}


static std::list<Event> getAllStoresToLoc(ExecutionGraph &g, GenericValue *ptr,
					  ExecutionContext &SF)
{
	std::list<Event> stores;

	for (unsigned int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type == W && lab.addr == ptr)
				stores.push_front(lab.pos);
		}
	}
	stores.push_front(Event()); /* Also push the initializer event */
	return stores;
}

static std::list<Event> getStoresToLoc(ExecutionGraph &g, GenericValue *ptr,
				       ExecutionContext &SF)
{
	Event l = getLastThreadEvent(g, g.currentT);
	std::vector<int> before = calcPorfBefore(g, l);
	
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		while (before[i] > 0 &&
		       (thr.eventList[before[i]].type != W ||
			thr.eventList[before[i]].addr != ptr))
			--before[i];
	}
	
	std::list<Event> es;
	for (unsigned int i = 0; i < before.size(); i++)
		if (before[i] > 0)
			es.push_front(Event(i, before[i] - 1));
	
	if (es.empty())
		return getAllStoresToLoc(g, ptr, SF);

	std::list<Event> stores;
	std::vector<int> before2 = calcPorfBefore(g, es);
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		for (int j = before2[i] + 1; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type == W && lab.addr == ptr)
				stores.push_front(Event(i, j));
		}
	}
	return stores;
}

static bool RMWCanReadFromWrite(ExecutionGraph &g, Event &write)
{
	for (int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type == R && lab.rf == write && lab.isRMW) {
				if (!(std::find(g.revisit.begin(), g.revisit.end(), lab.pos) != g.revisit.end()))
					return false;
				std::vector<int> after = calcPorfAfter(g, lab.pos);
				Event last = getLastThreadEvent(g, g.currentT);
				if (last.eventIndex >= after[last.threadIndex])
					return false;
			}
		}
	}
	return true;
}

static std::vector<Event> getPendingReads(ExecutionGraph &g, Event &RMW, Event &RMWrf)
{
	std::vector<Event> pending;

	for (int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type == R && lab.rf == RMWrf && lab.pos != RMW)
				pending.push_back(lab.pos);
		}
	}
	return pending;
}

static void addStoreToGraph(ExecutionGraph &g, GenericValue *ptr,
			    GenericValue val, bool isRMW)
{
	Thread &thr = g.threads[g.currentT];
	int &max = g.maxEvents[g.currentT]; 
	EventLabel lab(W, Event(g.currentT, max), ptr, val, isRMW);

	thr.eventList.push_back(lab);
	max++;
	return;
}

static void addReadToGraph(ExecutionGraph &g, GenericValue *ptr,
			   Event rf, bool isRMW)
{
	Thread &thr = g.threads[g.currentT];
	int &max = g.maxEvents[g.currentT];
	EventLabel lab(R, Event(g.currentT, max), ptr, rf, isRMW);

	/* TODO: Make actual consistency checks before adding the event */
	if (!g.isConsistent())
		return;
       
	thr.eventList.push_back(lab);
	max++;

	if (rf.isInitializer())
		return;

	Thread &rfThr = g.threads[rf.threadIndex];
	EventLabel &rfLab = rfThr.eventList[rf.eventIndex];

	rfLab.rfm1.push_front(lab.pos);
	return;
}

static std::list<Event> getRevisitLoads(ExecutionGraph &g, Event store)
{
	std::vector<int> before = calcPorfBefore(g, store);
	EventLabel &sLab = getEventLabel(g, store);
	std::list<Event> rev;

	WARN_ON(sLab.type != W, "getRevisitLoads called with non-store event?");
	for (std::list<Event>::iterator it = g.revisit.begin(); it != g.revisit.end(); ++it) {
		EventLabel &rLab = getEventLabel(g, *it);
		if (before[rLab.pos.threadIndex] < rLab.pos.eventIndex && rLab.addr == sLab.addr)
			rev.push_back(*it);
	}
	return rev;
}

/* Calculates all the subsets (excluding the empty set) for a set of Events */
static std::vector<std::list<Event> > calcPowerSet(std::list<Event> ls)
{
	std::vector<std::list<Event> > powerSet;
	unsigned int pSize = pow(2, ls.size());
	unsigned int j;
	
	for (unsigned int i = 1; i < pSize; i++) {
		std::list<Event> set;
		j = 0;
		for (std::list<Event>::iterator it = ls.begin(); it != ls.end(); ++it) {
			if (i & (1 << j))
				set.push_back(*it);
			j++;
		}
		powerSet.push_back(set);
	}
	return powerSet;
}

/* Calculates all the subsets that contain a particular element for a set of Events */
static std::vector<std::list<Event> > calcPowerSetElem(std::list<Event> ls, Event elem)
{
	std::vector<std::list<Event> > powerSet;
	unsigned int pSize = pow(2, ls.size());
	int i, j;
	
	for (i = 1; i < pSize; i++) {
		std::list<Event> set;
		bool pushSet = false;
		j = 0;
		for (std::list<Event>::iterator it = ls.begin(); it != ls.end(); ++it) {
			if (i & (1 << j)) {
				if (elem == *it)
					pushSet = true;
				set.push_back(*it);
			}
			j++;
		}
		if (pushSet)
			powerSet.push_back(set);
	}
	return powerSet;
}

static void cutGraphBefore(ExecutionGraph &g, std::vector<int> before)
{
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		g.maxEvents[i] = before[i] + 1;
		Thread &thr = g.threads[i];
		thr.eventList.erase(thr.eventList.begin() + before[i] + 1, thr.eventList.end());
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type != W)
				continue;
			for (std::list<Event>::iterator it = lab.rfm1.begin();
			     it != lab.rfm1.end(); ++it)
				if (it->eventIndex > before[it->threadIndex])
					lab.rfm1.erase(it--);
		}
	}
	for (std::list<Event>::iterator it = g.revisit.begin(); it != g.revisit.end(); ++it)
		if (it->eventIndex > before[it->threadIndex])
			g.revisit.erase(it--);
	return;
}

static void fillGraphBefore(ExecutionGraph &oldG, ExecutionGraph &newG, std::vector<int> before)
{
	for (unsigned int i = 0; i < oldG.threads.size(); i++) {
		Thread &oldThr = oldG.threads[i];
		newG.threads.push_back(Thread(oldThr.threadFun, oldThr.id));
		Thread &newThr = newG.threads[i];
		for (int j = 0; j <= before[i]; j++)
			newThr.eventList.push_back(oldThr.eventList[j]);
		newG.maxEvents.push_back(before[i] + 1);
	}
	for (std::list<Event>::iterator it = oldG.revisit.begin(); it != oldG.revisit.end(); ++it)
		if (it->eventIndex < newG.maxEvents[it->threadIndex])
			newG.revisit.push_back(*it);
	return;
}

static void cutGraphAfter(ExecutionGraph &g, std::list<Event> ls)
{
	if (ls.empty())
		return;
	
	std::vector<int> after = calcPorfAfter(g, ls);
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		g.maxEvents[i] = after[i];
		Thread &thr = g.threads[i];
		thr.eventList.erase(thr.eventList.begin() + after[i], thr.eventList.end());
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type != W) {
				continue;
			}
			for (std::list<Event>::iterator it = lab.rfm1.begin();
			     it != lab.rfm1.end(); ++it)
				if (it->eventIndex >= after[it->threadIndex])
					lab.rfm1.erase(it--);
		}
	}
	for (std::list<Event>::iterator it = g.revisit.begin(); it != g.revisit.end(); ++it)
		if (it->eventIndex >= after[it->threadIndex] ||
		    (std::find(ls.begin(), ls.end(), *it) != ls.end()))
			g.revisit.erase(it--);
}

static void modifyRfs(ExecutionGraph &g, std::list<Event> &es, Event store)
{
	if (es.empty())
		return;

	for (std::list<Event>::iterator it = es.begin(); it != es.end(); ++it) {
		EventLabel &lab = getEventLabel(g, *it);
		Event oldRf = lab.rf;
		lab.rf = store;
		if (!oldRf.isInitializer()) {
			EventLabel &oldL = getEventLabel(g, oldRf);
			oldL.rfm1.remove(*it);
		}
	}
	/* TODO: Do some check here for initializer or if it is already present? */
	EventLabel &sLab = getEventLabel(g, store);
	sLab.rfm1.insert(sLab.rfm1.end(), es.begin(), es.end());

//	std::vector<int> before = calcPorfBeforeNoRfs(g, es);
	std::vector<int> before = calcPorfBefore(g, es);
	for (std::list<Event>::iterator it = g.revisit.begin(); it != g.revisit.end(); ++it)
		if (it->eventIndex <= before[it->threadIndex])
			g.revisit.erase(it--);
	return;
}

/* TODO: Fix coding style -- sed '/load/read/', '/store/write/' */
/* TODO: Maybe return reference? */
GenericValue Interpreter::loadValueFromWrite(ExecutionGraph &g, Event &write,
					     Type *typ, GenericValue *ptr,
					     ExecutionContext &SF)
{
	if (write.isInitializer()) {
		GenericValue result;
		LoadValueFromMemory(result, ptr, typ);
		return result;
	}
	
	EventLabel &lab = getEventLabel(g, write);
	return lab.val;
}

static void validateGraph(ExecutionGraph &g)
{
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		WARN_ON(thr.eventList.size() != (unsigned int)g.maxEvents[i],
			"Max event does not correspond to thread size!\n");
		for (int j = 0; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.type == R) {
				Event &rf = lab.rf;
				if (lab.rf.isInitializer())
					continue;
				bool readExists = false;
				for (auto &r : getEventLabel(g, rf).rfm1)
					if (r == lab.pos)
						readExists = true;
				if (!readExists) {
					WARN("Read event is not the appropriate rf-1 list!\n");
					std::cerr << lab.pos << std::endl;
					printExecGraph(g);
					abort();
				}
			} else if (lab.type == W) {
				if (lab.rfm1.empty())
					continue;
				bool writeExists = false;
				for (auto &e : lab.rfm1)
					if (getEventLabel(g, e).rf == lab.pos)
						writeExists = true;
				if (!writeExists) {
					WARN("Write event is not marked in the read event!\n");
					std::cerr << lab.pos << std::endl;
					printExecGraph(g);
					abort();
				}
			}
		}
	}
	return;
}

//===----------------------------------------------------------------------===//
//                    Binary Instruction Implementations
//===----------------------------------------------------------------------===//

#define IMPLEMENT_BINARY_OPERATOR(OP, TY) \
   case Type::TY##TyID: \
     Dest.TY##Val = Src1.TY##Val OP Src2.TY##Val; \
     break

static void executeFAddInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(+, Float);
    IMPLEMENT_BINARY_OPERATOR(+, Double);
  default:
    dbgs() << "Unhandled type for FAdd instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFSubInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(-, Float);
    IMPLEMENT_BINARY_OPERATOR(-, Double);
  default:
    dbgs() << "Unhandled type for FSub instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFMulInst(GenericValue &Dest, GenericValue Src1,
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(*, Float);
    IMPLEMENT_BINARY_OPERATOR(*, Double);
  default:
    dbgs() << "Unhandled type for FMul instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFDivInst(GenericValue &Dest, GenericValue Src1, 
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(/, Float);
    IMPLEMENT_BINARY_OPERATOR(/, Double);
  default:
    dbgs() << "Unhandled type for FDiv instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

static void executeFRemInst(GenericValue &Dest, GenericValue Src1, 
                            GenericValue Src2, Type *Ty) {
  switch (Ty->getTypeID()) {
  case Type::FloatTyID:
    Dest.FloatVal = fmod(Src1.FloatVal, Src2.FloatVal);
    break;
  case Type::DoubleTyID:
    Dest.DoubleVal = fmod(Src1.DoubleVal, Src2.DoubleVal);
    break;
  default:
    dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
}

#define IMPLEMENT_INTEGER_ICMP(OP, TY) \
   case Type::IntegerTyID:  \
      Dest.IntVal = APInt(1,Src1.IntVal.OP(Src2.IntVal)); \
      break;

#define IMPLEMENT_VECTOR_INTEGER_ICMP(OP, TY)                        \
  case Type::VectorTyID: {                                           \
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());    \
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );            \
    for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)             \
      Dest.AggregateVal[_i].IntVal = APInt(1,                        \
      Src1.AggregateVal[_i].IntVal.OP(Src2.AggregateVal[_i].IntVal));\
  } break;

// Handle pointers specially because they must be compared with only as much
// width as the host has.  We _do not_ want to be comparing 64 bit values when
// running on a 32-bit target, otherwise the upper 32 bits might mess up
// comparisons if they contain garbage.
#define IMPLEMENT_POINTER_ICMP(OP) \
   case Type::PointerTyID: \
      Dest.IntVal = APInt(1,(void*)(intptr_t)Src1.PointerVal OP \
                            (void*)(intptr_t)Src2.PointerVal); \
      break;

static GenericValue executeICMP_EQ(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_POINTER_ICMP(==);
  default:
    dbgs() << "Unhandled type for ICMP_EQ predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_NE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_POINTER_ICMP(!=);
  default:
    dbgs() << "Unhandled type for ICMP_NE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_ULT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    dbgs() << "Unhandled type for ICMP_ULT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_SLT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_POINTER_ICMP(<);
  default:
    dbgs() << "Unhandled type for ICMP_SLT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_UGT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    dbgs() << "Unhandled type for ICMP_UGT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_SGT(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
  default:
    dbgs() << "Unhandled type for ICMP_SGT predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_ULE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    dbgs() << "Unhandled type for ICMP_ULE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_SLE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
  default:
    dbgs() << "Unhandled type for ICMP_SLE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_UGE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    dbgs() << "Unhandled type for ICMP_UGE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeICMP_SGE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
  default:
    dbgs() << "Unhandled type for ICMP_SGE predicate: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

void Interpreter::visitICmpInst(ICmpInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		Type *Ty    = I.getOperand(0)->getType();
		GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
		GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
		GenericValue R;   // Result
  
		switch (I.getPredicate()) {
		case ICmpInst::ICMP_EQ:  R = executeICMP_EQ(Src1,  Src2, Ty); break;
		case ICmpInst::ICMP_NE:  R = executeICMP_NE(Src1,  Src2, Ty); break;
		case ICmpInst::ICMP_ULT: R = executeICMP_ULT(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_SLT: R = executeICMP_SLT(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_UGT: R = executeICMP_UGT(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_SGT: R = executeICMP_SGT(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_ULE: R = executeICMP_ULE(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_SLE: R = executeICMP_SLE(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_UGE: R = executeICMP_UGE(Src1, Src2, Ty); break;
		case ICmpInst::ICMP_SGE: R = executeICMP_SGE(Src1, Src2, Ty); break;
		default:
			dbgs() << "Don't know how to handle this ICmp predicate!\n-->" << I;
			llvm_unreachable(nullptr);
		}
		
		SetValue(&I, R, SF);
		return;
	}
  ExecutionContext &SF = ECStack.back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result
  
  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  R = executeICMP_EQ(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_NE:  R = executeICMP_NE(Src1,  Src2, Ty); break;
  case ICmpInst::ICMP_ULT: R = executeICMP_ULT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLT: R = executeICMP_SLT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGT: R = executeICMP_UGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGT: R = executeICMP_SGT(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_ULE: R = executeICMP_ULE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SLE: R = executeICMP_SLE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_UGE: R = executeICMP_UGE(Src1, Src2, Ty); break;
  case ICmpInst::ICMP_SGE: R = executeICMP_SGE(Src1, Src2, Ty); break;
  default:
    dbgs() << "Don't know how to handle this ICmp predicate!\n-->" << I;
    llvm_unreachable(nullptr);
  }
 
  SetValue(&I, R, SF);
}

#define IMPLEMENT_FCMP(OP, TY) \
   case Type::TY##TyID: \
     Dest.IntVal = APInt(1,Src1.TY##Val OP Src2.TY##Val); \
     break

#define IMPLEMENT_VECTOR_FCMP_T(OP, TY)                             \
  assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());     \
  Dest.AggregateVal.resize( Src1.AggregateVal.size() );             \
  for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)              \
    Dest.AggregateVal[_i].IntVal = APInt(1,                         \
    Src1.AggregateVal[_i].TY##Val OP Src2.AggregateVal[_i].TY##Val);\
  break;

#define IMPLEMENT_VECTOR_FCMP(OP)                                   \
  case Type::VectorTyID:                                            \
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {   \
      IMPLEMENT_VECTOR_FCMP_T(OP, Float);                           \
    } else {                                                        \
        IMPLEMENT_VECTOR_FCMP_T(OP, Double);                        \
    }

static GenericValue executeFCMP_OEQ(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(==, Float);
    IMPLEMENT_FCMP(==, Double);
    IMPLEMENT_VECTOR_FCMP(==);
  default:
    dbgs() << "Unhandled type for FCmp EQ instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

#define IMPLEMENT_SCALAR_NANS(TY, X,Y)                                      \
  if (TY->isFloatTy()) {                                                    \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {             \
      Dest.IntVal = APInt(1,false);                                         \
      return Dest;                                                          \
    }                                                                       \
  } else {                                                                  \
    if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) {         \
      Dest.IntVal = APInt(1,false);                                         \
      return Dest;                                                          \
    }                                                                       \
  }

#define MASK_VECTOR_NANS_T(X,Y, TZ, FLAG)                                   \
  assert(X.AggregateVal.size() == Y.AggregateVal.size());                   \
  Dest.AggregateVal.resize( X.AggregateVal.size() );                        \
  for( uint32_t _i=0;_i<X.AggregateVal.size();_i++) {                       \
    if (X.AggregateVal[_i].TZ##Val != X.AggregateVal[_i].TZ##Val ||         \
        Y.AggregateVal[_i].TZ##Val != Y.AggregateVal[_i].TZ##Val)           \
      Dest.AggregateVal[_i].IntVal = APInt(1,FLAG);                         \
    else  {                                                                 \
      Dest.AggregateVal[_i].IntVal = APInt(1,!FLAG);                        \
    }                                                                       \
  }

#define MASK_VECTOR_NANS(TY, X,Y, FLAG)                                     \
  if (TY->isVectorTy()) {                                                   \
    if (dyn_cast<VectorType>(TY)->getElementType()->isFloatTy()) {          \
      MASK_VECTOR_NANS_T(X, Y, Float, FLAG)                                 \
    } else {                                                                \
      MASK_VECTOR_NANS_T(X, Y, Double, FLAG)                                \
    }                                                                       \
  }                                                                         \



static GenericValue executeFCMP_ONE(GenericValue Src1, GenericValue Src2,
                                    Type *Ty)
{
  GenericValue Dest;
  // if input is scalar value and Src1 or Src2 is NaN return false
  IMPLEMENT_SCALAR_NANS(Ty, Src1, Src2)
  // if vector input detect NaNs and fill mask
  MASK_VECTOR_NANS(Ty, Src1, Src2, false)
  GenericValue DestMask = Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(!=, Float);
    IMPLEMENT_FCMP(!=, Double);
    IMPLEMENT_VECTOR_FCMP(!=);
    default:
      dbgs() << "Unhandled type for FCmp NE instruction: " << *Ty << "\n";
      llvm_unreachable(nullptr);
  }
  // in vector case mask out NaN elements
  if (Ty->isVectorTy())
    for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
      if (DestMask.AggregateVal[_i].IntVal == false)
        Dest.AggregateVal[_i].IntVal = APInt(1,false);

  return Dest;
}

static GenericValue executeFCMP_OLE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<=, Float);
    IMPLEMENT_FCMP(<=, Double);
    IMPLEMENT_VECTOR_FCMP(<=);
  default:
    dbgs() << "Unhandled type for FCmp LE instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeFCMP_OGE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>=, Float);
    IMPLEMENT_FCMP(>=, Double);
    IMPLEMENT_VECTOR_FCMP(>=);
  default:
    dbgs() << "Unhandled type for FCmp GE instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeFCMP_OLT(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<, Float);
    IMPLEMENT_FCMP(<, Double);
    IMPLEMENT_VECTOR_FCMP(<);
  default:
    dbgs() << "Unhandled type for FCmp LT instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

static GenericValue executeFCMP_OGT(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>, Float);
    IMPLEMENT_FCMP(>, Double);
    IMPLEMENT_VECTOR_FCMP(>);
  default:
    dbgs() << "Unhandled type for FCmp GT instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }
  return Dest;
}

#define IMPLEMENT_UNORDERED(TY, X,Y)                                     \
  if (TY->isFloatTy()) {                                                 \
    if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {          \
      Dest.IntVal = APInt(1,true);                                       \
      return Dest;                                                       \
    }                                                                    \
  } else if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
    Dest.IntVal = APInt(1,true);                                         \
    return Dest;                                                         \
  }

#define IMPLEMENT_VECTOR_UNORDERED(TY, X,Y, _FUNC)                       \
  if (TY->isVectorTy()) {                                                \
    GenericValue DestMask = Dest;                                        \
    Dest = _FUNC(Src1, Src2, Ty);                                        \
      for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)               \
        if (DestMask.AggregateVal[_i].IntVal == true)                    \
          Dest.AggregateVal[_i].IntVal = APInt(1,true);                  \
      return Dest;                                                       \
  }

static GenericValue executeFCMP_UEQ(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OEQ)
  return executeFCMP_OEQ(Src1, Src2, Ty);

}

static GenericValue executeFCMP_UNE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_ONE)
  return executeFCMP_ONE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLE)
  return executeFCMP_OLE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGE(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGE)
  return executeFCMP_OGE(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ULT(GenericValue Src1, GenericValue Src2,
                                   Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLT)
  return executeFCMP_OLT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_UGT(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  IMPLEMENT_UNORDERED(Ty, Src1, Src2)
  MASK_VECTOR_NANS(Ty, Src1, Src2, true)
  IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGT)
  return executeFCMP_OGT(Src1, Src2, Ty);
}

static GenericValue executeFCMP_ORD(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].FloatVal ==
        Src1.AggregateVal[_i].FloatVal) &&
        (Src2.AggregateVal[_i].FloatVal ==
        Src2.AggregateVal[_i].FloatVal)));
    } else {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].DoubleVal ==
        Src1.AggregateVal[_i].DoubleVal) &&
        (Src2.AggregateVal[_i].DoubleVal ==
        Src2.AggregateVal[_i].DoubleVal)));
    }
  } else if (Ty->isFloatTy())
    Dest.IntVal = APInt(1,(Src1.FloatVal == Src1.FloatVal && 
                           Src2.FloatVal == Src2.FloatVal));
  else {
    Dest.IntVal = APInt(1,(Src1.DoubleVal == Src1.DoubleVal && 
                           Src2.DoubleVal == Src2.DoubleVal));
  }
  return Dest;
}

static GenericValue executeFCMP_UNO(GenericValue Src1, GenericValue Src2,
                                     Type *Ty) {
  GenericValue Dest;
  if(Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
      for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,
        ( (Src1.AggregateVal[_i].FloatVal !=
           Src1.AggregateVal[_i].FloatVal) ||
          (Src2.AggregateVal[_i].FloatVal !=
           Src2.AggregateVal[_i].FloatVal)));
      } else {
        for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
          Dest.AggregateVal[_i].IntVal = APInt(1,
          ( (Src1.AggregateVal[_i].DoubleVal !=
             Src1.AggregateVal[_i].DoubleVal) ||
            (Src2.AggregateVal[_i].DoubleVal !=
             Src2.AggregateVal[_i].DoubleVal)));
      }
  } else if (Ty->isFloatTy())
    Dest.IntVal = APInt(1,(Src1.FloatVal != Src1.FloatVal || 
                           Src2.FloatVal != Src2.FloatVal));
  else {
    Dest.IntVal = APInt(1,(Src1.DoubleVal != Src1.DoubleVal || 
                           Src2.DoubleVal != Src2.DoubleVal));
  }
  return Dest;
}

static GenericValue executeFCMP_BOOL(GenericValue Src1, GenericValue Src2,
                                    const Type *Ty, const bool val) {
  GenericValue Dest;
    if(Ty->isVectorTy()) {
      assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
      Dest.AggregateVal.resize( Src1.AggregateVal.size() );
      for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
        Dest.AggregateVal[_i].IntVal = APInt(1,val);
    } else {
      Dest.IntVal = APInt(1, val);
    }

    return Dest;
}

void Interpreter::visitFCmpInst(FCmpInst &I) {
  ExecutionContext &SF = ECStack.back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result
  
  switch (I.getPredicate()) {
  default:
    dbgs() << "Don't know how to handle this FCmp predicate!\n-->" << I;
    llvm_unreachable(nullptr);
  break;
  case FCmpInst::FCMP_FALSE: R = executeFCMP_BOOL(Src1, Src2, Ty, false); 
  break;
  case FCmpInst::FCMP_TRUE:  R = executeFCMP_BOOL(Src1, Src2, Ty, true); 
  break;
  case FCmpInst::FCMP_ORD:   R = executeFCMP_ORD(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNO:   R = executeFCMP_UNO(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UEQ:   R = executeFCMP_UEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OEQ:   R = executeFCMP_OEQ(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UNE:   R = executeFCMP_UNE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ONE:   R = executeFCMP_ONE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULT:   R = executeFCMP_ULT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLT:   R = executeFCMP_OLT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGT:   R = executeFCMP_UGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGT:   R = executeFCMP_OGT(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_ULE:   R = executeFCMP_ULE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OLE:   R = executeFCMP_OLE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_UGE:   R = executeFCMP_UGE(Src1, Src2, Ty); break;
  case FCmpInst::FCMP_OGE:   R = executeFCMP_OGE(Src1, Src2, Ty); break;
  }
 
  SetValue(&I, R, SF);
}

static GenericValue executeCmpInst(unsigned predicate, GenericValue Src1, 
                                   GenericValue Src2, Type *Ty) {
  GenericValue Result;
  switch (predicate) {
  case ICmpInst::ICMP_EQ:    return executeICMP_EQ(Src1, Src2, Ty);
  case ICmpInst::ICMP_NE:    return executeICMP_NE(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGT:   return executeICMP_UGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGT:   return executeICMP_SGT(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULT:   return executeICMP_ULT(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLT:   return executeICMP_SLT(Src1, Src2, Ty);
  case ICmpInst::ICMP_UGE:   return executeICMP_UGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SGE:   return executeICMP_SGE(Src1, Src2, Ty);
  case ICmpInst::ICMP_ULE:   return executeICMP_ULE(Src1, Src2, Ty);
  case ICmpInst::ICMP_SLE:   return executeICMP_SLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ORD:   return executeFCMP_ORD(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNO:   return executeFCMP_UNO(Src1, Src2, Ty);
  case FCmpInst::FCMP_OEQ:   return executeFCMP_OEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_UEQ:   return executeFCMP_UEQ(Src1, Src2, Ty);
  case FCmpInst::FCMP_ONE:   return executeFCMP_ONE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UNE:   return executeFCMP_UNE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLT:   return executeFCMP_OLT(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULT:   return executeFCMP_ULT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGT:   return executeFCMP_OGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGT:   return executeFCMP_UGT(Src1, Src2, Ty);
  case FCmpInst::FCMP_OLE:   return executeFCMP_OLE(Src1, Src2, Ty);
  case FCmpInst::FCMP_ULE:   return executeFCMP_ULE(Src1, Src2, Ty);
  case FCmpInst::FCMP_OGE:   return executeFCMP_OGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_UGE:   return executeFCMP_UGE(Src1, Src2, Ty);
  case FCmpInst::FCMP_FALSE: return executeFCMP_BOOL(Src1, Src2, Ty, false);
  case FCmpInst::FCMP_TRUE:  return executeFCMP_BOOL(Src1, Src2, Ty, true);
  default:
    dbgs() << "Unhandled Cmp predicate\n";
    llvm_unreachable(nullptr);
  }
}

void Interpreter::visitBinaryOperator(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  Type *Ty    = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue R;   // Result

  // First process vector operation
  if (Ty->isVectorTy()) {
    assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
    R.AggregateVal.resize(Src1.AggregateVal.size());

    // Macros to execute binary operation 'OP' over integer vectors
#define INTEGER_VECTOR_OPERATION(OP)                               \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
      R.AggregateVal[i].IntVal =                                   \
      Src1.AggregateVal[i].IntVal OP Src2.AggregateVal[i].IntVal;

    // Additional macros to execute binary operations udiv/sdiv/urem/srem since
    // they have different notation.
#define INTEGER_VECTOR_FUNCTION(OP)                                \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
      R.AggregateVal[i].IntVal =                                   \
      Src1.AggregateVal[i].IntVal.OP(Src2.AggregateVal[i].IntVal);

    // Macros to execute binary operation 'OP' over floating point type TY
    // (float or double) vectors
#define FLOAT_VECTOR_FUNCTION(OP, TY)                               \
      for (unsigned i = 0; i < R.AggregateVal.size(); ++i)          \
        R.AggregateVal[i].TY =                                      \
        Src1.AggregateVal[i].TY OP Src2.AggregateVal[i].TY;

    // Macros to choose appropriate TY: float or double and run operation
    // execution
#define FLOAT_VECTOR_OP(OP) {                                         \
  if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy())        \
    FLOAT_VECTOR_FUNCTION(OP, FloatVal)                               \
  else {                                                              \
    if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy())     \
      FLOAT_VECTOR_FUNCTION(OP, DoubleVal)                            \
    else {                                                            \
      dbgs() << "Unhandled type for OP instruction: " << *Ty << "\n"; \
      llvm_unreachable(0);                                            \
    }                                                                 \
  }                                                                   \
}

    switch(I.getOpcode()){
    default:
      dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(nullptr);
      break;
    case Instruction::Add:   INTEGER_VECTOR_OPERATION(+) break;
    case Instruction::Sub:   INTEGER_VECTOR_OPERATION(-) break;
    case Instruction::Mul:   INTEGER_VECTOR_OPERATION(*) break;
    case Instruction::UDiv:  INTEGER_VECTOR_FUNCTION(udiv) break;
    case Instruction::SDiv:  INTEGER_VECTOR_FUNCTION(sdiv) break;
    case Instruction::URem:  INTEGER_VECTOR_FUNCTION(urem) break;
    case Instruction::SRem:  INTEGER_VECTOR_FUNCTION(srem) break;
    case Instruction::And:   INTEGER_VECTOR_OPERATION(&) break;
    case Instruction::Or:    INTEGER_VECTOR_OPERATION(|) break;
    case Instruction::Xor:   INTEGER_VECTOR_OPERATION(^) break;
    case Instruction::FAdd:  FLOAT_VECTOR_OP(+) break;
    case Instruction::FSub:  FLOAT_VECTOR_OP(-) break;
    case Instruction::FMul:  FLOAT_VECTOR_OP(*) break;
    case Instruction::FDiv:  FLOAT_VECTOR_OP(/) break;
    case Instruction::FRem:
      if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy())
        for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
          R.AggregateVal[i].FloatVal = 
          fmod(Src1.AggregateVal[i].FloatVal, Src2.AggregateVal[i].FloatVal);
      else {
        if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy())
          for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
            R.AggregateVal[i].DoubleVal = 
            fmod(Src1.AggregateVal[i].DoubleVal, Src2.AggregateVal[i].DoubleVal);
        else {
          dbgs() << "Unhandled type for Rem instruction: " << *Ty << "\n";
          llvm_unreachable(nullptr);
        }
      }
      break;
    }
  } else {
    switch (I.getOpcode()) {
    default:
      dbgs() << "Don't know how to handle this binary operator!\n-->" << I;
      llvm_unreachable(nullptr);
      break;
    case Instruction::Add:   R.IntVal = Src1.IntVal + Src2.IntVal; break;
    case Instruction::Sub:   R.IntVal = Src1.IntVal - Src2.IntVal; break;
    case Instruction::Mul:   R.IntVal = Src1.IntVal * Src2.IntVal; break;
    case Instruction::FAdd:  executeFAddInst(R, Src1, Src2, Ty); break;
    case Instruction::FSub:  executeFSubInst(R, Src1, Src2, Ty); break;
    case Instruction::FMul:  executeFMulInst(R, Src1, Src2, Ty); break;
    case Instruction::FDiv:  executeFDivInst(R, Src1, Src2, Ty); break;
    case Instruction::FRem:  executeFRemInst(R, Src1, Src2, Ty); break;
    case Instruction::UDiv:  R.IntVal = Src1.IntVal.udiv(Src2.IntVal); break;
    case Instruction::SDiv:  R.IntVal = Src1.IntVal.sdiv(Src2.IntVal); break;
    case Instruction::URem:  R.IntVal = Src1.IntVal.urem(Src2.IntVal); break;
    case Instruction::SRem:  R.IntVal = Src1.IntVal.srem(Src2.IntVal); break;
    case Instruction::And:   R.IntVal = Src1.IntVal & Src2.IntVal; break;
    case Instruction::Or:    R.IntVal = Src1.IntVal | Src2.IntVal; break;
    case Instruction::Xor:   R.IntVal = Src1.IntVal ^ Src2.IntVal; break;
    }
  }
  SetValue(&I, R, SF);
}

static GenericValue executeSelectInst(GenericValue Src1, GenericValue Src2,
                                      GenericValue Src3, const Type *Ty) {
    GenericValue Dest;
    if(Ty->isVectorTy()) {
      assert(Src1.AggregateVal.size() == Src2.AggregateVal.size());
      assert(Src2.AggregateVal.size() == Src3.AggregateVal.size());
      Dest.AggregateVal.resize( Src1.AggregateVal.size() );
      for (size_t i = 0; i < Src1.AggregateVal.size(); ++i)
        Dest.AggregateVal[i] = (Src1.AggregateVal[i].IntVal == 0) ?
          Src3.AggregateVal[i] : Src2.AggregateVal[i];
    } else {
      Dest = (Src1.IntVal == 0) ? Src3 : Src2;
    }
    return Dest;
}

void Interpreter::visitSelectInst(SelectInst &I) {
  ExecutionContext &SF = ECStack.back();
  const Type * Ty = I.getOperand(0)->getType();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue R = executeSelectInst(Src1, Src2, Src3, Ty);
  SetValue(&I, R, SF);
}

//===----------------------------------------------------------------------===//
//                     Terminator Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::exitCalled(GenericValue GV) {
  // runAtExitHandlers() assumes there are no stack frames, but
  // if exit() was called, then it had a stack frame. Blow away
  // the stack before interpreting atexit handlers.
  ECStack.clear();
  runAtExitHandlers();
  exit(GV.IntVal.zextOrTrunc(32).getZExtValue());
}

/// Pop the last stack frame off of ECStack and then copy the result
/// back into the result variable if we are not returning void. The
/// result variable may be the ExitValue, or the Value of the calling
/// CallInst if there was a previous stack frame. This method may
/// invalidate any ECStack iterators you have. This method also takes
/// care of switching to the normal destination BB, if we are returning
/// from an invoke.
///
void Interpreter::popStackAndReturnValueToCaller(Type *RetTy,
                                                 GenericValue Result) {

  // Pop the current stack frame.
  ECStack.pop_back();

  if (ECStack.empty()) {  // Finished main.  Put result into exit code...
    if (RetTy && !RetTy->isVoidTy()) {          // Nonvoid return type?
      ExitValue = Result;   // Capture the exit value of the program
    } else {
      memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
    }
  } else {
    // If we have a previous stack frame, and we have a previous call,
    // fill in the return value...
    ExecutionContext &CallingSF = ECStack.back();
    if (Instruction *I = CallingSF.Caller.getInstruction()) {
      // Save result...
      if (!CallingSF.Caller.getType()->isVoidTy())
        SetValue(I, Result, CallingSF);
      if (InvokeInst *II = dyn_cast<InvokeInst> (I))
        SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
      CallingSF.Caller = CallSite();          // We returned from the call...
    }
  }
}

void Interpreter::visitReturnInst(ReturnInst &I) {
	if (!dryRun) {
		getECStack()->pop_back();
		return;
	}	
  ExecutionContext &SF = ECStack.back();
  Type *RetTy = Type::getVoidTy(I.getContext());
  GenericValue Result;

  // Save away the return value... (if we are not 'ret void')
  if (I.getNumOperands()) {
    RetTy  = I.getReturnValue()->getType();
    Result = getOperandValue(I.getReturnValue(), SF);
  }

  popStackAndReturnValueToCaller(RetTy, Result);
}

void Interpreter::visitUnreachableInst(UnreachableInst &I) {
  report_fatal_error("Program executed an 'unreachable' instruction!");
}

void Interpreter::visitBranchInst(BranchInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		BasicBlock *Dest;

		Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
		if (!I.isUnconditional()) {
			Value *Cond = I.getCondition();
			if (getOperandValue(Cond, SF).IntVal == 0) // If false cond...
				Dest = I.getSuccessor(1);
		}
		SwitchToNewBasicBlock(Dest, SF);		
		return;
	}
  ExecutionContext &SF = ECStack.back();
  BasicBlock *Dest;

  Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
  if (!I.isUnconditional()) {
    Value *Cond = I.getCondition();
    if (getOperandValue(Cond, SF).IntVal == 0) // If false cond...
      Dest = I.getSuccessor(1);
  }
  SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitSwitchInst(SwitchInst &I) {
  ExecutionContext &SF = ECStack.back();
  Value* Cond = I.getCondition();
  Type *ElTy = Cond->getType();
  GenericValue CondVal = getOperandValue(Cond, SF);

  // Check to see if any of the cases match...
  BasicBlock *Dest = nullptr;
  for (SwitchInst::CaseIt i = I.case_begin(), e = I.case_end(); i != e; ++i) {
    GenericValue CaseVal = getOperandValue(i.getCaseValue(), SF);
    if (executeICMP_EQ(CondVal, CaseVal, ElTy).IntVal != 0) {
      Dest = cast<BasicBlock>(i.getCaseSuccessor());
      break;
    }
  }
  if (!Dest) Dest = I.getDefaultDest();   // No cases matched: use default
  SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitIndirectBrInst(IndirectBrInst &I) {
  ExecutionContext &SF = ECStack.back();
  void *Dest = GVTOP(getOperandValue(I.getAddress(), SF));
  SwitchToNewBasicBlock((BasicBlock*)Dest, SF);
}


// SwitchToNewBasicBlock - This method is used to jump to a new basic block.
// This function handles the actual updating of block and instruction iterators
// as well as execution of all of the PHI nodes in the destination block.
//
// This method does this because all of the PHI nodes must be executed
// atomically, reading their inputs before any of the results are updated.  Not
// doing this can cause problems if the PHI nodes depend on other PHI nodes for
// their inputs.  If the input PHI node is updated before it is read, incorrect
// results can happen.  Thus we use a two phase approach.
//
void Interpreter::SwitchToNewBasicBlock(BasicBlock *Dest, ExecutionContext &SF){
  BasicBlock *PrevBB = SF.CurBB;      // Remember where we came from...
  SF.CurBB   = Dest;                  // Update CurBB to branch destination
  SF.CurInst = SF.CurBB->begin();     // Update new instruction ptr...

  if (!isa<PHINode>(SF.CurInst)) return;  // Nothing fancy to do

  // Loop over all of the PHI nodes in the current block, reading their inputs.
  std::vector<GenericValue> ResultValues;

  for (; PHINode *PN = dyn_cast<PHINode>(SF.CurInst); ++SF.CurInst) {
    // Search for the value corresponding to this previous bb...
    int i = PN->getBasicBlockIndex(PrevBB);
    assert(i != -1 && "PHINode doesn't contain entry for predecessor??");
    Value *IncomingValue = PN->getIncomingValue(i);

    // Save the incoming value for this PHI node...
    ResultValues.push_back(getOperandValue(IncomingValue, SF));
  }

  // Now loop over all of the PHI nodes setting their values...
  SF.CurInst = SF.CurBB->begin();
  for (unsigned i = 0; isa<PHINode>(SF.CurInst); ++SF.CurInst, ++i) {
    PHINode *PN = cast<PHINode>(SF.CurInst);
    SetValue(PN, ResultValues[i], SF);
  }
}

//===----------------------------------------------------------------------===//
//                     Memory Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitAllocaInst(AllocaInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();

		Type *Ty = I.getType()->getElementType();  // Type to be allocated

		// Get the number of elements being allocated by the array...
		unsigned NumElements = 
			getOperandValue(I.getOperand(0), SF).IntVal.getZExtValue();

		unsigned TypeSize = (size_t)TD.getTypeAllocSize(Ty);

		// Avoid malloc-ing zero bytes, use max()...
		unsigned MemToAlloc = std::max(1U, NumElements * TypeSize);

		// Allocate enough memory to hold the type...
		void *Memory = malloc(MemToAlloc);

		DEBUG(dbgs() << "Allocated Type: " << *Ty << " (" << TypeSize << " bytes) x " 
		      << NumElements << " (Total: " << MemToAlloc << ") at "
		      << uintptr_t(Memory) << '\n');

		GenericValue Result = PTOGV(Memory);
		assert(Result.PointerVal && "Null pointer returned by malloc!");
		SetValue(&I, Result, SF);

		if (I.getOpcode() == Instruction::Alloca)
			getECStack()->back().Allocas.add(Memory);
		return;
	}
  ExecutionContext &SF = ECStack.back();

  Type *Ty = I.getType()->getElementType();  // Type to be allocated

  // Get the number of elements being allocated by the array...
  unsigned NumElements = 
    getOperandValue(I.getOperand(0), SF).IntVal.getZExtValue();

  unsigned TypeSize = (size_t)TD.getTypeAllocSize(Ty);

  // Avoid malloc-ing zero bytes, use max()...
  unsigned MemToAlloc = std::max(1U, NumElements * TypeSize);

  // Allocate enough memory to hold the type...
  void *Memory = malloc(MemToAlloc);

  DEBUG(dbgs() << "Allocated Type: " << *Ty << " (" << TypeSize << " bytes) x " 
               << NumElements << " (Total: " << MemToAlloc << ") at "
               << uintptr_t(Memory) << '\n');

  GenericValue Result = PTOGV(Memory);
  assert(Result.PointerVal && "Null pointer returned by malloc!");
  SetValue(&I, Result, SF);

   if (I.getOpcode() == Instruction::Alloca)
	   ECStack.back().Allocas.add(Memory);
}

// getElementOffset - The workhorse for getelementptr.
//
GenericValue Interpreter::executeGEPOperation(Value *Ptr, gep_type_iterator I,
                                              gep_type_iterator E,
                                              ExecutionContext &SF) {
  assert(Ptr->getType()->isPointerTy() &&
         "Cannot getElementOffset of a nonpointer type!");

  uint64_t Total = 0;

  for (; I != E; ++I) {
    if (StructType *STy = dyn_cast<StructType>(*I)) {
      const StructLayout *SLO = TD.getStructLayout(STy);

      const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
      unsigned Index = unsigned(CPU->getZExtValue());

      Total += SLO->getElementOffset(Index);
    } else {
      SequentialType *ST = cast<SequentialType>(*I);
      // Get the index number for the array... which must be long type...
      GenericValue IdxGV = getOperandValue(I.getOperand(), SF);

      int64_t Idx;
      unsigned BitWidth = 
        cast<IntegerType>(I.getOperand()->getType())->getBitWidth();
      if (BitWidth == 32)
        Idx = (int64_t)(int32_t)IdxGV.IntVal.getZExtValue();
      else {
        assert(BitWidth == 64 && "Invalid index type for getelementptr");
        Idx = (int64_t)IdxGV.IntVal.getZExtValue();
      }
      Total += TD.getTypeAllocSize(ST->getElementType())*Idx;
    }
  }

  GenericValue Result;
  Result.PointerVal = ((char*)getOperandValue(Ptr, SF).PointerVal) + Total;
  DEBUG(dbgs() << "GEP Index " << Total << " bytes.\n");
  return Result;
}

void Interpreter::visitGetElementPtrInst(GetElementPtrInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeGEPOperation(I.getPointerOperand(),
                                   gep_type_begin(I), gep_type_end(I), SF), SF);
}

void Interpreter::visitLoadInst(LoadInst &I) {
	if(!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		GenericValue src = getOperandValue(I.getPointerOperand(), SF);
		GenericValue *ptr = (GenericValue *) GVTOP(src);

		/* TODO: Perform the load if it is not a GlobalValue (within if) */
		if (!isa<GlobalVariable>(I.getPointerOperand()))
			return;

		int c = ++globalCount[currentEG->currentT];
		Thread &thr = currentEG->threads[currentEG->currentT];
		if (currentEG->maxEvents[currentEG->currentT] > c) {
			EventLabel &lab = thr.eventList[c];
			GenericValue val = loadValueFromWrite(*currentEG, lab.rf, I.getType(), ptr, SF);
			SetValue(&I, val, SF);			
			return;
		}

		std::list<Event> stores = getStoresToLoc(*currentEG, ptr, SF);
		std::vector<int> preds;

		/* TODO: Replace this with an iterator */
		for (auto s : stores) {
			/* Mark reads as revisited only in the first graph */
			if (preds.empty()) { /* TODO: Maybe not create object? */
				addReadToGraph(*currentEG, ptr, s, false);
				Event e = getLastThreadEvent(*currentEG, currentEG->currentT);
				for (unsigned int k = 0; k < currentEG->threads.size(); k++)
					preds.push_back(currentEG->maxEvents[k] - 1);
				currentEG->revisit.push_front(e);
			} else {
				Event e = getLastThreadEvent(*currentEG, currentEG->currentT);
				currentStack->push_back(RevisitPair(R, e, s, preds));
			}
		}
		return;
	}
  ExecutionContext &SF = ECStack.back();
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  GenericValue *Ptr = (GenericValue*)GVTOP(SRC);
  GenericValue Result;
  LoadValueFromMemory(Result, Ptr, I.getType());
  SetValue(&I, Result, SF);
  if (I.isVolatile() && PrintVolatile)
    dbgs() << "Volatile load " << I;
}

void Interpreter::visitStoreInst(StoreInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		GenericValue val = getOperandValue(I.getOperand(0), SF);
		GenericValue src = getOperandValue(I.getPointerOperand(), SF);
		GenericValue *ptr = (GenericValue *) GVTOP(src);

		/* TODO: Perform the store if it is not a GlobalValue (within if) */		
		if (!isa<GlobalVariable>(I.getPointerOperand()))
			return;

		int c = ++globalCount[currentEG->currentT];
		if (currentEG->maxEvents[currentEG->currentT] > c)
			return;

		addStoreToGraph(*currentEG, ptr, val, false);
		Event s = getLastThreadEvent(*currentEG, currentEG->currentT);
		std::list<Event> ls = getRevisitLoads(*currentEG, s);
		std::vector<std::list<Event> > revisitSets = calcPowerSet(ls);
		std::vector<int> preds;

		for (unsigned int k = 0; k < currentEG->threads.size(); k++)
			preds.push_back(currentEG->maxEvents[k] - 1);
		for (std::vector<std::list<Event> >::iterator it = revisitSets.begin();
		     it != revisitSets.end(); ++it) {
			/* TODO: Replace the below with a function */
			bool checkSet = true;
			std::vector<int> after = calcPorfAfter(*currentEG, *it);
			
			for (std::list<Event>::iterator li = it->begin();
			     li != it->end(); ++li) 
				if (after[li->threadIndex] <= li->eventIndex)
					checkSet = false;
			if (checkSet && after[s.threadIndex] > s.eventIndex) {
				RevisitPair p = RevisitPair(W, s, *it, preds);
//				printRevisitPair(p);
//				printExecGraph(*currentEG);
				ExecutionGraph eg;
//				printExecGraph(g);
				fillGraphBefore(*currentEG, eg, preds);
				cutGraphAfter(eg, *it);
				modifyRfs(eg, *it, s);
//				printExecGraph(eg);
				visitGraph(eg);
			}
		}
		return;
	}
  ExecutionContext &SF = ECStack.back();
  GenericValue Val = getOperandValue(I.getOperand(0), SF);
  GenericValue SRC = getOperandValue(I.getPointerOperand(), SF);
  StoreValueToMemory(Val, (GenericValue *)GVTOP(SRC),
                     I.getOperand(0)->getType());
  if (I.isVolatile() && PrintVolatile)
    dbgs() << "Volatile store: " << I;
}

void Interpreter::visitAtomicCmpXchgInst(AtomicCmpXchgInst &I) {
	if (dryRun)
		dbgs() << "Please do not use CAS operations outside of thread code!\n";

	ExecutionGraph &g = *currentEG;
	ExecutionContext &SF = getECStack()->back();
	GenericValue cmpVal = getOperandValue(I.getCompareOperand(), SF);
	GenericValue newVal = getOperandValue(I.getNewValOperand(), SF);
	GenericValue src = getOperandValue(I.getPointerOperand(), SF);
	GenericValue *ptr = (GenericValue *) GVTOP(src);
	Type *typ = I.getCompareOperand()->getType();
	GenericValue result;

	/* TODO: Perform the load if it is not a GlobalValue (within if) */
	if (!isa<GlobalVariable>(I.getPointerOperand()))
		return;

	int c = ++globalCount[g.currentT];
	Thread &thr = g.threads[g.currentT];
	if (c == g.maxEvents[g.currentT] - 1) {
		EventLabel &lab = thr.eventList[c];
		GenericValue oldVal = loadValueFromWrite(g, lab.rf, typ, ptr, SF);
		GenericValue cmpRes = executeICMP_EQ(oldVal, cmpVal, typ);
		if (cmpRes.IntVal.getBoolValue()) {
			std::vector<Event> pendingReads = getPendingReads(g, lab.pos, lab.rf);
			/* TODO: Fix the check below */
			WARN_ON(pendingReads.size() > 1, "More than 1 pending RMWs?\n");
			// if (pendingReads.size() > 1) {
			// 	std::cerr << lab.pos << " reads from " << lab.rf << std::endl;
			// 	printExecGraph(g);
			// 	for (auto r : pendingReads)
			// 		std::cerr << r << std::endl;
			// }
			addStoreToGraph(g, ptr, newVal, true);
			++globalCount[g.currentT];
			
			Event s = getLastThreadEvent(g, g.currentT);
			std::list<Event> ls = getRevisitLoads(g, s);
			std::vector<std::list<Event> > revisitSets;
			if (pendingReads.size() == 0) {
				revisitSets = calcPowerSet(ls);
			} else {
				revisitSets = calcPowerSetElem(ls, pendingReads.back());
				shouldContinue = false;
			}
			
			/* TODO: Replace this with getGraphState() */
			std::vector<int> writePreds;
			for (int k = 0; k < g.threads.size(); k++)
				writePreds.push_back(g.maxEvents[k] - 1);
			
			for (std::vector<std::list<Event> >::iterator rit = revisitSets.begin();
			     rit != revisitSets.end(); ++rit) {
				bool checkSet = true;
				std::vector<int> after = calcPorfAfter(g, *rit);
				
				for (std::list<Event>::iterator li = rit->begin();
				     li != rit->end(); ++li) 
					if (after[li->threadIndex] <= li->eventIndex)
						checkSet = false;
				if (checkSet && after[s.threadIndex] > s.eventIndex) {
					ExecutionGraph eg;
//				printExecGraph(g);
					fillGraphBefore(g, eg, writePreds);
					cutGraphAfter(eg, *rit);
					modifyRfs(eg, *rit, s);
//				printExecGraph(eg);
					visitGraph(eg);
				}
			}
		}
			
		/* TODO: Check move semantics .. */
		result.AggregateVal.push_back(oldVal);
		result.AggregateVal.push_back(cmpRes);
		SetValue(&I, result, SF);
		return;
	}
	if (c < g.maxEvents[g.currentT]) {
		EventLabel &lab = thr.eventList[c];
		GenericValue oldVal = loadValueFromWrite(g, lab.rf, typ, ptr, SF);
		GenericValue cmpRes = executeICMP_EQ(oldVal, cmpVal, typ);
		result.AggregateVal.push_back(oldVal);
		result.AggregateVal.push_back(cmpRes);
		SetValue(&I, result, SF);
		return;
	}

	std::list<Event> stores = getStoresToLoc(g, ptr, SF);
	std::vector<int> readPreds;
	for (std::list<Event>::iterator it = stores.begin(); it != stores.end(); ++it) {
		GenericValue oldVal = loadValueFromWrite(g, *it, typ, ptr, SF);
		GenericValue cmpRes = executeICMP_EQ(oldVal, cmpVal, typ);
		if (cmpRes.IntVal.getBoolValue() && !RMWCanReadFromWrite(g, *it))
			continue;
		if (readPreds.empty()) { /* TODO: Maybe not create object? */
			addReadToGraph(g, ptr, *it, true);
			Event e = getLastThreadEvent(g, g.currentT);
			for (int k = 0; k < g.threads.size(); k++)
				readPreds.push_back(g.maxEvents[k] - 1);
			g.revisit.push_front(e);

			/* Must add store after calling getPendingRMWs() */
			if (cmpRes.IntVal.getBoolValue()) {
				std::vector<Event> pendingReads = getPendingReads(g, e, *it);
				/* TODO: Fix the check below */
				WARN_ON(pendingReads.size() > 1, "More than 1 pending RMWs?");
				addStoreToGraph(g, ptr, newVal, true);
				
				Event s = getLastThreadEvent(g, g.currentT);
				std::list<Event> ls = getRevisitLoads(g, s);
				std::vector<std::list<Event> > revisitSets;
				if (pendingReads.size() == 0) {
					revisitSets = calcPowerSet(ls);
				} else {
					revisitSets = calcPowerSetElem(ls, pendingReads.back());
					shouldContinue = false;
				}

				/* TODO: Replace this with getGraphState() */
				std::vector<int> writePreds;
				for (int k = 0; k < g.threads.size(); k++)
					writePreds.push_back(g.maxEvents[k] - 1);
				
				for (std::vector<std::list<Event> >::iterator rit = revisitSets.begin();
				     rit != revisitSets.end(); ++rit) {
					bool checkSet = true;
					std::vector<int> after = calcPorfAfter(g, *rit);
					
					for (std::list<Event>::iterator li = rit->begin();
					     li != rit->end(); ++li) 
						if (after[li->threadIndex] <= li->eventIndex)
							checkSet = false;
					if (checkSet && after[s.threadIndex] > s.eventIndex) {
						ExecutionGraph eg;
//				printExecGraph(g);
						fillGraphBefore(g, eg, writePreds);
						cutGraphAfter(eg, *rit);
						modifyRfs(eg, *rit, s);
//				printExecGraph(eg);
						visitGraph(eg);
					}
				}
			}
			
			/* TODO: Check move semantics .. */
			result.AggregateVal.push_back(oldVal);
			result.AggregateVal.push_back(cmpRes);
			SetValue(&I, result, SF);
		} else {
			Event e = getLastThreadEvent(g, g.currentT);
			currentStack->push_back(RevisitPair(R, e, *it, readPreds));
		}
	}
	return;
}

//===----------------------------------------------------------------------===//
//                 Miscellaneous Instruction Implementations
//===----------------------------------------------------------------------===//

void Interpreter::visitCallSite(CallSite CS) {
  ExecutionContext &SF = ECStack.back();

  // Check to see if this is an intrinsic function call...
  Function *F = CS.getCalledFunction();
  if (F && F->isDeclaration())
    switch (F->getIntrinsicID()) {
    case Intrinsic::not_intrinsic:
      break;
    case Intrinsic::vastart: { // va_start
      GenericValue ArgIndex;
      ArgIndex.UIntPairVal.first = ECStack.size() - 1;
      ArgIndex.UIntPairVal.second = 0;
      SetValue(CS.getInstruction(), ArgIndex, SF);
      return;
    }
    case Intrinsic::vaend:    // va_end is a noop for the interpreter
      return;
    case Intrinsic::vacopy:   // va_copy: dest = src
      SetValue(CS.getInstruction(), getOperandValue(*CS.arg_begin(), SF), SF);
      return;
    default:
      // If it is an unknown intrinsic function, use the intrinsic lowering
      // class to transform it into hopefully tasty LLVM code.
      //
      BasicBlock::iterator me(CS.getInstruction());
      BasicBlock *Parent = CS.getInstruction()->getParent();
      bool atBegin(Parent->begin() == me);
      if (!atBegin)
        --me;
      IL->LowerIntrinsicCall(cast<CallInst>(CS.getInstruction()));

      // Restore the CurInst pointer to the first instruction newly inserted, if
      // any.
      if (atBegin) {
        SF.CurInst = Parent->begin();
      } else {
        SF.CurInst = me;
        ++SF.CurInst;
      }
      return;
    }


  SF.Caller = CS;
  std::vector<GenericValue> ArgVals;
  const unsigned NumArgs = SF.Caller.arg_size();
  ArgVals.reserve(NumArgs);
  uint16_t pNum = 1;
  for (CallSite::arg_iterator i = SF.Caller.arg_begin(),
         e = SF.Caller.arg_end(); i != e; ++i, ++pNum) {
    Value *V = *i;
    ArgVals.push_back(getOperandValue(V, SF));
  }

  // To handle indirect calls, we must get the pointer value from the argument
  // and treat it as a function pointer.
  GenericValue SRC = getOperandValue(SF.Caller.getCalledValue(), SF);
  callFunction((Function*)GVTOP(SRC), ArgVals);
}

// auxiliary function for shift operations
static unsigned getShiftAmount(uint64_t orgShiftAmount,
                               llvm::APInt valueToShift) {
  unsigned valueWidth = valueToShift.getBitWidth();
  if (orgShiftAmount < (uint64_t)valueWidth)
    return orgShiftAmount;
  // according to the llvm documentation, if orgShiftAmount > valueWidth,
  // the result is undfeined. but we do shift by this rule:
  return (NextPowerOf2(valueWidth-1) - 1) & orgShiftAmount;
}


void Interpreter::visitShl(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitLShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitAShr(BinaryOperator &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;
  const Type *Ty = I.getType();

  if (Ty->isVectorTy()) {
    size_t src1Size = Src1.AggregateVal.size();
    assert(src1Size == Src2.AggregateVal.size());
    for (unsigned i = 0; i < src1Size; i++) {
      GenericValue Result;
      uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
      llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
      Result.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
      Dest.AggregateVal.push_back(Result);
    }
  } else {
    // scalar
    uint64_t shiftAmount = Src2.IntVal.getZExtValue();
    llvm::APInt valueToShift = Src1.IntVal;
    Dest.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
  }

  SetValue(&I, Dest, SF);
}

GenericValue Interpreter::executeTruncInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  Type *SrcTy = SrcVal->getType();
  if (SrcTy->isVectorTy()) {
    Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned NumElts = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(NumElts);
    for (unsigned i = 0; i < NumElts; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.trunc(DBitWidth);
  } else {
    IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.trunc(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeSExtInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  if (SrcTy->isVectorTy()) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.sext(DBitWidth);
  } else {
    const IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.sext(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeZExtInst(Value *SrcVal, Type *DstTy,
                                          ExecutionContext &SF) {
  const Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  if (SrcTy->isVectorTy()) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.zext(DBitWidth);
  } else {
    const IntegerType *DITy = cast<IntegerType>(DstTy);
    unsigned DBitWidth = DITy->getBitWidth();
    Dest.IntVal = Src.IntVal.zext(DBitWidth);
  }
  return Dest;
}

GenericValue Interpreter::executeFPTruncInst(Value *SrcVal, Type *DstTy,
                                             ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcVal->getType()->getTypeID() == Type::VectorTyID) {
    assert(SrcVal->getType()->getScalarType()->isDoubleTy() &&
           DstTy->getScalarType()->isFloatTy() &&
           "Invalid FPTrunc instruction");

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].FloatVal = (float)Src.AggregateVal[i].DoubleVal;
  } else {
    assert(SrcVal->getType()->isDoubleTy() && DstTy->isFloatTy() &&
           "Invalid FPTrunc instruction");
    Dest.FloatVal = (float)Src.DoubleVal;
  }

  return Dest;
}

GenericValue Interpreter::executeFPExtInst(Value *SrcVal, Type *DstTy,
                                           ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcVal->getType()->getTypeID() == Type::VectorTyID) {
    assert(SrcVal->getType()->getScalarType()->isFloatTy() &&
           DstTy->getScalarType()->isDoubleTy() && "Invalid FPExt instruction");

    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);
    for (unsigned i = 0; i < size; i++)
      Dest.AggregateVal[i].DoubleVal = (double)Src.AggregateVal[i].FloatVal;
  } else {
    assert(SrcVal->getType()->isFloatTy() && DstTy->isDoubleTy() &&
           "Invalid FPExt instruction");
    Dest.DoubleVal = (double)Src.FloatVal;
  }

  return Dest;
}

GenericValue Interpreter::executeFPToUIInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcTy->getTypeID() == Type::VectorTyID) {
    const Type *DstVecTy = DstTy->getScalarType();
    const Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal.
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToUI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
            Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
            Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToUI instruction");

    if (SrcTy->getTypeID() == Type::FloatTyID)
      Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else {
      Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
    }
  }

  return Dest;
}

GenericValue Interpreter::executeFPToSIInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcTy->getTypeID() == Type::VectorTyID) {
    const Type *DstVecTy = DstTy->getScalarType();
    const Type *SrcVecTy = SrcTy->getScalarType();
    uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (SrcVecTy->getTypeID() == Type::FloatTyID) {
      assert(SrcVecTy->isFloatingPointTy() && "Invalid FPToSI instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
            Src.AggregateVal[i].FloatVal, DBitWidth);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
            Src.AggregateVal[i].DoubleVal, DBitWidth);
    }
  } else {
    // scalar
    unsigned DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
    assert(SrcTy->isFloatingPointTy() && "Invalid FPToSI instruction");

    if (SrcTy->getTypeID() == Type::FloatTyID)
      Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
    else {
      Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
    }
  }
  return Dest;
}

GenericValue Interpreter::executeUIToFPInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcVal->getType()->getTypeID() == Type::VectorTyID) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid UIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
            APIntOps::RoundAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
            APIntOps::RoundAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid UIToFP instruction");
    if (DstTy->getTypeID() == Type::FloatTyID)
      Dest.FloatVal = APIntOps::RoundAPIntToFloat(Src.IntVal);
    else {
      Dest.DoubleVal = APIntOps::RoundAPIntToDouble(Src.IntVal);
    }
  }
  return Dest;
}

GenericValue Interpreter::executeSIToFPInst(Value *SrcVal, Type *DstTy,
                                            ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if (SrcVal->getType()->getTypeID() == Type::VectorTyID) {
    const Type *DstVecTy = DstTy->getScalarType();
    unsigned size = Src.AggregateVal.size();
    // the sizes of src and dst vectors must be equal
    Dest.AggregateVal.resize(size);

    if (DstVecTy->getTypeID() == Type::FloatTyID) {
      assert(DstVecTy->isFloatingPointTy() && "Invalid SIToFP instruction");
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].FloatVal =
            APIntOps::RoundSignedAPIntToFloat(Src.AggregateVal[i].IntVal);
    } else {
      for (unsigned i = 0; i < size; i++)
        Dest.AggregateVal[i].DoubleVal =
            APIntOps::RoundSignedAPIntToDouble(Src.AggregateVal[i].IntVal);
    }
  } else {
    // scalar
    assert(DstTy->isFloatingPointTy() && "Invalid SIToFP instruction");

    if (DstTy->getTypeID() == Type::FloatTyID)
      Dest.FloatVal = APIntOps::RoundSignedAPIntToFloat(Src.IntVal);
    else {
      Dest.DoubleVal = APIntOps::RoundSignedAPIntToDouble(Src.IntVal);
    }
  }

  return Dest;
}

GenericValue Interpreter::executePtrToIntInst(Value *SrcVal, Type *DstTy,
                                              ExecutionContext &SF) {
  uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(SrcVal->getType()->isPointerTy() && "Invalid PtrToInt instruction");

  Dest.IntVal = APInt(DBitWidth, (intptr_t) Src.PointerVal);
  return Dest;
}

GenericValue Interpreter::executeIntToPtrInst(Value *SrcVal, Type *DstTy,
                                              ExecutionContext &SF) {
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);
  assert(DstTy->isPointerTy() && "Invalid PtrToInt instruction");

  uint32_t PtrSize = TD.getPointerSizeInBits();
  if (PtrSize != Src.IntVal.getBitWidth())
    Src.IntVal = Src.IntVal.zextOrTrunc(PtrSize);

  Dest.PointerVal = PointerTy(intptr_t(Src.IntVal.getZExtValue()));
  return Dest;
}

GenericValue Interpreter::executeBitCastInst(Value *SrcVal, Type *DstTy,
                                             ExecutionContext &SF) {

  // This instruction supports bitwise conversion of vectors to integers and
  // to vectors of other types (as long as they have the same size)
  Type *SrcTy = SrcVal->getType();
  GenericValue Dest, Src = getOperandValue(SrcVal, SF);

  if ((SrcTy->getTypeID() == Type::VectorTyID) ||
      (DstTy->getTypeID() == Type::VectorTyID)) {
    // vector src bitcast to vector dst or vector src bitcast to scalar dst or
    // scalar src bitcast to vector dst
    bool isLittleEndian = TD.isLittleEndian();
    GenericValue TempDst, TempSrc, SrcVec;
    const Type *SrcElemTy;
    const Type *DstElemTy;
    unsigned SrcBitSize;
    unsigned DstBitSize;
    unsigned SrcNum;
    unsigned DstNum;

    if (SrcTy->getTypeID() == Type::VectorTyID) {
      SrcElemTy = SrcTy->getScalarType();
      SrcBitSize = SrcTy->getScalarSizeInBits();
      SrcNum = Src.AggregateVal.size();
      SrcVec = Src;
    } else {
      // if src is scalar value, make it vector <1 x type>
      SrcElemTy = SrcTy;
      SrcBitSize = SrcTy->getPrimitiveSizeInBits();
      SrcNum = 1;
      SrcVec.AggregateVal.push_back(Src);
    }

    if (DstTy->getTypeID() == Type::VectorTyID) {
      DstElemTy = DstTy->getScalarType();
      DstBitSize = DstTy->getScalarSizeInBits();
      DstNum = (SrcNum * SrcBitSize) / DstBitSize;
    } else {
      DstElemTy = DstTy;
      DstBitSize = DstTy->getPrimitiveSizeInBits();
      DstNum = 1;
    }

    if (SrcNum * SrcBitSize != DstNum * DstBitSize)
      llvm_unreachable("Invalid BitCast");

    // If src is floating point, cast to integer first.
    TempSrc.AggregateVal.resize(SrcNum);
    if (SrcElemTy->isFloatTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal =
            APInt::floatToBits(SrcVec.AggregateVal[i].FloatVal);

    } else if (SrcElemTy->isDoubleTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal =
            APInt::doubleToBits(SrcVec.AggregateVal[i].DoubleVal);
    } else if (SrcElemTy->isIntegerTy()) {
      for (unsigned i = 0; i < SrcNum; i++)
        TempSrc.AggregateVal[i].IntVal = SrcVec.AggregateVal[i].IntVal;
    } else {
      // Pointers are not allowed as the element type of vector.
      llvm_unreachable("Invalid Bitcast");
    }

    // now TempSrc is integer type vector
    if (DstNum < SrcNum) {
      // Example: bitcast <4 x i32> <i32 0, i32 1, i32 2, i32 3> to <2 x i64>
      unsigned Ratio = SrcNum / DstNum;
      unsigned SrcElt = 0;
      for (unsigned i = 0; i < DstNum; i++) {
        GenericValue Elt;
        Elt.IntVal = 0;
        Elt.IntVal = Elt.IntVal.zext(DstBitSize);
        unsigned ShiftAmt = isLittleEndian ? 0 : SrcBitSize * (Ratio - 1);
        for (unsigned j = 0; j < Ratio; j++) {
          APInt Tmp;
          Tmp = Tmp.zext(SrcBitSize);
          Tmp = TempSrc.AggregateVal[SrcElt++].IntVal;
          Tmp = Tmp.zext(DstBitSize);
          Tmp = Tmp.shl(ShiftAmt);
          ShiftAmt += isLittleEndian ? SrcBitSize : -SrcBitSize;
          Elt.IntVal |= Tmp;
        }
        TempDst.AggregateVal.push_back(Elt);
      }
    } else {
      // Example: bitcast <2 x i64> <i64 0, i64 1> to <4 x i32>
      unsigned Ratio = DstNum / SrcNum;
      for (unsigned i = 0; i < SrcNum; i++) {
        unsigned ShiftAmt = isLittleEndian ? 0 : DstBitSize * (Ratio - 1);
        for (unsigned j = 0; j < Ratio; j++) {
          GenericValue Elt;
          Elt.IntVal = Elt.IntVal.zext(SrcBitSize);
          Elt.IntVal = TempSrc.AggregateVal[i].IntVal;
          Elt.IntVal = Elt.IntVal.lshr(ShiftAmt);
          // it could be DstBitSize == SrcBitSize, so check it
          if (DstBitSize < SrcBitSize)
            Elt.IntVal = Elt.IntVal.trunc(DstBitSize);
          ShiftAmt += isLittleEndian ? DstBitSize : -DstBitSize;
          TempDst.AggregateVal.push_back(Elt);
        }
      }
    }

    // convert result from integer to specified type
    if (DstTy->getTypeID() == Type::VectorTyID) {
      if (DstElemTy->isDoubleTy()) {
        Dest.AggregateVal.resize(DstNum);
        for (unsigned i = 0; i < DstNum; i++)
          Dest.AggregateVal[i].DoubleVal =
              TempDst.AggregateVal[i].IntVal.bitsToDouble();
      } else if (DstElemTy->isFloatTy()) {
        Dest.AggregateVal.resize(DstNum);
        for (unsigned i = 0; i < DstNum; i++)
          Dest.AggregateVal[i].FloatVal =
              TempDst.AggregateVal[i].IntVal.bitsToFloat();
      } else {
        Dest = TempDst;
      }
    } else {
      if (DstElemTy->isDoubleTy())
        Dest.DoubleVal = TempDst.AggregateVal[0].IntVal.bitsToDouble();
      else if (DstElemTy->isFloatTy()) {
        Dest.FloatVal = TempDst.AggregateVal[0].IntVal.bitsToFloat();
      } else {
        Dest.IntVal = TempDst.AggregateVal[0].IntVal;
      }
    }
  } else { //  if ((SrcTy->getTypeID() == Type::VectorTyID) ||
           //     (DstTy->getTypeID() == Type::VectorTyID))

    // scalar src bitcast to scalar dst
    if (DstTy->isPointerTy()) {
      assert(SrcTy->isPointerTy() && "Invalid BitCast");
      Dest.PointerVal = Src.PointerVal;
    } else if (DstTy->isIntegerTy()) {
      if (SrcTy->isFloatTy())
        Dest.IntVal = APInt::floatToBits(Src.FloatVal);
      else if (SrcTy->isDoubleTy()) {
        Dest.IntVal = APInt::doubleToBits(Src.DoubleVal);
      } else if (SrcTy->isIntegerTy()) {
        Dest.IntVal = Src.IntVal;
      } else {
        llvm_unreachable("Invalid BitCast");
      }
    } else if (DstTy->isFloatTy()) {
      if (SrcTy->isIntegerTy())
        Dest.FloatVal = Src.IntVal.bitsToFloat();
      else {
        Dest.FloatVal = Src.FloatVal;
      }
    } else if (DstTy->isDoubleTy()) {
      if (SrcTy->isIntegerTy())
        Dest.DoubleVal = Src.IntVal.bitsToDouble();
      else {
        Dest.DoubleVal = Src.DoubleVal;
      }
    } else {
      llvm_unreachable("Invalid Bitcast");
    }
  }

  return Dest;
}

void Interpreter::visitTruncInst(TruncInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		SetValue(&I, executeTruncInst(I.getOperand(0), I.getType(), SF), SF);
		return;
	}
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSExtInst(SExtInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeSExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitZExtInst(ZExtInst &I) {
	if (!dryRun) {
		ExecutionContext &SF = getECStack()->back();
		SetValue(&I, executeZExtInst(I.getOperand(0), I.getType(), SF), SF);
		return;
	}
		
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeZExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPTruncInst(FPTruncInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPExtInst(FPExtInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitUIToFPInst(UIToFPInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeUIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitSIToFPInst(SIToFPInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeSIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToUIInst(FPToUIInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPToUIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitFPToSIInst(FPToSIInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeFPToSIInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitPtrToIntInst(PtrToIntInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executePtrToIntInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitIntToPtrInst(IntToPtrInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeIntToPtrInst(I.getOperand(0), I.getType(), SF), SF);
}

void Interpreter::visitBitCastInst(BitCastInst &I) {
  ExecutionContext &SF = ECStack.back();
  SetValue(&I, executeBitCastInst(I.getOperand(0), I.getType(), SF), SF);
}

#define IMPLEMENT_VAARG(TY) \
   case Type::TY##TyID: Dest.TY##Val = Src.TY##Val; break

void Interpreter::visitVAArgInst(VAArgInst &I) {
  ExecutionContext &SF = ECStack.back();

  // Get the incoming valist parameter.  LLI treats the valist as a
  // (ec-stack-depth var-arg-index) pair.
  GenericValue VAList = getOperandValue(I.getOperand(0), SF);
  GenericValue Dest;
  GenericValue Src = ECStack[VAList.UIntPairVal.first]
                      .VarArgs[VAList.UIntPairVal.second];
  Type *Ty = I.getType();
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    Dest.IntVal = Src.IntVal;
    break;
  IMPLEMENT_VAARG(Pointer);
  IMPLEMENT_VAARG(Float);
  IMPLEMENT_VAARG(Double);
  default:
    dbgs() << "Unhandled dest type for vaarg instruction: " << *Ty << "\n";
    llvm_unreachable(nullptr);
  }

  // Set the Value of this Instruction.
  SetValue(&I, Dest, SF);

  // Move the pointer to the next vararg.
  ++VAList.UIntPairVal.second;
}

void Interpreter::visitExtractElementInst(ExtractElementInst &I) {
  ExecutionContext &SF = ECStack.back();
  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest;

  Type *Ty = I.getType();
  const unsigned indx = unsigned(Src2.IntVal.getZExtValue());

  if(Src1.AggregateVal.size() > indx) {
    switch (Ty->getTypeID()) {
    default:
      dbgs() << "Unhandled destination type for extractelement instruction: "
      << *Ty << "\n";
      llvm_unreachable(nullptr);
      break;
    case Type::IntegerTyID:
      Dest.IntVal = Src1.AggregateVal[indx].IntVal;
      break;
    case Type::FloatTyID:
      Dest.FloatVal = Src1.AggregateVal[indx].FloatVal;
      break;
    case Type::DoubleTyID:
      Dest.DoubleVal = Src1.AggregateVal[indx].DoubleVal;
      break;
    }
  } else {
    dbgs() << "Invalid index in extractelement instruction\n";
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitInsertElementInst(InsertElementInst &I) {
  ExecutionContext &SF = ECStack.back();
  Type *Ty = I.getType();

  if(!(Ty->isVectorTy()) )
    llvm_unreachable("Unhandled dest type for insertelement instruction");

  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue Dest;

  Type *TyContained = Ty->getContainedType(0);

  const unsigned indx = unsigned(Src3.IntVal.getZExtValue());
  Dest.AggregateVal = Src1.AggregateVal;

  if(Src1.AggregateVal.size() <= indx)
      llvm_unreachable("Invalid index in insertelement instruction");
  switch (TyContained->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
    case Type::IntegerTyID:
      Dest.AggregateVal[indx].IntVal = Src2.IntVal;
      break;
    case Type::FloatTyID:
      Dest.AggregateVal[indx].FloatVal = Src2.FloatVal;
      break;
    case Type::DoubleTyID:
      Dest.AggregateVal[indx].DoubleVal = Src2.DoubleVal;
      break;
  }
  SetValue(&I, Dest, SF);
}

void Interpreter::visitShuffleVectorInst(ShuffleVectorInst &I){
  ExecutionContext &SF = ECStack.back();

  Type *Ty = I.getType();
  if(!(Ty->isVectorTy()))
    llvm_unreachable("Unhandled dest type for shufflevector instruction");

  GenericValue Src1 = getOperandValue(I.getOperand(0), SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Src3 = getOperandValue(I.getOperand(2), SF);
  GenericValue Dest;

  // There is no need to check types of src1 and src2, because the compiled
  // bytecode can't contain different types for src1 and src2 for a
  // shufflevector instruction.

  Type *TyContained = Ty->getContainedType(0);
  unsigned src1Size = (unsigned)Src1.AggregateVal.size();
  unsigned src2Size = (unsigned)Src2.AggregateVal.size();
  unsigned src3Size = (unsigned)Src3.AggregateVal.size();

  Dest.AggregateVal.resize(src3Size);

  switch (TyContained->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
      break;
    case Type::IntegerTyID:
      for( unsigned i=0; i<src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].IntVal = Src1.AggregateVal[j].IntVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].IntVal = Src2.AggregateVal[j-src1Size].IntVal;
        else
          // The selector may not be greater than sum of lengths of first and
          // second operands and llasm should not allow situation like
          // %tmp = shufflevector <2 x i32> <i32 3, i32 4>, <2 x i32> undef,
          //                      <2 x i32> < i32 0, i32 5 >,
          // where i32 5 is invalid, but let it be additional check here:
          llvm_unreachable("Invalid mask in shufflevector instruction");
      }
      break;
    case Type::FloatTyID:
      for( unsigned i=0; i<src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].FloatVal = Src1.AggregateVal[j].FloatVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].FloatVal = Src2.AggregateVal[j-src1Size].FloatVal;
        else
          llvm_unreachable("Invalid mask in shufflevector instruction");
        }
      break;
    case Type::DoubleTyID:
      for( unsigned i=0; i<src3Size; i++) {
        unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
        if(j < src1Size)
          Dest.AggregateVal[i].DoubleVal = Src1.AggregateVal[j].DoubleVal;
        else if(j < src1Size + src2Size)
          Dest.AggregateVal[i].DoubleVal =
            Src2.AggregateVal[j-src1Size].DoubleVal;
        else
          llvm_unreachable("Invalid mask in shufflevector instruction");
      }
      break;
  }
  SetValue(&I, Dest, SF);
}

void Interpreter::visitExtractValueInst(ExtractValueInst &I) {
	if (dryRun) {
		dbgs() << "Please do not use RMWs outside of thread code!\n";
		return;
	}
	
  ExecutionContext &SF = getECStack()->back();
  Value *Agg = I.getAggregateOperand();
  GenericValue Dest;
  GenericValue Src = getOperandValue(Agg, SF);

  ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();
  GenericValue *pSrc = &Src;

  for (unsigned i = 0 ; i < Num; ++i) {
    pSrc = &pSrc->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }

  Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());
  switch (IndexedType->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for extractelement instruction");
    break;
    case Type::IntegerTyID:
      Dest.IntVal = pSrc->IntVal;
    break;
    case Type::FloatTyID:
      Dest.FloatVal = pSrc->FloatVal;
    break;
    case Type::DoubleTyID:
      Dest.DoubleVal = pSrc->DoubleVal;
    break;
    case Type::ArrayTyID:
    case Type::StructTyID:
    case Type::VectorTyID:
      Dest.AggregateVal = pSrc->AggregateVal;
    break;
    case Type::PointerTyID:
      Dest.PointerVal = pSrc->PointerVal;
    break;
  }

  SetValue(&I, Dest, SF);
}

void Interpreter::visitInsertValueInst(InsertValueInst &I) {

  ExecutionContext &SF = ECStack.back();
  Value *Agg = I.getAggregateOperand();

  GenericValue Src1 = getOperandValue(Agg, SF);
  GenericValue Src2 = getOperandValue(I.getOperand(1), SF);
  GenericValue Dest = Src1; // Dest is a slightly changed Src1

  ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
  unsigned Num = I.getNumIndices();

  GenericValue *pDest = &Dest;
  for (unsigned i = 0 ; i < Num; ++i) {
    pDest = &pDest->AggregateVal[*IdxBegin];
    ++IdxBegin;
  }
  // pDest points to the target value in the Dest now

  Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());

  switch (IndexedType->getTypeID()) {
    default:
      llvm_unreachable("Unhandled dest type for insertelement instruction");
    break;
    case Type::IntegerTyID:
      pDest->IntVal = Src2.IntVal;
    break;
    case Type::FloatTyID:
      pDest->FloatVal = Src2.FloatVal;
    break;
    case Type::DoubleTyID:
      pDest->DoubleVal = Src2.DoubleVal;
    break;
    case Type::ArrayTyID:
    case Type::StructTyID:
    case Type::VectorTyID:
      pDest->AggregateVal = Src2.AggregateVal;
    break;
    case Type::PointerTyID:
      pDest->PointerVal = Src2.PointerVal;
    break;
  }

  SetValue(&I, Dest, SF);
}

GenericValue Interpreter::getConstantExprValue (ConstantExpr *CE,
                                                ExecutionContext &SF) {
  switch (CE->getOpcode()) {
  case Instruction::Trunc:
      return executeTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::ZExt:
      return executeZExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SExt:
      return executeSExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPTrunc:
      return executeFPTruncInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPExt:
      return executeFPExtInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::UIToFP:
      return executeUIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::SIToFP:
      return executeSIToFPInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToUI:
      return executeFPToUIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::FPToSI:
      return executeFPToSIInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::PtrToInt:
      return executePtrToIntInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::IntToPtr:
      return executeIntToPtrInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::BitCast:
      return executeBitCastInst(CE->getOperand(0), CE->getType(), SF);
  case Instruction::GetElementPtr:
    return executeGEPOperation(CE->getOperand(0), gep_type_begin(CE),
                               gep_type_end(CE), SF);
  case Instruction::FCmp:
  case Instruction::ICmp:
    return executeCmpInst(CE->getPredicate(),
                          getOperandValue(CE->getOperand(0), SF),
                          getOperandValue(CE->getOperand(1), SF),
                          CE->getOperand(0)->getType());
  case Instruction::Select:
    return executeSelectInst(getOperandValue(CE->getOperand(0), SF),
                             getOperandValue(CE->getOperand(1), SF),
                             getOperandValue(CE->getOperand(2), SF),
                             CE->getOperand(0)->getType());
  default :
    break;
  }

  // The cases below here require a GenericValue parameter for the result
  // so we initialize one, compute it and then return it.
  GenericValue Op0 = getOperandValue(CE->getOperand(0), SF);
  GenericValue Op1 = getOperandValue(CE->getOperand(1), SF);
  GenericValue Dest;
  Type * Ty = CE->getOperand(0)->getType();
  switch (CE->getOpcode()) {
  case Instruction::Add:  Dest.IntVal = Op0.IntVal + Op1.IntVal; break;
  case Instruction::Sub:  Dest.IntVal = Op0.IntVal - Op1.IntVal; break;
  case Instruction::Mul:  Dest.IntVal = Op0.IntVal * Op1.IntVal; break;
  case Instruction::FAdd: executeFAddInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FSub: executeFSubInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FMul: executeFMulInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FDiv: executeFDivInst(Dest, Op0, Op1, Ty); break;
  case Instruction::FRem: executeFRemInst(Dest, Op0, Op1, Ty); break;
  case Instruction::SDiv: Dest.IntVal = Op0.IntVal.sdiv(Op1.IntVal); break;
  case Instruction::UDiv: Dest.IntVal = Op0.IntVal.udiv(Op1.IntVal); break;
  case Instruction::URem: Dest.IntVal = Op0.IntVal.urem(Op1.IntVal); break;
  case Instruction::SRem: Dest.IntVal = Op0.IntVal.srem(Op1.IntVal); break;
  case Instruction::And:  Dest.IntVal = Op0.IntVal & Op1.IntVal; break;
  case Instruction::Or:   Dest.IntVal = Op0.IntVal | Op1.IntVal; break;
  case Instruction::Xor:  Dest.IntVal = Op0.IntVal ^ Op1.IntVal; break;
  case Instruction::Shl:  
    Dest.IntVal = Op0.IntVal.shl(Op1.IntVal.getZExtValue());
    break;
  case Instruction::LShr: 
    Dest.IntVal = Op0.IntVal.lshr(Op1.IntVal.getZExtValue());
    break;
  case Instruction::AShr: 
    Dest.IntVal = Op0.IntVal.ashr(Op1.IntVal.getZExtValue());
    break;
  default:
    dbgs() << "Unhandled ConstantExpr: " << *CE << "\n";
    llvm_unreachable("Unhandled ConstantExpr");
  }
  return Dest;
}

GenericValue Interpreter::getOperandValue(Value *V, ExecutionContext &SF) {
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
    return getConstantExprValue(CE, SF);
  } else if (Constant *CPV = dyn_cast<Constant>(V)) {
    return getConstantValue(CPV);
  } else if (GlobalValue *GV = dyn_cast<GlobalValue>(V)) {
    return PTOGV(getPointerToGlobal(GV));
  } else {
    return SF.Values[V];
  }
}

//===----------------------------------------------------------------------===//
//                        Dispatch and Execution Code
//===----------------------------------------------------------------------===//

/* callAssertFail - Call to assert() macro */
void Interpreter::callAssertFail(Function *F,
				 const std::vector<GenericValue> &ArgVals)
{
	std::string err;

	err = (ArgVals.size()) ? (char *) GVTOP(ArgVals[0]) : "Unknown";
/* TODO: Construct an Error class that will take care of the error handling */
	dbgs() << "Assertion violation: " << err << "\n";
	abort();
}

/* callPthreadCreate - Call to pthread_create() function */
void Interpreter::callPthreadCreate(Function *F,
				    const std::vector<GenericValue> &ArgVals)
{
	Function *calledFun = (Function*) GVTOP(ArgVals[2]);
	ExecutionContext SF;
	std::vector<llvm::ExecutionContext> stackSF;
	
	if (dryRun) {
		initNumThreads++;
		Thread thr(calledFun, initNumThreads);
		initGraph.threads.push_back(thr);
		
		SF.CurFunction = calledFun;
		SF.CurBB = &calledFun->front();
		SF.CurInst = SF.CurBB->begin();
		
		// Run through the function arguments and initialize their values...
		assert((ArgVals.size() == calledFun->arg_size() ||
			(ArgVals.size() > calledFun->arg_size() &&
			 calledFun->getFunctionType()->isVarArg()))&&
		       "Invalid number of values passed to function invocation!");
		
		// Handle non-varargs arguments...
		unsigned i = 0;
		for (Function::arg_iterator AI = calledFun->arg_begin(), E = calledFun->arg_end();
		     AI != E; ++AI, ++i){
			SetValue(&*AI, ArgVals[i], SF);
		}
		
		// Handle varargs arguments...
		SF.VarArgs.assign(ArgVals.begin()+i, ArgVals.end());

		stackSF.push_back(SF);
		initStacks.push_back(stackSF);
	}
}

/* callPthreadJoin - Call to pthread_join() function */
void Interpreter::callPthreadJoin(Function *F,
				  const std::vector<GenericValue> &ArgVals)
{
}

/* callPthreadExit - Call to pthread_exit() function */
void Interpreter::callPthreadExit(Function *F,
				  const std::vector<GenericValue> &ArgVals)
{
}

//===----------------------------------------------------------------------===//
// callFunction - Execute the specified function...
//
void Interpreter::callFunction(Function *F,
                               const std::vector<GenericValue> &ArgVals)
{
  /* Custom function support */
  std::string functionName = F->getName().str();
  if (functionName == "__assert_fail") {
	  callAssertFail(F, ArgVals);
	  return;
  } else if (functionName == "pthread_create") {
	  callPthreadCreate(F, ArgVals);
	  return;
  } else if (functionName == "pthread_join") {
	  callPthreadJoin(F, ArgVals);
	  return;
  } else if (functionName == "pthread_exit") {
	  callPthreadExit(F, ArgVals);
	  return;
  }
	
  assert((ECStack.empty() || !ECStack.back().Caller.getInstruction() ||
          ECStack.back().Caller.arg_size() == ArgVals.size()) &&
         "Incorrect number of arguments passed into function call!");
  // Make a new stack frame... and fill it in.
  ECStack.push_back(ExecutionContext());
  ExecutionContext &StackFrame = ECStack.back();
  StackFrame.CurFunction = F;

  // Special handling for external functions.
  if (F->isDeclaration()) {
    GenericValue Result = callExternalFunction (F, ArgVals);
    // Simulate a 'ret' instruction of the appropriate type.
    popStackAndReturnValueToCaller (F->getReturnType (), Result);
    return;
  }

  // Get pointers to first LLVM BB & Instruction in function.
  StackFrame.CurBB     = &F->front();
  StackFrame.CurInst   = StackFrame.CurBB->begin();

  // Run through the function arguments and initialize their values...
  assert((ArgVals.size() == F->arg_size() ||
         (ArgVals.size() > F->arg_size() && F->getFunctionType()->isVarArg()))&&
         "Invalid number of values passed to function invocation!");

  // Handle non-varargs arguments...
  unsigned i = 0;
  for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end(); 
       AI != E; ++AI, ++i)
    SetValue(&*AI, ArgVals[i], StackFrame);

  // Handle varargs arguments...
  StackFrame.VarArgs.assign(ArgVals.begin()+i, ArgVals.end());
}

bool Interpreter::scheduleNext(ExecutionGraph &g)
{
	for (unsigned int i = 0; i < g.threads.size(); i++) {
		if (!ECStacks[i].empty()) {
			g.currentT = i;
			return true;
		}
	}
	return false;
}

void Interpreter::visitGraph(ExecutionGraph &g)
{
	std::vector<RevisitPair> workqueue;

	ExecutionGraph *oldEG = currentEG;
	std::vector<RevisitPair> *oldStack = currentStack;
	std::vector<std::vector<ExecutionContext> > oldECStacks = ECStacks;
	bool oldContinue = shouldContinue;
	currentEG = &g;
	currentStack = &workqueue;
	ECStacks = initStacks;
	std::vector<int> oldGlobalCount;
	for (int i = 0; i < initNumThreads; i++) {
		oldGlobalCount.push_back(globalCount[i]);
		globalCount[i] = 0;
	}

	while (true) {
//		validateGraph(g);
		if (scheduleNext(g)) {
			shouldContinue = true;
			ExecutionContext &SF = getECStack()->back();
			Instruction &I = *SF.CurInst++;
			visit(I);
		} else {
			shouldContinue = false;
		}

		if (shouldContinue)
			continue;

		if (workqueue.empty()) {
//			std::cerr << "Workqueue empty, graph:\n";
//			printExecGraph(g);
			explored++;
			for (int i = 0; i < initNumThreads; i++)
				globalCount[i] = oldGlobalCount[i];
			shouldContinue = oldContinue;
			ECStacks = oldECStacks;
			currentStack = oldStack;
			currentEG = oldEG;
			return;
		}
			
		RevisitPair &p = workqueue.back();
//		std::cerr << "Popping from workqueue...\n";

		if (p.type == R) {
//			printRevisitPair(p);
//			printExecGraph(g);
			cutGraphBefore(g, p.preds);
			
			EventLabel &lab1 = getEventLabel(g, p.e);
			Event oldRf = lab1.rf;
			lab1.rf = p.rf;
			if (!p.rf.isInitializer()) {
				EventLabel &lab2 = getEventLabel(g, p.rf);
				lab2.rfm1.push_front(p.e);
			}
			if (!oldRf.isInitializer()) {
				EventLabel &lab3 = getEventLabel(g, oldRf);
				lab3.rfm1.remove(p.e);
			}
			std::vector<int> before = calcPorfBefore(g, p.e);
			for (std::list<Event>::iterator it = g.revisit.begin();
			     it != g.revisit.end(); ++it)
				if (it->eventIndex <= before[it->threadIndex])
					g.revisit.erase(it--);
//			printExecGraph(g);
			explored++;
			ECStacks = initStacks;
			for (int i = 0; i < initNumThreads; i++)
				globalCount[i] = 0;
		} else {
			WARN("This should not happen\n");
		}
		workqueue.pop_back();
	}
}


void Interpreter::run() {
  while (!ECStack.empty()) {
    // Interpret a single instruction & increment the "PC".
    ExecutionContext &SF = ECStack.back();  // Current stack frame
    Instruction &I = *SF.CurInst++;         // Increment before execute

    // Track the number of dynamic instructions executed.
    ++NumDynamicInsts;

    DEBUG(dbgs() << "About to interpret: " << I);
    visit(I);   // Dispatch to one of the visit* methods...
#if 0
    // This is not safe, as visiting the instruction could lower it and free I.
DEBUG(
    if (!isa<CallInst>(I) && !isa<InvokeInst>(I) && 
        I.getType() != Type::VoidTy) {
      dbgs() << "  --> ";
      const GenericValue &Val = SF.Values[&I];
      switch (I.getType()->getTypeID()) {
      default: llvm_unreachable("Invalid GenericValue Type");
      case Type::VoidTyID:    dbgs() << "void"; break;
      case Type::FloatTyID:   dbgs() << "float " << Val.FloatVal; break;
      case Type::DoubleTyID:  dbgs() << "double " << Val.DoubleVal; break;
      case Type::PointerTyID: dbgs() << "void* " << intptr_t(Val.PointerVal);
        break;
      case Type::IntegerTyID: 
        dbgs() << "i" << Val.IntVal.getBitWidth() << " "
               << Val.IntVal.toStringUnsigned(10)
               << " (0x" << Val.IntVal.toStringUnsigned(16) << ")\n";
        break;
      }
    });
#endif
  }

  dryRun = false;
  /* There is not an event for any thread -- yet */
  /* maxEvents points to the array index of the next event */ 
  for (int i = 0; i < initNumThreads; i++) {
	  Thread &t = initGraph.threads[i];
	  t.eventList.push_back(EventLabel(NA, Event(-1, -1)));
	  initGraph.maxEvents.push_back(1);
	  globalCount.push_back(0);
  }

  visitGraph(initGraph);

  std::cerr << "Number of explored executions: " << explored << std::endl;
}
