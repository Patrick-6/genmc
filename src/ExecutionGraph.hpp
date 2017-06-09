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

#include "Error.hpp"
#include "Event.hpp"
#include "Thread.hpp"

#include <unordered_map>
#include <unordered_set>

/* Types and variables */
/* TODO: integrate this into EventLabel?? */
struct RevisitPair {
	Event e;
	Event rf;
	Event shouldRf;
	std::vector<int> preds;
	std::vector<Event> revisit;

	RevisitPair(Event e, Event rf, Event shouldRf, std::vector<int> preds,
		    std::vector<Event> revisit)
		: e(e), rf(rf), shouldRf(shouldRf), preds(preds), revisit(revisit) {};
};

/*
 * ExecutionGraph class - This class represents an execution graph
 */
class ExecutionGraph {
public:
	std::vector<Thread> threads;
	std::vector<int> maxEvents;
	std::vector<Event> revisit;
	std::unordered_map<llvm::GenericValue *, std::vector<Event> > modOrder;
	std::vector<RevisitPair> workqueue;
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
	std::vector<int> getGraphState(void);
	std::vector<llvm::ExecutionContext> &getThreadECStack(int thread);
	std::vector<Event> getRevisitLoads(Event store);

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

	/* Calculation of [(po U rf)*] predecessors and successors */
	std::vector<int> getPorfAfter(Event e);
	std::vector<int> getPorfAfter(const std::vector<Event > &es);
	std::vector<int> getPorfBefore(Event e);
	std::vector<int> getPorfBefore(const std::vector<Event> &es);
	std::vector<int> getPorfBeforeNoRfs(const std::vector<Event> &es);
	std::vector<int> getHbBefore(Event e);
	std::vector<int> getHbBefore(const std::vector<Event> &es);

	/* Graph modification methods */
	void cutBefore(std::vector<int> &preds, std::vector<Event> &rev);
	void cutToCopyAfter(ExecutionGraph &other, std::vector<int> &after);
	void modifyRfs(std::vector<Event> &es, Event store);
	void markReadsAsVisited(std::vector<Event> &K, std::vector<Event> K0, Event store);

	/* Consistency checks */
	bool isConsistent();

	/* Graph exploration methods */
	bool scheduleNext(void);

	/* Debugging methods */
	void validateGraph(void);

	/* Printing facilities */
	void printTraceBefore(Event e);

	/* Overloaded operators */
	friend std::ostream& operator<<(std::ostream &s, const ExecutionGraph &g);

protected:
	void addEventToGraph(EventLabel &lab);
	void addReadToGraphCommon(EventLabel &lab, Event &rf);
	void addStoreToGraphCommon(EventLabel &lab);
	void calcPorfAfter(const Event &e, std::vector<int> &a);
	void calcPorfBefore(const Event &e, std::vector<int> &a);
	void calcHbBefore(const Event &e, std::vector<int> &a);
	void calcTraceBefore(const Event &e, std::vector<int> &a, std::stringstream &buf);
};

extern std::vector<std::vector<llvm::ExecutionContext> > initStacks;

extern bool shouldContinue;
extern bool executionCompleted;
extern bool globalAccess;
extern bool interpStore;
extern bool interpRMW;

/* TODO: Move this to Interpreter.h, and also remove the relevant header */
extern std::unordered_set<llvm::GenericValue *> globalVars;
extern std::unordered_set<std::string> uniqueExecs;

extern ExecutionGraph initGraph;
extern ExecutionGraph *currentEG;

#endif /* __EXECUTION_GRAPH_HPP__ */
