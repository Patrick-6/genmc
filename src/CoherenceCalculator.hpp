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

#ifndef __COHERENCE_CALCULATOR_HPP__
#define __COHERENCE_CALCULATOR_HPP__

#include "Calculator.hpp"
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>

class BackwardRevisit;
class ExecutionGraph;

/*******************************************************************************
 **                      CoherenceCalculator Class (Abstract)
 ******************************************************************************/

/*
 * An abstract class for modeling the different ways we can track coherence
 * (e.g, with the usage of modification orders, or by calculating WB, etc).
 * Defines the minimal interface such implementations should offer.
 */
class CoherenceCalculator : public Calculator {

public:
	using StoreList = std::vector<Event>;
	using LocMap = std::unordered_map<SAddr, StoreList>;

	using iterator = LocMap::iterator;
	using const_iterator = LocMap::const_iterator;

	using store_iterator = StoreList::iterator;
	using const_store_iterator = StoreList::const_iterator;

	using reverse_store_iterator = StoreList::reverse_iterator;
	using const_reverse_store_iterator = StoreList::const_reverse_iterator;

public:

	/* Constructor */
	CoherenceCalculator(ExecutionGraph &m, bool ooo)
		: Calculator(m), outOfOrder(ooo), sentinel() {}

	/* Returns whether the model we are operating under supports
	 * out-of-order execution */
	bool supportsOutOfOrder() const { return outOfOrder; };

	~CoherenceCalculator() = default;

	iterator begin() { return stores.begin(); }
	const_iterator begin() const { return stores.begin(); };
	iterator end() { return stores.end(); }
	const_iterator end() const { return stores.end(); }

	store_iterator store_begin(SAddr addr) { return stores[addr].begin(); }
	const_store_iterator store_begin(SAddr addr) const { return stores.at(addr).begin(); };
	store_iterator store_end(SAddr addr) { return stores[addr].end(); }
	const_store_iterator store_end(SAddr addr) const { return stores.at(addr).end(); }

	reverse_store_iterator store_rbegin(SAddr addr) { return stores[addr].rbegin(); }
	const_reverse_store_iterator store_rbegin(SAddr addr) const { return stores.at(addr).rbegin(); };
	reverse_store_iterator store_rend(SAddr addr) { return stores[addr].rend(); }
	const_reverse_store_iterator store_rend(SAddr addr) const { return stores.at(addr).rend(); }

	iterator init_rf_begin() { return initRfs.begin(); }
	const_iterator init_rf_begin() const { return initRfs.begin(); }
	iterator init_rf_end() { return initRfs.end(); }
	const_iterator init_rf_end() const { return initRfs.end(); }

	store_iterator init_rf_begin(SAddr addr) { return initRfs[addr].begin(); }
	const_store_iterator init_rf_begin(SAddr addr) const { return initRfs.at(addr).begin(); };
	store_iterator init_rf_end(SAddr addr) { return initRfs[addr].end(); }
	const_store_iterator init_rf_end(SAddr addr) const { return initRfs.at(addr).end(); }

	reverse_store_iterator init_rf_rbegin(SAddr addr) { return initRfs[addr].rbegin(); }
	const_reverse_store_iterator init_rf_rbegin(SAddr addr) const { return initRfs.at(addr).rbegin(); };
	reverse_store_iterator init_rf_rend(SAddr addr) { return initRfs[addr].rend(); }
	const_reverse_store_iterator init_rf_rend(SAddr addr) const { return initRfs.at(addr).rend(); }

	/*** co-iterators for known writes and locations ***/

	const_store_iterator
	co_succ_begin(SAddr addr, Event store) const;
	const_store_iterator
	co_succ_end(SAddr addr, Event store) const;

	const_store_iterator
	co_imm_succ_begin(SAddr addr, Event store) const;
	const_store_iterator
	co_imm_succ_end(SAddr addr, Event store) const;

