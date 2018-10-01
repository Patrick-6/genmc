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
#include "Library.hpp"
#include "Interpreter.h"
#include "ModOrder.hpp"
//#include "RevisitSet.hpp"
#include "Thread.hpp"

/*
 * ExecutionGraph class - This class represents an execution graph
 */
class ExecutionGraph {
public:
	std::vector<Thread> threads;
	View maxEvents;
//	RevisitSet revisit;
	ModOrder modOrder;
	llvm::Interpreter *EE;
	std::vector<void *> stackAllocas;
	std::vector<void *> heapAllocas;
	std::vector<void *> freedMem;
	int currentT;

	/* Constructors */
	ExecutionGraph(llvm::Interpreter *EE);
	// ExecutionGraph(std::vector<Thread> ts, std::vector<int> es,
	// 	       std::vector<Event> re, int t)
	// 	: threads(ts), maxEvents(es), revisit(re), currentT(t) {};

	/* Basic getter methods */
	EventLabel& getEventLabel(Event &e);
	EventLabel& getPreviousLabel(Event &e);
	EventLabel& getLastThreadLabel(int thread);
	Event getLastThreadEvent(int thread);
	Event getLastThreadRelease(int thread, llvm::GenericValue *addr);
	View getEventHbView(Event e);
	View getEventMsgView(Event e);
	std::vector<llvm::GenericValue *> getDoubleLocs();
	std::vector<int> getGraphState(void);
	std::vector<llvm::ExecutionContext> &getECStack(int thread);
	bool isThreadComplete(int thread);
	std::vector<EventLabel> getPrefixLabelsNotBefore(Event &e, std::vector<int> &before);
	std::vector<Event> getRevisitLoads(Event &store);
	std::vector<Event> getRevisitLoadsRMW(Event &store);

	/* Basic setter methods */
	void addReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			    llvm::Type *typ, Event rf);
	void addGReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			     llvm::Type *typ, Event rf, std::string functionName);
	void addCASReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			       llvm::GenericValue &val, llvm::GenericValue &nextVal,
			       llvm::Type *typ, Event rf);
	void addRMWReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			       llvm::GenericValue &nextVal, llvm::AtomicRMWInst::BinOp op,
			       llvm::Type *typ, Event rf);
	void addStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			     llvm::GenericValue &val, llvm::Type *typ);
	void addGStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			      llvm::GenericValue &val, llvm::Type *typ,
			      std::string functionName);
	void addCASStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				llvm::GenericValue &val, llvm::Type *typ);
	void addRMWStoreToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
				llvm::GenericValue &val, llvm::Type *typ);
	void addFenceToGraph(llvm::AtomicOrdering ord);
	void addTCreateToGraph(int cid);
	void addTJoinToGraph(int cid);
	void addStartToGraph(int tid, Event tc);
	void addFinishToGraph();

	/* Calculation of [(po U rf)*] predecessors and successors */
	std::vector<int> getPorfAfter(Event e);
	std::vector<int> getPorfAfter(const std::vector<Event > &es);
	std::vector<int> getPorfBefore(Event e);
	std::vector<int> getPorfBefore(const std::vector<Event> &es);
	std::vector<int> getPorfBeforeNoRfs(const std::vector<Event> &es);
	View getHbBefore(Event e);
	View getHbBefore(const std::vector<Event> &es);
	View getHbPoBefore(Event e);
	std::vector<int> getHbRfBefore(std::vector<Event> &es);

	/* Calculation of writes a read can read from */
	std::vector<Event> getStoresToLoc(llvm::GenericValue *addr, ModelType model);
	std::vector<std::vector<Event> > splitLocMOBefore(View &before, std::vector<Event> &stores);

	/* Graph modification methods */
	void changeRf(EventLabel &lab, Event store);
	void cutToLoad(Event &load);
	void restoreStorePrefix(EventLabel &rLab, std::vector<int> &storePorfBefore,
				std::vector<EventLabel> &storePrefix);
	void clearAllStacks(void);

	/* Consistency checks */
	bool isConsistent();
	bool isPscAcyclic();
	bool isWbAcyclic();

	/* Scheduling methods */
	bool scheduleNext(void);
	void tryToBacktrack(void);

	/* Race detection methods */
	Event findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);
	Event findRaceForNewStore(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);

	/* Library consistency checks */
	std::vector<Event> getLibraryEvents(Library &lib);
	std::vector<Event> filterLibConstraints(Library &lib, Event &load,
						const std::vector<Event> &stores);

	/* Debugging methods */
	void validateGraph(void);

	/* Outputting facilities */
	void printTraceBefore(Event e);
	void prettyPrintGraph();
	void dotPrintToFile(std::string &filename, std::vector<int> &before, Event e);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	void calcLoadHbView(EventLabel &lab, Event prev, Event &rf);
	void addEventToGraph(EventLabel &lab);
	void addReadToGraphCommon(EventLabel &lab, Event &rf);
	void addStoreToGraphCommon(EventLabel &lab);
	void calcPorfAfter(const Event &e, std::vector<int> &a);
	void calcPorfBefore(const Event &e, std::vector<int> &a);
	void calcHbRfBefore(Event &e, llvm::GenericValue *addr, std::vector<int> &a);
	void calcRelRfPoBefore(int thread, int index, View &v);
	void calcTraceBefore(const Event &e, std::vector<int> &a, std::stringstream &buf);
//	std::vector<Event> calcOptionalRfs(Event store, std::vector<Event> &locMO);
	std::vector<int> calcSCFencesSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCFencesPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	bool isWriteRfBefore(View &before, Event e);
	bool isRMWLoad(Event &e);
	void spawnAllChildren(int thread);
	std::vector<Event> findOverwrittenBoundary(llvm::GenericValue *addr, int thread);
	std::vector<Event> getStoresWeakRA(llvm::GenericValue *addr);
	std::vector<Event> getStoresMO(llvm::GenericValue *addr);
	std::vector<Event> getStoresWB(llvm::GenericValue *addr);
	std::vector<Event> getRMWChain(Event &store);
	std::vector<Event> getStoresHbAfterStores(llvm::GenericValue *loc, std::vector<Event> &chain);
	std::pair<std::vector<int>, std::vector<int> >
	addReadsToSCList(std::vector<Event> &scs, std::vector<Event> &fcs,
			 std::vector<int> &moAfter, std::vector<int> &moRfAfter,
			 std::vector<bool> &matrix, std::list<Event> &es);
	void addSCEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
		       llvm::GenericValue *addr, std::vector<bool> &matrix);
	std::vector<bool> calcWb(std::vector<Event> &stores);

	void getPoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getRfEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getHbEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getRfm1EdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			      std::vector<Event> &tos);
	void getMoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void calcSingleStepPairs(Library &lib,
				 std::vector<std::pair<Event, std::vector<Event> > > &froms,
				 std::vector<Event> &tos,
				 llvm::StringMap<std::vector<bool> > &relMap,
				 std::vector<bool> &relMatrix, std::string &step);
	void addStepEdgeToMatrix(Library &lib, std::vector<Event> &es,
				 llvm::StringMap<std::vector<bool> > &relMap,
				 std::vector<bool> &relMatrix,
				 std::vector<std::string> &steps);
	llvm::StringMap<std::vector<bool> >
	calculateAllRelations(Library &lib, std::vector<Event> &es);
};

extern bool interpStore;
extern bool interpRMW;

#endif /* __EXECUTION_GRAPH_HPP__ */
