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

#include "DepInfo.hpp"
#include "Error.hpp"
#include "Event.hpp"
#include "EventLabel.hpp"
#include "Library.hpp"
#include "Matrix2D.hpp"
#include "ModOrder.hpp"
#include "VectorClock.hpp"
#include <llvm/ADT/StringMap.h>

#include <memory>

/*******************************************************************************
 **                           ExecutionGraph Class
 ******************************************************************************/

/*
 * An class representing plain execution graphs. This class offers
 * the basic infrastructure all types of graphs should provide (e.g.,
 * calculation of hb-predecessors, psc, etc). More specialized types
 * of graphs can provide extra functionality (e.g., take dependencies
 * into account when restricting a graph).
 */
class ExecutionGraph {

private:
	using Thread = std::vector<std::unique_ptr<EventLabel> >;
	using Graph = std::vector<Thread>;

public:
	/* Constructors */
	ExecutionGraph();

	/* Iterators */
	using iterator = Graph::iterator;
	using const_iterator = Graph::const_iterator;
	using reverse_iterator = Graph::reverse_iterator;
	using const_reverse_iterator = Graph::const_reverse_iterator;

	iterator begin() { return events.begin(); };
	iterator end() { return events.end(); };
	const_iterator begin() const { return events.begin(); };
	const_iterator end() const { return events.end(); };

	reverse_iterator rbegin() { return events.rbegin(); };
	reverse_iterator rend()   { return events.rend(); };
	const_reverse_iterator rbegin() const { return events.rbegin(); };
	const_reverse_iterator rend()   const { return events.rend(); };


	/* Modification order methods */
	const ModOrder& getModOrder() const { return modOrder; };
	const std::vector<Event>& getModOrder(const llvm::GenericValue *addr) {
		return modOrder[addr];
	};


	/* Thread-related methods */

	/* Creates a new thread in the execution graph */
	inline void addNewThread() { events.push_back({}); };

	/* Returns the number of threads currently in the graph */
	inline unsigned int getNumThreads() const { return events.size(); };

	/* Returns the size of the thread tid */
	inline unsigned int getThreadSize(int tid) const { return events[tid].size(); };

	/* Returns true if the thread tid is empty */
	inline bool isThreadEmpty(int tid) const { return getThreadSize(tid) == 0; };


	/* Event addition methods */

	/* Returns the next available stamp (and increases the counter) */
	unsigned int nextStamp();

	const ReadLabel *addReadToGraph(int tid, int index,
					llvm::AtomicOrdering ord,
					const llvm::GenericValue *ptr,
					const llvm::Type *typ, Event rf);

	const FaiReadLabel *addFaiReadToGraph(int tid, int index,
					      llvm::AtomicOrdering ord,
					      const llvm::GenericValue *ptr,
					      const llvm::Type *typ, Event rf,
					      llvm::AtomicRMWInst::BinOp op,
					      llvm::GenericValue &&opValue);

	const CasReadLabel *addCasReadToGraph(int tid, int index,
					      llvm::AtomicOrdering ord,
					      const llvm::GenericValue *ptr,
					      const llvm::Type *typ, Event rf,
					      const llvm::GenericValue &expected,
					      const llvm::GenericValue &swap,
					      bool isLock = false);

	const LibReadLabel *addLibReadToGraph(int tid, int index,
					      llvm::AtomicOrdering ord,
					      const llvm::GenericValue *ptr,
					      const llvm::Type *typ, Event rf,
					      std::string functionName);

	const WriteLabel *addStoreToGraph(int tid, int index,
					  llvm::AtomicOrdering ord,
					  const llvm::GenericValue *ptr,
					  const llvm::Type *typ,
					  const llvm::GenericValue &val,
					  int offsetMO, bool isUnlock = false);

	const FaiWriteLabel *addFaiStoreToGraph(int tid, int index,
						llvm::AtomicOrdering ord,
						const llvm::GenericValue *ptr,
						const llvm::Type *typ,
						const llvm::GenericValue &val,
						int offsetMO);

