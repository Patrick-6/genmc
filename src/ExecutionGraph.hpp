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
	int currentT;
	unsigned int timestamp;

	/* Constructors */
	ExecutionGraph(llvm::Interpreter *EE);
	// ExecutionGraph(std::vector<Thread> ts, std::vector<int> es,
	// 	       std::vector<Event> re, int t)
	// 	: threads(ts), maxEvents(es), revisit(re), currentT(t) {};

	/* Basic getter methods */
	unsigned int nextStamp();
	EventLabel& getEventLabel(const Event &e);
	EventLabel& getPreviousLabel(const Event &e);
	EventLabel& getLastThreadLabel(int thread);
	Event getLastThreadEvent(int thread);
	Event getLastThreadRelease(int thread, llvm::GenericValue *addr);
	std::pair<std::vector<Event>, std::vector<Event> > getSCs();
	std::vector<llvm::GenericValue *> getDoubleLocs();
	View getGraphState(void);
	std::vector<Event> getPendingRMWs(EventLabel &sLab);
	Event getPendingLibRead(EventLabel &lab);
	std::vector<llvm::ExecutionContext> &getECStack(int thread);
	bool isThreadComplete(int thread);
	bool isWriteRfBefore(View &before, Event e);

	/* Basic setter methods */
	void addReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			    llvm::Type *typ, Event rf, EventAttr attr,
			    llvm::GenericValue &&cmpVal, llvm::GenericValue &&rmwVal,
			    llvm::AtomicRMWInst::BinOp op);
	void addLibReadToGraph(llvm::AtomicOrdering ord, llvm::GenericValue *ptr,
			       llvm::Type *typ, Event rf, std::string functionName);
	void addStoreToGraph(llvm::Type *typ, llvm::GenericValue *ptr, llvm::GenericValue &val,
			     int offsetMO, EventAttr attr, llvm::AtomicOrdering ord);
	void addLibStoreToGraph(llvm::Type *typ, llvm::GenericValue *ptr,
				llvm::GenericValue &val, int offsetMO,
				EventAttr attr, llvm::AtomicOrdering ord,
				std::string functionName, bool isInit);
	void addFenceToGraph(llvm::AtomicOrdering ord);
	void addTCreateToGraph(int cid);
	void addTJoinToGraph(int cid);
	void addStartToGraph(int tid, Event tc);
	void addFinishToGraph();

	/* Calculation of [(po U rf)*] predecessors and successors */
	View getMsgView(Event e);
	View getPorfBefore(Event e);
	View getHbBefore(Event e);
	View getHbPoBefore(Event e);
	View getHbBefore(const std::vector<Event> &es);
	View getHbRfBefore(std::vector<Event> &es);
	View getPorfBeforeNoRfs(const std::vector<Event> &es);
	std::vector<Event> getMoOptRfAfter(Event store);

	std::pair<std::vector<Event>, std::vector<bool> >
	calcWb(llvm::GenericValue *addr);
	std::pair<std::vector<Event>, std::vector<bool> >
	calcWbRestricted(llvm::GenericValue *addr, View &v);

	/* Calculation of particular sets of events/event labels */
	std::vector<EventLabel>	getPrefixLabelsNotBefore(View &prefix, View &before);
	std::vector<Event> getRfsNotBefore(const std::vector<EventLabel> &labs,
					   View &before);
	std::vector<std::pair<Event, Event> >
	getMOPredsInBefore(const std::vector<EventLabel> &labs,
			   View &before);
	std::vector<Event> getRMWChainDownTo(const EventLabel &sLab, const Event lower);
	std::vector<Event> getRMWChainUpTo(const EventLabel &sLab, const Event upper);
	std::vector<Event> getStoresHbAfterStores(llvm::GenericValue *loc,
						  const std::vector<Event> &chain);

	/* Calculation of writes a read can read from */
	std::pair<int, int> splitLocMOBefore(llvm::GenericValue *addr, View &before);

	/* Calculation of loads that can be revisited */
	std::vector<Event> findOverwrittenBoundary(llvm::GenericValue *addr, int thread);
	std::vector<Event> getRevisitable(const EventLabel &sLab);

	/* Graph modification methods */
	void changeRf(EventLabel &lab, Event store);
	View getViewFromStamp(unsigned int stamp);
	void cutToStamp(unsigned int stamp);
	void cutToView(View &view);
	void restoreStorePrefix(EventLabel &rLab, View &storePorfBefore,
				std::vector<EventLabel> &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);

	/* Equivalence checks */
	bool equivPrefixes(unsigned int stamp, const std::vector<EventLabel> &prefixA,
			   const std::vector<EventLabel> &prefixB);
	bool equivPlacings(unsigned int stamp,
			   const std::vector<std::pair<Event, Event> > &moPlacingsA,
			   const std::vector<std::pair<Event, Event> > &moPlacingsB);


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
	void clearAllStacks(void);

	/* Race detection methods */
	Event findRaceForNewLoad(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);
	Event findRaceForNewStore(llvm::AtomicOrdering ord, llvm::GenericValue *ptr);

	/* Library consistency checks */
	std::vector<Event> getLibEventsInView(Library &lib, View &v);
	std::vector<Event> getLibConsRfsInView(Library &lib, EventLabel &rLab,
					       const std::vector<Event> &stores, View &v);
	bool isLibConsistentInView(Library &lib, View &v);

	/* Debugging methods */
	void validateGraph(void);

	/* Outputting facilities */
	void printTraceBefore(Event e);
	void prettyPrintGraph();
	void dotPrintToFile(std::string &filename, View &before, Event e);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	void calcLoadPoRfView(EventLabel &lab, Event prev, Event &rf);
	void calcLoadHbView(EventLabel &lab, Event prev, Event &rf);
	void addEventToGraph(EventLabel &lab);
	void addReadToGraphCommon(EventLabel &lab, Event &rf);
	void addStoreToGraphCommon(EventLabel &lab);
	void calcPorfAfter(const Event &e, View &a);
	void calcHbRfBefore(Event &e, llvm::GenericValue *addr, View &a);
	void calcRelRfPoBefore(int thread, int index, View &v);
	void calcTraceBefore(const Event &e, View &a, std::stringstream &buf);
	std::vector<int> calcSCFencesSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCFencesPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	std::vector<int> calcSCPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event &e);
	bool isRMWLoad(Event &e);
	void spawnAllChildren(int thread);
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

	void getPoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getRfEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getHbEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getRfm1EdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			      std::vector<Event> &tos);
	void getWbEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void getMoEdgePairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
			    std::vector<Event> &tos);
	void calcSingleStepPairs(std::vector<std::pair<Event, std::vector<Event> > > &froms,
				 std::vector<Event> &tos,
				 llvm::StringMap<std::vector<bool> > &relMap,
				 std::vector<bool> &relMatrix, std::string &step);
	void addStepEdgeToMatrix(std::vector<Event> &es,
				 llvm::StringMap<std::vector<bool> > &relMap,
				 std::vector<bool> &relMatrix,
				 std::vector<std::string> &steps);
	llvm::StringMap<std::vector<bool> >
	calculateAllRelations(Library &lib, std::vector<Event> &es);
};

#endif /* __EXECUTION_GRAPH_HPP__ */
