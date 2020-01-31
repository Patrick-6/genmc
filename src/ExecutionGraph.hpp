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

#include "DriverGraphEnumAPI.hpp"
#include "DepInfo.hpp"
#include "Error.hpp"
#include "Event.hpp"
#include "EventLabel.hpp"
#include "Library.hpp"
#include "Matrix2D.hpp"
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
	/* Should be used for the contruction of execution graphs */
	class Builder;

protected:
	/* Constructor should only be called from the builder */
	friend class GraphBuilder;
	friend class GraphManager;
	ExecutionGraph();

public:
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


	/* Event getter methods */

	/* Returns the label in the position denoted by event e */
	const EventLabel *getEventLabel(Event e) const;
	EventLabel *getEventLabel(Event e) {
		return events[e.thread][e.index].get();
	};

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

	/* LAPOR: Returns the last lock that is not matched before "upperLimit".
	 * If no such event exists, returns INIT */
	Event getLastThreadUnmatchedLockLAPOR(const Event upperLimit) const;

	/* LAPOR: Returns the last lock at location "loc" before "upperLimit".
	 * If no such event exists, returns INIT */
	Event getLastThreadLockAtLocLAPOR(const Event upperLimit,
					  const llvm::GenericValue *loc) const;

	/* LAPOR: Returns a linear extension of LB */
	std::vector<Event> getLbOrderingLAPOR() const;

	/* Returns pair with all SC accesses and all SC fences */
	std::pair<std::vector<Event>, std::vector<Event> > getSCs() const;

	/* Given an write label sLab that is part of an RMW, return all
	 * other RMWs that read from the same write. Of course, there must
	 * be _at most_ one other RMW reading from the same write (see [Rex] set) */
	std::vector<Event> getPendingRMWs(const WriteLabel *sLab) const;

	/* Similar to getPendingRMWs() but for libraries (w/ functional RF) */
	Event getPendingLibRead(const LibReadLabel *lab) const;

	virtual std::unique_ptr<VectorClock> getRevisitView(const ReadLabel *rLab,
							    const WriteLabel *wLab) const;

	/* Returns a list of all events satisfying property F */
	template <typename F>
	std::vector<Event> collectAllEvents(F cond) const {
		std::vector<Event> result;

		for (auto i = 0u; i < getNumThreads(); i++)
			for (auto j = 0u; j < getThreadSize(i); j++)
				if (cond(getEventLabel(Event(i, j))))
					result.push_back(Event(i, j));
		return result;
	}


	/* Calculation of [(po U rf)*] predecessors and successors */
	const DepView &getPPoRfBefore(Event e) const;
	const View &getPorfBefore(Event e) const;
	const View &getHbBefore(Event e) const;
	const View &getHbPoBefore(Event e) const;
	View getHbBefore(const std::vector<Event> &es) const;
	View getPorfBeforeNoRfs(const std::vector<Event> &es) const;
	std::vector<Event> getInitRfsAtLoc(const llvm::GenericValue *addr) const;

	/* Matrix filling for external relation calculation */
	void populatePorfEntries(Matrix2D<Event> &relation) const;
	void populatePPoRfEntries(Matrix2D<Event> &relation) const;
	void populateHbEntries(Matrix2D<Event> &relation) const;


	/* Boolean helper functions */

	/* Return true if its argument is the load part of a successful RMW */
	bool isRMWLoad(const Event e) const;
	bool isRMWLoad(const EventLabel *lab) const;

	/* Returns true if e is hb-before w, or any of the reads that read from w */
	bool isHbOptRfBefore(const Event e, const Event write) const;
	bool isHbOptRfBeforeInView(const Event e, const Event write,
				   const VectorClock &v) const;

	/* Returns true if e is hb-before w, or any of the reads that read from w
	 * in the relation "rel".
	 * Pre: all examined events need to be a part of rel */
	template <typename F = bool (*)(Event)>
	bool isHbOptRfBeforeRel(const Matrix2D<Event> &rel, Event a, Event b,
				F prop = [](Event e){ return true; }) const;

	/* Returns true if a (or any of the reads reading from a) is hb-before b */
	bool isWriteRfBefore(Event a, Event b) const;

	/* Returns true if a (or any of the reads reading from a) is before b in
	 * the relation "rel".
	 * Pre: all examined events need to be a part of rel */
	template <typename F = bool (*)(Event)>
	bool isWriteRfBeforeRel(const Matrix2D<Event> &rel, Event a, Event b,
				F prop = [](Event e){ return true; }) const;

	/* Returns true if store is read a successful RMW in the location ptr */
	bool isStoreReadByExclusiveRead(Event store, const llvm::GenericValue *ptr) const;

	/* Returns true if store is read by a successful RMW that is either non
	 * revisitable, or in the view porfBefore */
	bool isStoreReadBySettledRMW(Event store, const llvm::GenericValue *ptr,
				     const VectorClock &porfBefore) const;

	/* Returns true if the graph that will be created when sLab revisits rLab
	 * will be the same as the current one */
	virtual bool revisitModifiesGraph(const ReadLabel *rLab,
					  const EventLabel *sLab) const;


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
	void changeStoreOffset(const llvm::GenericValue *addr, Event s,
			       int newOffset);
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
	virtual std::vector<Event> getRevisitable(const WriteLabel *sLab) const;


	/* Prefix saving and restoring */

	/* Returns a vector clock representing the prefix of e.
	 * Depending on whether dependencies are tracked, the prefix can be
	 * either (po U rf) or (AR U rf) */
	virtual const VectorClock& getPrefixView(Event e) const;

	/* Returns a vector clock representing the events added before e */
	virtual std::unique_ptr<VectorClock> getPredsView(Event e) const;

	/* Saves the prefix of sLab that is not before rLab.
	 * (Because of functional libraries, sLab can be a ReadLabel as well.) */
	virtual std::vector<std::unique_ptr<EventLabel> >
	getPrefixLabelsNotBefore(const EventLabel *sLab, const ReadLabel *rLab) const;

	/* Returns a list of the rfs of the reads in labs */
	std::vector<Event>
	extractRfs(const std::vector<std::unique_ptr<EventLabel> > &labs) const;

	/* Returns pairs of the form <store, pred> where store is a write from labs,
	 * and pred is an mo-before store that was added before rLab */
	std::vector<std::pair<Event, Event> >
	saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &prefix,
			    const ReadLabel *rLab) const;

	/* Restores the prefix stored in storePrefix (for revisiting rLab) and
	 * also the moPlacings of the above prefix */
	void restoreStorePrefix(const ReadLabel *rLab,
				std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);

	/* Graph cutting */

	/* Returns a view of the graph representing events with stamp <= st */
	View getViewFromStamp(unsigned int st) const;

	/* Simmilar to getViewFromStamp() but returns a DepView */
	DepView getDepViewFromStamp(unsigned int st) const;

	/* Cuts a graph so that it only contains events with stamp <= st */
	virtual void cutToStamp(unsigned int st);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ExecutionGraph &g);