	const CasWriteLabel *addCasStoreToGraph(int tid, int index,
						llvm::AtomicOrdering ord,
						const llvm::GenericValue *ptr,
						const llvm::Type *typ,
						const llvm::GenericValue &val,
						int offsetMO, bool isLock = false);

	const LibWriteLabel *addLibStoreToGraph(int tid, int index,
						llvm::AtomicOrdering ord,
						const llvm::GenericValue *ptr,
						const llvm::Type *typ,
						llvm::GenericValue &val,
						int offsetMO,
						std::string functionName,
						bool isInit);

	const FenceLabel *addFenceToGraph(int tid, int index, llvm::AtomicOrdering ord);

	const MallocLabel *addMallocToGraph(int tid, int index, const void *addr,
					    unsigned int size, bool isLocal = false);

	const FreeLabel *addFreeToGraph(int tid, int index, const void *addr,
					unsigned int size);

	const ThreadCreateLabel *addTCreateToGraph(int tid, int index, int cid);

	const ThreadJoinLabel *addTJoinToGraph(int tid, int index, int cid);

	const ThreadStartLabel *addStartToGraph(int tid, int index, Event tc);

	const ThreadFinishLabel *addFinishToGraph(int tid, int index);


	/* Event getter methods */

	/* Returns the label in the position denoted by event e */
	const EventLabel *getEventLabel(Event e) const;

	/* Returns the label in the previous position of e.
	 * Does _not_ perform any out-of-bounds checks */
	const EventLabel *getPreviousLabel(Event e) const;
	const EventLabel *getPreviousLabel(const EventLabel *lab) const;

	/* Returns the previous non-empty label of e. Since all threads
	 * have an initializing event, it returns that as a base case */
	const EventLabel *getPreviousNonEmptyLabel(Event e) const;
	const EventLabel *getPreviousNonEmptyLabel(const EventLabel *lab) const;

	/* Returns the last label in the thread tid */
	const EventLabel *getLastThreadLabel(int tid) const;

	/* Returns the last event in the thread tid */
	Event getLastThreadEvent(int tid) const;

	/* Returns the last release before upperLimit in the latter's thread.
	 * If it's not a fence, then it has to be at location addr */
	Event getLastThreadReleaseAtLoc(Event upperLimit,
					const llvm::GenericValue *addr) const;

	/* Returns the last release before upperLimit in the latter's thread */
	Event getLastThreadRelease(Event upperLimit) const;

	/* Returns a list of acquire (R or F) in upperLimit's thread (before it) */
	std::vector<Event> getThreadAcquiresAndFences(const Event upperLimit) const;

	/* Returns pair with all SC accesses and all SC fences */
	std::pair<std::vector<Event>, std::vector<Event> > getSCs();

	/* Returns a list with all accesses that are accessed at least twice */
	std::vector<const llvm::GenericValue *> getDoubleLocs();

	/* Given an write label sLab that is part of an RMW, return all
	 * other RMWs that read from the same write. Of course, there must
	 * be _at most_ one other RMW reading from the same write (see [Rex] set) */
	std::vector<Event> getPendingRMWs(const WriteLabel *sLab);

	/* Similar to getPendingRMWs() but for libraries (w/ functional RF) */
	Event getPendingLibRead(const LibReadLabel *lab);

	/* Returns a list of stores that access location loc, not part chain,
	 * that are hb-after some store in chain */
	std::vector<Event> getStoresHbAfterStores(const llvm::GenericValue *loc,
						  const std::vector<Event> &chain);