	const_reverse_store_iterator
	co_pred_begin(SAddr addr, Event store) const;
	const_reverse_store_iterator
	co_pred_end(SAddr addr, Event store) const;

	const_reverse_store_iterator
	co_imm_pred_begin(SAddr addr, Event store) const;
	const_reverse_store_iterator
	co_imm_pred_end(SAddr addr, Event store) const;

	/*** co-iterators that work for all kinds of events ***/

	const_store_iterator co_succ_begin(Event e) const;
	const_store_iterator co_succ_end(Event e) const;

	const_store_iterator co_succ_begin(const EventLabel *lab) const {
		return co_succ_begin(lab->getPos());
	}
	const_store_iterator co_succ_end(const EventLabel *lab) const {
		return co_succ_end(lab->getPos());
	}

	const_store_iterator co_imm_succ_begin(Event e) const;
	const_store_iterator co_imm_succ_end(Event e) const;

	const_store_iterator co_imm_succ_begin(const EventLabel *lab) const {
		return co_imm_succ_begin(lab->getPos());
	}
	const_store_iterator co_imm_succ_end(const EventLabel *lab) const {
		return co_imm_succ_end(lab->getPos());
	}

	const_reverse_store_iterator
	co_pred_begin(Event e) const;
	const_reverse_store_iterator
	co_pred_end(Event e) const;

	const_reverse_store_iterator co_pred_begin(const EventLabel *lab) const {
		return co_pred_begin(lab->getPos());
	}
	const_reverse_store_iterator co_pred_end(const EventLabel *lab) const {
		return co_pred_end(lab->getPos());
	}

	const_reverse_store_iterator
	co_imm_pred_begin(Event e) const;
	const_reverse_store_iterator
	co_imm_pred_end(Event e) const;

	const_reverse_store_iterator co_imm_pred_begin(const EventLabel *lab) const {
		return co_imm_pred_begin(lab->getPos());
	}
	const_reverse_store_iterator co_imm_pred_end(const EventLabel *lab) const {
		return co_imm_pred_end(lab->getPos());
	}

	/*** fr-iterators for known reads and locations ***/

	const_store_iterator fr_succ_begin(SAddr addr, Event load) const;
	const_store_iterator fr_succ_end(SAddr addr, Event load) const;

	const_store_iterator fr_imm_succ_begin(SAddr addr, Event laod) const;
	const_store_iterator fr_imm_succ_end(SAddr addr, Event load) const;

	const_store_iterator
	fr_imm_pred_begin(SAddr addr, Event load) const;
	const_store_iterator
	fr_imm_pred_end(SAddr addr, Event load) const;

	/*** fr-iterators that work for all kinds of events ***/

	const_store_iterator fr_succ_begin(Event e) const;
	const_store_iterator fr_succ_end(Event e) const;

	const_store_iterator fr_succ_begin(const EventLabel *lab) const {
		return fr_succ_begin(lab->getPos());
	}
	const_store_iterator fr_succ_end(const EventLabel *lab) const {
		return fr_succ_end(lab->getPos());
	}

	const_store_iterator fr_imm_succ_begin(Event e) const;
	const_store_iterator fr_imm_succ_end(Event e) const;

	const_store_iterator fr_imm_succ_begin(const EventLabel *lab) const {
		return fr_imm_succ_begin(lab->getPos());
	}
	const_store_iterator fr_imm_succ_end(const EventLabel *lab) const {
		return fr_imm_succ_end(lab->getPos());
	}

	const_store_iterator
	fr_imm_pred_begin(Event e) const;
	const_store_iterator
	fr_imm_pred_end(Event e) const;

	const_store_iterator fr_imm_pred_begin(const EventLabel *lab) const {
		return fr_imm_pred_begin(lab->getPos());
	}
	const_store_iterator fr_imm_pred_end(const EventLabel *lab) const {
		return fr_imm_pred_end(lab->getPos());
	}

