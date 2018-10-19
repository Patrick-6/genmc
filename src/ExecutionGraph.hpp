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
	EventLabel& getEventLabel(const Event &e);
	EventLabel& getPreviousLabel(const Event &e);
	EventLabel& getLastThreadLabel(int thread);
	Event getLastThreadEvent(int thread);
	Event getLastThreadRelease(int thread, llvm::GenericValue *addr);
	View getEventHbView(Event e);
	View getEventMsgView(Event e);
	std::pair<std::vector<Event>, std::vector<Event> > getSCs();
	std::vector<llvm::GenericValue *> getDoubleLocs();
	View getGraphState(void);
	std::vector<llvm::ExecutionContext> &getECStack(int thread);
	bool isThreadComplete(int thread);

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

	/* Calculation of particular sets of events/event labels */
	std::vector<EventLabel>	getPrefixLabelsNotBefore(const std::vector<int> &prefix,
							 View &before);
	std::vector<Event> getRfsNotBefore(const std::vector<EventLabel> &labs,
					   View &before);
	std::vector<std::pair<Event, Event> >
	getMOPredsInBefore(const std::vector<EventLabel> &labs,
			   View &before);

	/* Calculation of writes a read can read from */
	std::vector<Event> getStoresToLocWeakRA(llvm::GenericValue *addr);
	std::vector<Event> getStoresToLocMO(llvm::GenericValue *addr);
	std::vector<Event> getStoresToLocWB(llvm::GenericValue *addr);
	std::pair<std::vector<Event>, std::vector<Event> >
	splitLocMOBefore(const std::vector<Event> &locMO, View &before);

	/* Calculation of loads that can be revisited */
	std::vector<Event> getRevisitLoadsMO(const EventLabel &sLab);
	std::vector<Event> getRevisitLoadsWB(const EventLabel &sLab);
	std::vector<Event> getRevisitLoadsRMWWB(EventLabel &sLab);

	/* Graph modification methods */
	void changeRf(EventLabel &lab, Event store);
	void cutToEventView(Event &e, View &view);
	void restoreStorePrefix(EventLabel &rLab, std::vector<int> &storePorfBefore,
				std::vector<EventLabel> &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);
	void clearAllStacks(void);

	/* Consistency checks */
	bool isConsistent();
	bool isPscWeakAcyclicWB();
	bool isPscWbAcyclicWB();
	bool isPscAcyclicWB();
	bool isPscAcyclicMO();
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
	std::vector<Event> calcOptionalRfs(const std::vector<Event> &locMO, Event store);
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
	std::vector<Event> getRMWChain(EventLabel sLab);
	std::vector<Event> getStoresHbAfterStores(llvm::GenericValue *loc, const std::vector<Event> &chain);
	void addRbEdges(std::vector<Event> &scs, std::vector<Event> &fcs,
			std::vector<int> &moAfter, std::vector<int> &moRfAfter,
			std::vector<bool> &matrix, EventLabel &lab);
	void addMoRfEdges(std::vector<Event> &scs, std::vector<Event> &fcs,
			  std::vector<int> &moAfter, std::vector<int> &moRfAfter,
			  std::vector<bool> &matrix, EventLabel &lab);
	std::vector<int> getSCRfSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, EventLabel &lab);
	std::vector<int> getSCFenceRfSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, EventLabel &lab);
	void addInitEdges(std::vector<Event> &scs, std::vector<Event> &fcs, std::vector<bool> &matrix);
	void addSbHbEdges(std::vector<Event> &scs, std::vector<bool> &matrix);
	void addSCEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
		       std::vector<Event> &mo, std::vector<bool> &matrix);
	void addSCWbEcos(std::vector<Event> &scs, std::vector<Event> &fcs,
			 std::vector<Event> &stores, std::vector<bool> &wbMatrix,
			 std::vector<bool> &pscMatrix);
	std::pair<std::vector<Event>, std::vector<bool> >
	calcWb(llvm::GenericValue *addr);

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

#endif /* __EXECUTION_GRAPH_HPP__ */