	/* Calculation of [(po U rf)*] predecessors and successors */
	const DepView &getPPoRfBefore(Event e);
	const View &getPorfBefore(Event e);
	const View &getHbBefore(Event e);
	const View &getHbPoBefore(Event e);
	View getHbBefore(const std::vector<Event> &es);
	View getHbRfBefore(const std::vector<Event> &es);
	View getPorfBeforeNoRfs(const std::vector<Event> &es);
	std::vector<Event> getInitRfsAtLoc(const llvm::GenericValue *addr);
	std::vector<Event> getMoOptRfAfter(const WriteLabel *sLab);
	std::vector<Event> getMoInvOptRfAfter(const WriteLabel *sLab);


	/* WB calculations */

	/* Calculates WB */
	Matrix2D<Event> calcWb(const llvm::GenericValue *addr);

	/* Calculates WB restricted in v */
	Matrix2D<Event> calcWbRestricted(const llvm::GenericValue *addr,
					 const VectorClock &v);


	/* Boolean helper functions */

	/* Returns true if e is hb-before w, or any of the reads that read from w */
	bool isHbOptRfBefore(const Event e, const Event write);
	bool isHbOptRfBeforeInView(const Event e, const Event write,
				   const VectorClock &v);

	/* Returns true if e (or any of the reads that read from it) is in before */
	bool isWriteRfBefore(const View &before, Event e);

	/* Returns true if store is read a successful RMW in the location ptr */
	bool isStoreReadByExclusiveRead(Event store, const llvm::GenericValue *ptr);

	/* Returns true if store is read by a successful RMW that is either non
	 * revisitable, or in the view porfBefore */
	bool isStoreReadBySettledRMW(Event store, const llvm::GenericValue *ptr,
				     const VectorClock &porfBefore);


	/* Race detection methods */

	Event findRaceForNewLoad(const ReadLabel *rLab);
	Event findRaceForNewStore(const WriteLabel *wLab);


	/* Consistency checks */

	bool isConsistent();
	bool isPscWeakAcyclicWB();
	bool isPscWbAcyclicWB();
	bool isPscAcyclicWB();

	Matrix2D<Event> calcPscMO();
	bool isPscAcyclicMO();

	bool isWbAcyclic();


	/* Library consistency checks */

	std::vector<Event> getLibEventsInView(const Library &lib, const View &v);
	std::vector<Event> getLibConsRfsInView(const Library &lib, Event read,
					       const std::vector<Event> &stores,
					       const View &v);
	bool isLibConsistentInView(const Library &lib, const View &v);
	void addInvalidRfs(Event read, const std::vector<Event> &es);
	void addBottomToInvalidRfs(Event read);


	/* Debugging methods */

	void validate(void);


	/* Graph modification methods */

	void changeRf(Event read, Event store);
	void resetJoin(Event join);
	bool updateJoin(Event join, Event childLast);


	/* Revisit set methods */

	/* Returns true if the revisit set for rLab contains the pair
	 * <writePrefix, moPlacings>*/
	bool revisitSetContains(const ReadLabel *rLab, const std::vector<Event> &writePrefix,
				const std::vector<std::pair<Event, Event> > &moPlacings) const;

	/* Adds to the revisit set of rLab the pair <writePrefix, moPlacings> */
	void addToRevisitSet(const ReadLabel *rLab, const std::vector<Event> &writePrefix,
			     const std::vector<std::pair<Event, Event> > &moPlacings);

	/* Returns a list of loads that can be revisited */
	virtual std::vector<Event> getRevisitable(const WriteLabel *sLab);


	/* Prefix saving and restoring */

	virtual const VectorClock& getPrefixView(Event e);

	/* Saves the prefix of sLab that is not before rLab */
	virtual std::vector<std::unique_ptr<EventLabel> >
	getPrefixLabelsNotBefore(const WriteLabel *sLab, const ReadLabel *rLab);

	/* Returns a list of the rfs of the reads in labs */
	std::vector<Event> extractRfs(const std::vector<std::unique_ptr<EventLabel> > &labs);

