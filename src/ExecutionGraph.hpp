/*
 * RCMC -- Model Checking for C11 programs.
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
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#ifndef __EXECUTION_GRAPH_HPP__
#define __EXECUTION_GRAPH_HPP__

#include "Config.hpp"
#include "Error.hpp"
#include "Event.hpp"
#include "ModOrder.hpp"
#include "RevisitSet.hpp"
#include "Thread.hpp"

#include <unordered_set>

enum StackItemType { RevR, RevW };
/*
 * StackItem class - This class represents alternative choices for reads which
 * are pushed to a stack to be visited later.
 */
struct StackItem {
	StackItemType type;
	Event e;
	Event rf;
	Event shouldRf;
	std::vector<int> preds;
	RevisitSet revisit;
	llvm::GenericValue *addr;
	std::vector<Event> locMO;
	Event prevMO;

	StackItem(Event e, Event rf, Event shouldRf, std::vector<int> preds,
		  RevisitSet revisit)
		: type(RevR), e(e), rf(rf), shouldRf(shouldRf), preds(preds), revisit(revisit) {};
	StackItem(Event e, Event prevMO, std::vector<int> preds, RevisitSet revisit,
		  llvm::GenericValue *addr, std::vector<Event> locMO)
		: type(RevW), e(e), preds(preds), revisit(revisit),
		  addr(addr), locMO(locMO), prevMO(prevMO) {}
};

/*
 * ExecutionGraph class - This class represents an execution graph
 */
class ExecutionGraph {
public:
	std::vector<Thread> threads;
	std::vector<int> maxEvents;
	RevisitSet revisit;
	ModOrder modOrder;
	std::vector<StackItem> workqueue;
	std::vector<void *> stackAllocas;
	std::vector<void *> heapAllocas;
	std::vector<void *> freedMem;
	int currentT;

	/* Constructors */
	ExecutionGraph();
	// ExecutionGraph(std::vector<Thread> ts, std::vector<int> es,
	// 	       std::vector<Event> re, int t)
	// 	: threads(ts), maxEvents(es), revisit(re), currentT(t) {};

	/* Basic getter methods */
	EventLabel& getEventLabel(Event &e);
	EventLabel& getPreviousLabel(Event &e);
	Event getLastThreadEvent(int thread);
	Event getLastThreadRelease(int thread, llvm::GenericValue *addr);
	View getEventHbView(Event e);
	View getEventMsgView(Event e);
	std::vector<llvm::GenericValue *> getDoubleLocs();
	std::vector<int> getGraphState(void);
	std::vector<llvm::ExecutionContext> &getThreadECStack(int thread);
	std::vector<Event> getRevisitLoads(Event store);
	std::vector<Event> getRevisitLoadsNonMaximal(Event store);

	/* Basic setter methods */
	void addReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			    llvm::Type *typ, Event rf);
	void addCASReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			       llvm::GenericValue &val, llvm::Type *typ, Event rf);
	void addRMWReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			       llvm::Type *typ, Event rf);
	void addStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			     llvm::GenericValue &val, llvm::Type *typ);
	void addCASStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				llvm::GenericValue &val, llvm::Type *typ);
	void addRMWStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				llvm::GenericValue &val, llvm::Type *typ);
	void addFenceToGraph(llvm::AtomicOrdering ord);

	/* Calculation of [(po U rf)*] predecessors and successors */
	std::vector<int> getPorfAfter(Event e);
	std::vector<int> getPorfAfter(const std::vector<Event > &es);
	std::vector<int> getPorfBefore(Event e);
	std::vector<int> getPorfBefore(const std::vector<Event> &es);
	std::vector<int> getPorfBeforeNoRfs(const std::vector<Event> &es);
	std::vector<int> getHbBefore(Event e);
	std::vector<int> getHbBefore(const std::vector<Event> &es);
	std::vector<int> getHbPoBefore(Event e);

	/* Calculation of writes a read can read from */
	std::vector<Event> getStoresToLoc(llvm::GenericValue *addr, ModelType model);
	std::vector<std::vector<Event> > splitLocMOBefore(std::vector<int> &before,
							  std::vector<Event> &stores);

	/* Graph modification methods */
	void cutBefore(std::vector<int> &preds, RevisitSet &rev);
	void cutToCopyAfter(ExecutionGraph &other, std::vector<int> &after);
	void modifyRfs(std::vector<Event> &es, Event store);
	void clearAllStacks(void);

	/* Consistency checks */
	bool isConsistent();

	/* Graph exploration methods */
	bool scheduleNext(void);
	void tryToBacktrack(void);

	/* Race detection methods */
	Event findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);
	Event findRaceForNewStore(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);

	/* PSC calculation */
	bool isPscAcyclic();

	/* Debugging methods */
	void validateGraph(void);

	/* Printing facilities */
	void printTraceBefore(Event e);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	void addEventToGraph(EventLabel &lab);
	void addReadToGraphCommon(EventLabel &lab, Event &rf);
	void addStoreToGraphCommon(EventLabel &lab);
	void calcPorfAfter(const Event &e, std::vector<int> &a);
	void calcPorfBefore(const Event &e, std::vector<int> &a);
	void calcRelRfPoBefore(int thread, int index, View &v);
	void calcTraceBefore(const Event &e, std::vector<int> &a, std::stringstream &buf);
	std::vector<Event> calcOptionalRfs(Event store, std::vector<Event> &locMO);
	bool isWriteRfBefore(std::vector<int> &before, Event e);
	std::vector<Event> findOverwrittenBoundary(llvm::GenericValue *addr, int thread);
	std::vector<Event> getStoresWeakRA(llvm::GenericValue *addr);
	std::vector<Event> getStoresMO(llvm::GenericValue *addr);
	bool isHbBeforeFence(Event &e, std::vector<Event> &fcs);
	std::vector<int> addReadsToSCList(std::vector<Event> &scs, std::vector<Event> &fcs,
					  std::vector<int> &moAfter, std::vector<int> &moRfAfter,
					  std::vector<bool> &matrix, std::list<Event> &es);
	void addSCEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
		       llvm::GenericValue *addr, std::vector<bool> &matrix);
};

extern std::vector<std::vector<llvm::ExecutionContext> > initStacks;

extern bool shouldContinue;
extern bool executionCompleted;
extern bool globalAccess;
extern bool interpStore;
extern bool interpRMW;

/* TODO: Move this to Interpreter.h, and also remove the relevant header */
extern std::unordered_set<void *> globalVars;
extern std::unordered_set<std::string> uniqueExecs;

extern ExecutionGraph initGraph;
extern ExecutionGraph *currentEG;

#endif /* __EXECUTION_GRAPH_HPP__ */