protected:
	/* Event addition methods should be called from the managing objects,
	 * so that the relation managing objects are also informed */
	const ReadLabel *addReadLabelToGraph(std::unique_ptr<ReadLabel> lab,
					     Event rf);
	const EventLabel *addOtherLabelToGraph(std::unique_ptr<EventLabel> lab);

	void resizeThread(unsigned int tid, unsigned int size) {
		events[tid].resize(size);
	};

	void setEventLabel(Event e, std::unique_ptr<EventLabel> lab) {
		events[e.thread][e.index] = std::move(lab);
	};

	void calcPorfAfter(const Event e, View &a);
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
	/* A collection of threads and the events for each threads */
	Graph events;

	/* The next available timestamp */
	unsigned int timestamp;
};

template <typename F>
bool ExecutionGraph::isHbOptRfBeforeRel(const Matrix2D<Event> &rel, Event a, Event b,
					F prop /* = [](Event e){ return true; } */) const
{
	if (rel(a, b))
		return true;

	const EventLabel *lab = getEventLabel(b);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (auto &r : wLab->getReadersList()) {
		if (prop(r) && rel(a, r))
			return true;
	}
	return false;
}

template <typename F>
bool ExecutionGraph::isWriteRfBeforeRel(const Matrix2D<Event> &rel, Event a, Event b,
					F prop /* = [&](Event e){ return true; } */) const
{
	if (rel(a, b))
		return true;

	const EventLabel *lab = getEventLabel(a);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (auto &r : wLab->getReadersList())
		if (prop(r) && rel(r, b))
			return true;
	return false;
}

#endif /* __EXECUTION_GRAPH_HPP__ */
