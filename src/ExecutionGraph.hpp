/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef __EXECUTION_GRAPH_HPP__
#define __EXECUTION_GRAPH_HPP__

#include "Error.hpp"
#include "Event.hpp"
#include "EventLabel.hpp"
#include "Library.hpp"
#include "ModOrder.hpp"
#include <llvm/ADT/StringMap.h>

/*
 * ExecutionGraph class - This class represents an execution graph
 */
class ExecutionGraph {
public:
	std::vector<std::vector<EventLabel> > events;
	ModOrder modOrder;
	unsigned int timestamp;

	/* Constructors */
	ExecutionGraph();

	/* Basic getter methods */
	unsigned int nextStamp();
	EventLabel& getEventLabel(Event e);
	EventLabel& getPreviousLabel(Event e);
	EventLabel& getLastThreadLabel(int thread);
	Event getLastThreadEvent(int thread) const;
	Event getLastThreadRelease(int thread, const llvm::GenericValue *addr) const;
	std::pair<std::vector<Event>, std::vector<Event> > getSCs();
	std::vector<const llvm::GenericValue *> getDoubleLocs();
	std::vector<Event> getPendingRMWs(const EventLabel &sLab);
	Event getPendingLibRead(const EventLabel &lab);
	bool isWriteRfBefore(const View &before, Event e);
	bool isStoreReadByExclusiveRead(Event store, const llvm::GenericValue *ptr);
	bool isStoreReadBySettledRMW(Event store, const llvm::GenericValue *ptr, const View &porfBefore);

	/* Basic setter methods */
	EventLabel& addReadToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord, const llvm::GenericValue *ptr,
				   llvm::Type *typ, Event rf, llvm::GenericValue &&cmpVal,
				   llvm::GenericValue &&rmwVal, llvm::AtomicRMWInst::BinOp op);
	EventLabel& addLibReadToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord, const llvm::GenericValue *ptr,
				      llvm::Type *typ, Event rf, std::string functionName);
	EventLabel& addStoreToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord, const llvm::GenericValue *ptr,
				    llvm::Type *typ, llvm::GenericValue &val, int offsetMO);
	EventLabel& addLibStoreToGraph(int tid, EventAttr attr, llvm::AtomicOrdering ord, const llvm::GenericValue *ptr,
				       llvm::Type *typ, llvm::GenericValue &val, int offsetMO,
				       std::string functionName, bool isInit);
	EventLabel& addFenceToGraph(int tid, llvm::AtomicOrdering ord);
	EventLabel& addMallocToGraph(int tid, const llvm::GenericValue *addr, const llvm::GenericValue &val);
	EventLabel& addFreeToGraph(int tid, const llvm::GenericValue *addr, const llvm::GenericValue &val);
	EventLabel& addTCreateToGraph(int tid, int cid);
	EventLabel& addTJoinToGraph(int tid, int cid);
	EventLabel& addStartToGraph(int tid, Event tc);
	EventLabel& addFinishToGraph(int tid);

	/* Calculation of [(po U rf)*] predecessors and successors */
	View getMsgView(Event e);
	View getPorfBefore(Event e);
	View getHbBefore(Event e);
	View getHbPoBefore(Event e);
	View getHbBefore(const std::vector<Event> &es);
	View getHbRfBefore(const std::vector<Event> &es);
	View getPorfBeforeNoRfs(const std::vector<Event> &es);
	std::vector<Event> getMoOptRfAfter(Event store);

	std::pair<std::vector<Event>, std::vector<bool> >
	calcWb(const llvm::GenericValue *addr);
	std::pair<std::vector<Event>, std::vector<bool> >
	calcWbRestricted(const llvm::GenericValue *addr, const View &v);

	/* Calculation of particular sets of events/event labels */
	Event getRMWChainUpperLimit(const EventLabel &sLab, const Event upper);
	Event getRMWChainLowerLimit(const EventLabel &sLab, const Event lower);
	Event getRMWChainLowerLimitInView(const EventLabel &sLab, const Event lower, const View &v);
	std::vector<Event> getRMWChain(const EventLabel &sLab);
	std::vector<Event> getStoresHbAfterStores(const llvm::GenericValue *loc,
						  const std::vector<Event> &chain);
	std::vector<EventLabel>	getPrefixLabelsNotBefore(const View &prefix, const View &before);
	std::vector<Event> getRfsNotBefore(const std::vector<EventLabel> &labs,
					   const View &before);
	std::vector<std::pair<Event, Event> >
	getMOPredsInBefore(const std::vector<EventLabel> &labs,
			   const View &before);

	/* Calculation of writes a read can read from */
	std::pair<int, int> splitLocMOBefore(const llvm::GenericValue *addr, const View &before);

	/* Calculation of loads that can be revisited */
	std::vector<Event> findOverwrittenBoundary(const llvm::GenericValue *addr, int thread);
	std::vector<Event> getRevisitable(const EventLabel &sLab);

	/* Graph modification methods */
	void changeRf(EventLabel &lab, Event store);
	View getViewFromStamp(unsigned int stamp);
	void cutToStamp(unsigned int stamp);
	void cutToView(const View &view);
	void restoreStorePrefix(EventLabel &rLab, std::vector<EventLabel> &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);

	/* Consistency checks */
	bool isConsistent();
	bool isPscWeakAcyclicWB();
	bool isPscWbAcyclicWB();
	bool isPscAcyclicWB();
	bool isPscAcyclicMO();
	bool isWbAcyclic();

	/* Race detection methods */
	Event findRaceForNewLoad(Event e);
	Event findRaceForNewStore(Event e);

	/* Library consistency checks */
	std::vector<Event> getLibEventsInView(Library &lib, const View &v);
	std::vector<Event> getLibConsRfsInView(Library &lib, EventLabel &rLab,
					       const std::vector<Event> &stores, const View &v);
	bool isLibConsistentInView(Library &lib, const View &v);

	/* Debugging methods */
	void validate(void);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	void calcLoadPoRfView(EventLabel &lab);
	void calcLoadHbView(EventLabel &lab);
	EventLabel& addEventToGraph(EventLabel &lab);
	EventLabel& addReadToGraphCommon(EventLabel &lab, Event rf);
	EventLabel& addStoreToGraphCommon(EventLabel &lab);
	void calcPorfAfter(const Event e, View &a);
	void calcHbRfBefore(Event e, const llvm::GenericValue *addr, View &a);
	void calcRelRfPoBefore(int thread, int index, View &v);
	std::vector<int> calcSCFencesSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event e);
	std::vector<int> calcSCFencesPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event e);
	std::vector<int> calcSCSuccs(std::vector<Event> &scs, std::vector<Event> &fcs, Event e);
	std::vector<int> calcSCPreds(std::vector<Event> &scs, std::vector<Event> &fcs, Event e);
	bool isRMWLoad(Event e);
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