	const_store_iterator
	fr_init_pred_begin(Event e) const;
	const_store_iterator
	fr_init_pred_end(Event e) const;


	/* Whether a location is tracked (i.e., we are aware of it) */
	bool tracksLoc(SAddr addr) const { return stores.count(addr); }

	/* Whether a location has no stores */
	bool isLocEmpty(SAddr addr) const { return store_begin(addr) == store_end(addr); }

	/* Whether a location has more than one store */
	bool hasMoreThanOneStore(SAddr addr) const {
		return !isLocEmpty(addr) && ++store_begin(addr) != store_end(addr);
	}

	/* Track coherence at location addr */
	void
	trackCoherenceAtLoc(SAddr addr);

	/* Tracks that READ is reading INIT in ADDR */
	void
	addInitRfToLoc(SAddr addr, Event read);

	/* Shoud be called when READ is no longer reading INIT in ADDR*/
	void
	removeInitRfToLoc(SAddr addr, Event read);

	/* Adds STORE to ADDR at the offset specified by OFFSET.
	 * (Use -1 to insert it maximally.) */
	void
	addStoreToLoc(SAddr addr, Event store, int offset);

	/* Adds STORE to ATTR and ensures it will be co-after PRED.
	 * (Use INIT to insert it minimally.) */
	void
	addStoreToLocAfter(SAddr addr, Event store, Event pred);

	/* Returns whether STORE is maximal in LOC */
	bool
	isCoMaximal(SAddr addr, Event store);

	/* Returns whether STORE is maximal in LOC.
	 * Pre: Cached information for this location exist. */
	bool
	isCachedCoMaximal(SAddr addr, Event store);


#ifdef ENABLE_GENMC_DEBUG
	/* Saves the coherence status for all write labels in prefix.
	 * This means that for each write we save a predecessor in preds (or within
	 * the prefix itself), which will be present when the prefix is restored. */
	std::vector<std::pair<Event, Event> >
	saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &prefix,
			    const ReadLabel *rLab) const;
#endif

	const StoreList &getStoresToLoc(SAddr addr) const { return stores.at(addr); }

	/* Returns the offset for a particular store */
	int getStoreOffset(SAddr addr, Event e) const;

	/* Calculator overrides */

	/* Changes the offset of "store" to "newOffset" */
	void changeStoreOffset(SAddr addr, Event store, int newOffset);

	void initCalc() override;

	Calculator::CalculationResult doCalc() override;

	/* Stops tracking all stores not included in "preds" in the graph */
	void removeAfter(const VectorClock &preds) override;

	std::unique_ptr<Calculator> clone(ExecutionGraph &g) const override {
		return LLVM_MAKE_UNIQUE<CoherenceCalculator>(g, outOfOrder);
	}

protected:
	const StoreList &getInitRfsToLoc(SAddr addr) const { return initRfs.at(addr); }
	StoreList &getInitRfsToLoc(SAddr addr) { return initRfs[addr]; }

	const const_store_iterator &getSentinel() const { return sentinel; }
	const const_reverse_store_iterator &getRevSentinel() const {
		return rsentinel;
	}
	/* Returns true if the location "loc" contains the event "e" */
	bool locContains(SAddr loc, Event e) const;

	/* Caches MO-idx hints for all stores in ADDR for faster reverse lookup.
	 * If START != -1, it only does so starting from the START-th store */
	void cacheMOIdxHints(SAddr addr, int start = -1) const;

	/* Whether the model which we are operating under supports out-of-order
	 * execution. This enables some extra optimizations, in certain cases. */
	bool outOfOrder;

	/* Maps loc -> store list */
	LocMap stores;

	/* Maps loc -> init-rf list */
	LocMap initRfs;

	/* Dummy sentinel iterators */
	const_store_iterator sentinel;
	const_reverse_store_iterator rsentinel;
};

#endif /* __COHERENCE_CALCULATOR_HPP__ */