	/* Returns pairs of the form <store, pred> where store is a write from labs,
	 * and pred is an mo-before store that is in before */
	std::vector<std::pair<Event, Event> >
	getMOPredsInBefore(const std::vector<std::unique_ptr<EventLabel> > &labs,
			   const VectorClock &before);

	/* Restores the prefix stored in storePrefix (for revisiting rLab) and
	 * also the moPlacings of the above prefix */
	void restoreStorePrefix(const ReadLabel *rLab,
				std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);

	/* Graph cutting */

	/* Returns a view of the graph representing events with stamp <= st */
	View getViewFromStamp(unsigned int st);

	/* Simmilar to getViewFromStamp() but returns a DepView */
	DepView getDepViewFromStamp(unsigned int st);

	/* Cuts a graph so that it only contains events with stamp <= st */
	virtual void cutToStamp(unsigned int st);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	void resizeThread(unsigned int tid, unsigned int size) {
		events[tid].resize(size);
	};

	void setEventLabel(Event e, std::unique_ptr<EventLabel> lab) {
		events[e.thread][e.index] = std::move(lab);
	};

	ModOrder& getModOrder() { return modOrder; };
	std::vector<unsigned int> calcRMWLimits(const Matrix2D<Event> &wb);
	View calcBasicHbView(const EventLabel *lab) const;
	View calcBasicPorfView(const EventLabel *lab) const;
	DepView calcBasicPPoRfView(const EventLabel *lab);
	const EventLabel *addEventToGraph(std::unique_ptr<EventLabel> lab);
	const ReadLabel *addReadToGraphCommon(std::unique_ptr<ReadLabel> lab);
	const WriteLabel *addStoreToGraphCommon(std::unique_ptr<WriteLabel> lab);
	void calcPorfAfter(const Event e, View &a);
	void calcHbRfBefore(Event e, const llvm::GenericValue *addr, View &a);
	void calcRelRfPoBefore(const Event last, View &v);
	std::vector<Event> calcSCFencesSuccs(const std::vector<Event> &fcs, const Event e);
	std::vector<Event> calcSCFencesPreds(const std::vector<Event> &fcs, const Event e);
	std::vector<Event> calcSCSuccs(const std::vector<Event> &fcs, const Event e);
	std::vector<Event> calcSCPreds(const std::vector<Event> &fcs, const Event e);
	std::vector<Event> getSCRfSuccs(const std::vector<Event> &fcs, const Event e);
	std::vector<Event> getSCFenceRfSuccs(const std::vector<Event> &fcs, const Event e);
	bool isRMWLoad(const Event e);
	bool isRMWLoad(const EventLabel *lab);
	void spawnAllChildren(int thread);
	void addRbEdges(std::vector<Event> &fcs, std::vector<Event> &moAfter,
			std::vector<Event> &moRfAfter, Matrix2D<Event> &matrix, const Event &e);
	void addMoRfEdges(std::vector<Event> &fcs, std::vector<Event> &moAfter,
			  std::vector<Event> &moRfAfter, Matrix2D<Event> &matrix, const Event &e);
	void addInitEdges(std::vector<Event> &fcs, Matrix2D<Event> &matrix);
	void addSbHbEdges(Matrix2D<Event> &matrix);
	void addSCEcos(std::vector<Event> &fcs, std::vector<Event> &mo, Matrix2D<Event> &matrix);
	void addSCWbEcos(std::vector<Event> &fcs, Matrix2D<Event> &wbMatrix, Matrix2D<Event> &pscMatrix);

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
				 const std::string &step, std::vector<Event> &tos);
	void addStepEdgeToMatrix(std::vector<Event> &es,
				 Matrix2D<Event> &relMatrix,
				 const std::vector<std::string> &substeps);
	llvm::StringMap<Matrix2D<Event> >
	calculateAllRelations(const Library &lib, std::vector<Event> &es);

private:
	Graph events;
	ModOrder modOrder;
	unsigned int timestamp;
};

#endif /* __EXECUTION_GRAPH_HPP__ */
