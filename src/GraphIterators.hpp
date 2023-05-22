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

#ifndef __GRAPH_ITERATORS_HPP__
#define __GRAPH_ITERATORS_HPP__

#include "config.h"
#include "ExecutionGraph.hpp"
#include "CoherenceCalculator.hpp"
#include <llvm/ADT/iterator_range.h>
#include <iterator>
#include <type_traits>
#include <utility>

/*
 * Helper iterators for ExecutionGraphs
 */

/*******************************************************************************
 **                         LabelIterator Class
 ******************************************************************************/

/*
 * This class implements some helper iterators for ExecutionGraph.
 * A bit ugly, but easily tunable, and deals with UP containers
 */
template<typename ThreadT, typename ThreadItT, typename LabelT, typename LabelItT>
class LabelIterator {

protected:
	ThreadT *threads;
	ThreadItT thread;
	LabelItT label;

public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = LabelT;
	using difference_type = signed;
	using pointer = LabelT *;
	using reference = LabelT &;

	using BaseT = LabelIterator<ThreadT, ThreadItT, LabelT, LabelItT>;

	/*** Constructor ***/
	LabelIterator() = default;

	/* begin()/end() constructor */
	template<typename G, typename U = ThreadItT,
		 std::enable_if_t<!std::is_base_of_v<BaseT, std::decay_t<G>>, bool> = true,
		 std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	LabelIterator(G &g) : threads(&g.getThreadList()), thread(g.begin()) {
		if (thread != threads->end()) {
			label = thread->begin();
			advanceThread();
		}
	}
	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	LabelIterator(G &g, bool) : threads(&g.getThreadList()), thread(g.end()) {}

	/* rbegin()/rend() constructor */
	template<typename G, typename U = ThreadItT,
		 std::enable_if_t<!std::is_base_of_v<BaseT, std::decay_t<G>>, bool> = true,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	LabelIterator(G &g) : threads(&g.getThreadList()), thread(g.rbegin()) {
		if (thread != threads->rend()) {
			label = thread->rbegin();
			advanceThread();
		}
	}
	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	LabelIterator(G &g, bool) : threads(&g.getThreadList()), thread(g.rend()) {}

	/* iterator-from-label constructor (normal iterator) */
	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	LabelIterator(G &g, pointer p)
		: threads(&g.getThreadList()), thread(g.begin() + p->getThread()),
		  label(thread->begin() + p->getIndex()) {}

	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	LabelIterator(G &g, Event e)
		: threads(&g.getThreadList()), thread(g.begin() + e.thread),
		  label(thread->begin() + e.index) { advanceThread(); }

	/* iterator-from-label constructor (reverse iterator) */
	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	LabelIterator(G &g, pointer p)
		: threads(&g.getThreadList()), thread(g.rbegin() + threads->size() - p->getThread()-1),
		  label(thread->rbegin() + g.getThreadSize(p->getThread()) - p->getIndex()-1) {}

	template<typename G, typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	LabelIterator(G &g, Event e)
		: threads(&g.getThreadList()), thread(g.rbegin() + g.getThreadList().size() - e.thread-1),
		  label(thread->rbegin() + g.getThreadSize(e.thread) - e.index-1) {
		advanceThread();
	}


	/*** Operators ***/
	inline pointer operator*() const { return &**label; }
	inline pointer operator->() const { return operator*(); }

	template<typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	inline bool operator==(const LabelIterator &other) const {
		return thread == other.thread &&
		       (thread == threads->end() || label == other.label);
	}

	template<typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	inline bool operator==(const LabelIterator &other) const {
		return thread == other.thread &&
		       (thread == threads->rend() || label == other.label);
	}

	inline bool operator!=(const LabelIterator& other) const {
		return !operator==(other);
	}

	LabelIterator& operator++() {
		++label;
		advanceThread();
		return *this;
	}
	inline LabelIterator operator++(int) {
		auto tmp = *this; ++*this; return tmp;
	}

	LabelIterator& operator--() {
		while (thread == threads->end() || label == thread->begin()) {
			--thread;
			label = thread->end();
		}
		--label;
		return *this;
	}
	inline LabelIterator operator--(int) {
		auto tmp = *this; --*this; return tmp;
	}

protected:
	/* Checks whether we have reached the end of a thread, and appropriately
	 * advances the thread and label iterators. Does nothing if that is not the case. */
	template<typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().begin())>::value> * = nullptr>
	inline void advanceThread() {
		while (label == thread->end()) {
			++thread;
			if (thread == threads->end())
				break;
			label = thread->begin();
		}
	}

	template<typename U = ThreadItT,
		 typename std::enable_if_t<std::is_same<U, decltype(
		std::declval<ThreadT>().rbegin())>::value> * = nullptr>
	inline void advanceThread() {
		while (label == thread->rend()) {
			++thread;
			if (thread == threads->rend())
				break;
			label = thread->rbegin();
		}
	}
};

using label_iterator = LabelIterator<ExecutionGraph::ThreadList,
				     ExecutionGraph::iterator,
				     EventLabel,
				     ExecutionGraph::Thread::iterator>;
using const_label_iterator = LabelIterator<const ExecutionGraph::ThreadList,
					   ExecutionGraph::const_iterator,
					   const EventLabel,
					   ExecutionGraph::Thread::const_iterator>;

using reverse_label_iterator = LabelIterator<ExecutionGraph::ThreadList,
					     ExecutionGraph::reverse_iterator,
					     EventLabel,
					     ExecutionGraph::Thread::reverse_iterator>;
using const_reverse_label_iterator = LabelIterator<const ExecutionGraph::ThreadList,
						   ExecutionGraph::const_reverse_iterator,
						   const EventLabel,
						   ExecutionGraph::Thread::const_reverse_iterator>;


/*******************************************************************************
 **                         EventIterator Class
 ******************************************************************************/

/*
 * Helper that overrides LabelIterator to iterate over events
 */
template<typename ThreadT, typename ThreadItT, typename LabelT, typename LabelItT>
class EventIterator : public LabelIterator<ThreadT, ThreadItT, LabelT, LabelItT> {

protected:
	using Base = LabelIterator<ThreadT, ThreadItT, LabelT, LabelItT>;

public:
	using value_type = Event;
	using pointer = Event *;
	using reference = Event &;

	EventIterator() = default;

	/* begin() constructor */
	template<typename G>
	EventIterator(G &g) : Base(g) {}

	template<typename G>
	EventIterator(G &g, bool) : Base(g, true) {}

	template<typename G>
	EventIterator(G &g, value_type e) : Base(g, e) {}

	/*** Operators ***/
	inline reference operator*() const { return (*this->label)->getPos(); }
	inline pointer operator->() const { return &operator*(); }

	EventIterator& operator++() {
		return static_cast<EventIterator&>(Base::operator++());
	}
	EventIterator operator++(int) {
		auto tmp = *this; Base::operator++(); return tmp;
	}

	EventIterator& operator--() {
		return static_cast<EventIterator&>(Base::operator--());
	}
	inline EventIterator operator--(int) {
		auto tmp = *this; Base::operator--(); return tmp;
	}
};

using event_iterator = EventIterator<ExecutionGraph::ThreadList,
				     ExecutionGraph::iterator,
				     EventLabel,
				     ExecutionGraph::Thread::iterator>;
using const_event_iterator = EventIterator<const ExecutionGraph::ThreadList,
					   ExecutionGraph::const_iterator,
					   const EventLabel,
					   ExecutionGraph::Thread::const_iterator>;

using reverse_event_iterator = EventIterator<ExecutionGraph::ThreadList,
					     ExecutionGraph::reverse_iterator,
					     EventLabel,
					     ExecutionGraph::Thread::reverse_iterator>;
using const_reverse_event_iterator = EventIterator<const ExecutionGraph::ThreadList,
						   ExecutionGraph::const_reverse_iterator,
						   const EventLabel,
						   ExecutionGraph::Thread::const_reverse_iterator>;


/*******************************************************************************
 **                         label-iteration utilities
 ******************************************************************************/

using label_range = llvm::iterator_range<label_iterator>;
using const_label_range = llvm::iterator_range<const_label_iterator>;

inline label_iterator label_begin(ExecutionGraph &G) { return label_iterator(G); }
inline const_label_iterator label_begin(const ExecutionGraph &G)
{
	return const_label_iterator(G);
}

inline label_iterator label_end(ExecutionGraph &G)   { return label_iterator(G, true); }
inline const_label_iterator label_end(const ExecutionGraph &G)
{
	return const_label_iterator(G, true);
}

inline label_range labels(ExecutionGraph &G) { return label_range(label_begin(G), label_end(G)); }
inline const_label_range labels(const ExecutionGraph &G) {
	return const_label_range(label_begin(G), label_end(G));
}


/*******************************************************************************
 **                         event-iteration utilities
 ******************************************************************************/

using event_range = llvm::iterator_range<event_iterator>;
using const_event_range = llvm::iterator_range<const_event_iterator>;

using reverse_event_range = llvm::iterator_range<reverse_event_iterator>;
using const_reverse_event_range = llvm::iterator_range<const_reverse_event_iterator>;

inline event_iterator event_begin(ExecutionGraph &G) { return event_iterator(G); }
inline const_event_iterator event_begin(const ExecutionGraph &G)
{
	return const_event_iterator(G);
}

inline event_iterator event_end(ExecutionGraph &G)   { return event_iterator(G, true); }
inline const_event_iterator event_end(const ExecutionGraph &G)
{
	return const_event_iterator(G, true);
}

inline event_range events(ExecutionGraph &G) { return event_range(event_begin(G), event_end(G)); }
inline const_event_range events(const ExecutionGraph &G) {
	return const_event_range(event_begin(G), event_end(G));
}

inline reverse_event_iterator event_rbegin(ExecutionGraph &G) { return reverse_event_iterator(G); }
inline const_reverse_event_iterator event_rbegin(const ExecutionGraph &G)
{
	return const_reverse_event_iterator(G);
}

inline reverse_event_iterator event_rend(ExecutionGraph &G) { return reverse_event_iterator(G, true); }
inline const_reverse_event_iterator event_rend(const ExecutionGraph &G)
{
	return const_reverse_event_iterator(G, true);
}

inline reverse_event_range revents(ExecutionGraph &G)
{
	return reverse_event_range(event_rbegin(G), event_rend(G));
}
inline const_reverse_event_range revents(const ExecutionGraph &G) {
	return const_reverse_event_range(event_rbegin(G), event_rend(G));
}


/*******************************************************************************
 **                         store-iteration utilities
 ******************************************************************************/

using const_store_iterator = CoherenceCalculator::const_store_iterator;
using const_reverse_store_iterator = CoherenceCalculator::const_reverse_store_iterator;

using const_store_range = llvm::iterator_range<const_store_iterator>;
using const_reverse_store_range = llvm::iterator_range<const_reverse_store_iterator>;

inline const_store_iterator store_begin(const ExecutionGraph &G, SAddr addr)
{
	return G.getCoherenceCalculator()->store_begin(addr);
}
inline const_store_iterator store_end(const ExecutionGraph &G, SAddr addr)
{
	return G.getCoherenceCalculator()->store_end(addr);
}

inline const_reverse_store_iterator store_rbegin(const ExecutionGraph &G, SAddr addr)
{
	return G.getCoherenceCalculator()->store_rbegin(addr);
}
inline const_reverse_store_iterator store_rend(const ExecutionGraph &G, SAddr addr)
{
	return G.getCoherenceCalculator()->store_rend(addr);
}

inline const_store_range stores(const ExecutionGraph &G, SAddr addr)
{
	return const_store_range(store_begin(G, addr), store_end(G, addr));
}
inline const_reverse_store_range rstores(const ExecutionGraph &G, SAddr addr)
{
	return const_reverse_store_range(store_rbegin(G, addr), store_rend(G, addr));
}


/*******************************************************************************
 **                         co-iteration utilities
 ******************************************************************************/

inline const_store_iterator co_succ_begin(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_succ_begin(addr, store);
}
inline const_store_iterator co_succ_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_succ_begin(e);
}
inline const_store_iterator co_succ_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_succ_begin(G, lab->getPos());
}

inline const_store_iterator co_succ_end(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_succ_end(addr, store);
}
inline const_store_iterator co_succ_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_succ_end(e);
}
inline const_store_iterator co_succ_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_succ_end(G, lab->getPos());
}


inline const_store_range co_succs(const ExecutionGraph &G, SAddr addr, Event store)
{
	return const_store_range(co_succ_begin(G, addr, store), co_succ_end(G, addr, store));
}
inline const_store_range co_succs(const ExecutionGraph &G, Event e)
{
	return const_store_range(co_succ_begin(G, e), co_succ_end(G, e));
}
inline const_store_range co_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_succs(G, lab->getPos());
}


inline const_store_iterator co_imm_succ_begin(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_imm_succ_begin(addr, store);
}
inline const_store_iterator co_imm_succ_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_imm_succ_begin(e);
}
inline const_store_iterator co_imm_succ_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_succ_begin(G, lab->getPos());
}

inline const_store_iterator co_imm_succ_end(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_imm_succ_end(addr, store);
}
inline const_store_iterator co_imm_succ_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_imm_succ_end(e);
}
inline const_store_iterator co_imm_succ_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_succ_end(G, lab->getPos());
}

inline const_store_range co_imm_succs(const ExecutionGraph &G, SAddr addr, Event store)
{
	return const_store_range(co_imm_succ_begin(G, addr, store), co_imm_succ_end(G, addr, store));
}
inline const_store_range co_imm_succs(const ExecutionGraph &G, Event e)
{
	return const_store_range(co_imm_succ_begin(G, e), co_imm_succ_end(G, e));
}
inline const_store_range co_imm_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_succs(G, lab->getPos());
}


inline const_reverse_store_iterator co_pred_begin(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_pred_begin(addr, store);
}
inline const_reverse_store_iterator co_pred_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_pred_begin(e);
}
inline const_reverse_store_iterator co_pred_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_pred_begin(G, lab->getPos());
}

inline const_reverse_store_iterator co_pred_end(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_pred_end(addr, store);
}
inline const_reverse_store_iterator co_pred_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_pred_end(e);
}
inline const_reverse_store_iterator co_pred_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_pred_end(G, lab->getPos());
}


inline const_reverse_store_range co_preds(const ExecutionGraph &G, SAddr addr, Event store)
{
	return const_reverse_store_range(co_pred_begin(G, addr, store), co_pred_end(G, addr, store));
}
inline const_reverse_store_range co_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_store_range(co_pred_begin(G, e), co_pred_end(G, e));
}
inline const_reverse_store_range co_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_preds(G, lab->getPos());
}


inline const_reverse_store_iterator co_imm_pred_begin(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_imm_pred_begin(addr, store);
}
inline const_reverse_store_iterator co_imm_pred_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_imm_pred_begin(e);
}
inline const_reverse_store_iterator co_imm_pred_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_pred_begin(G, lab->getPos());
}

inline const_reverse_store_iterator co_imm_pred_end(const ExecutionGraph &G, SAddr addr, Event store)
{
	return G.getCoherenceCalculator()->co_imm_pred_end(addr, store);
}
inline const_reverse_store_iterator co_imm_pred_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->co_imm_pred_end(e);
}
inline const_reverse_store_iterator co_imm_pred_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_pred_end(G, lab->getPos());
}

inline const_reverse_store_range co_imm_preds(const ExecutionGraph &G, SAddr addr, Event store)
{
	return const_reverse_store_range(co_imm_pred_begin(G, addr, store),
					 co_imm_pred_end(G, addr, store));
}
inline const_reverse_store_range co_imm_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_store_range(co_imm_pred_begin(G, e), co_imm_pred_end(G, e));
}
inline const_reverse_store_range co_imm_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return co_imm_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         fr-iteration utilities
 ******************************************************************************/

using const_fr_iterator = CoherenceCalculator::const_store_iterator;
using const_reverse_fr_iterator = CoherenceCalculator::const_reverse_store_iterator;

using const_fr_range = llvm::iterator_range<const_fr_iterator>;
using const_reverse_fr_range = llvm::iterator_range<const_reverse_fr_iterator>;


inline const_fr_iterator fr_succ_begin(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_succ_begin(addr, load);
}
inline const_fr_iterator fr_succ_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_succ_begin(e);
}
inline const_fr_iterator fr_succ_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_succ_begin(G, lab->getPos());
}

inline const_fr_iterator fr_succ_end(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_succ_end(addr, load);
}
inline const_fr_iterator fr_succ_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_succ_end(e);
}
inline const_fr_iterator fr_succ_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_succ_end(G, lab->getPos());
}


inline const_fr_range fr_succs(const ExecutionGraph &G, SAddr addr, Event load)
{
	return const_fr_range(fr_succ_begin(G, addr, load), fr_succ_end(G, addr, load));
}
inline const_fr_range fr_succs(const ExecutionGraph &G, Event e)
{
	return const_fr_range(fr_succ_begin(G, e), fr_succ_end(G, e));
}
inline const_fr_range fr_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_succs(G, lab->getPos());
}


inline const_fr_iterator fr_imm_succ_begin(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_imm_succ_begin(addr, load);
}
inline const_fr_iterator fr_imm_succ_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_imm_succ_begin(e);
}
inline const_fr_iterator fr_imm_succ_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_succ_begin(G, lab->getPos());
}

inline const_fr_iterator fr_imm_succ_end(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_imm_succ_end(addr, load);
}
inline const_fr_iterator fr_imm_succ_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_imm_succ_end(e);
}
inline const_fr_iterator fr_imm_succ_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_succ_end(G, lab->getPos());
}

inline const_fr_range fr_imm_succs(const ExecutionGraph &G, SAddr addr, Event load)
{
	return const_fr_range(fr_imm_succ_begin(G, addr, load), fr_imm_succ_end(G, addr, load));
}
inline const_fr_range fr_imm_succs(const ExecutionGraph &G, Event e)
{
	return const_fr_range(fr_imm_succ_begin(G, e), fr_imm_succ_end(G, e));
}
inline const_fr_range fr_imm_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_succs(G, lab->getPos());
}


inline const_fr_iterator fr_imm_pred_begin(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_imm_pred_begin(addr, load);
}
inline const_fr_iterator fr_imm_pred_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_imm_pred_begin(e);
}
inline const_fr_iterator fr_imm_pred_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_pred_begin(G, lab->getPos());
}

inline const_fr_iterator fr_imm_pred_end(const ExecutionGraph &G, SAddr addr, Event load)
{
	return G.getCoherenceCalculator()->fr_imm_pred_end(addr, load);
}
inline const_fr_iterator fr_imm_pred_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_imm_pred_end(e);
}
inline const_fr_iterator fr_imm_pred_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_pred_end(G, lab->getPos());
}

inline const_fr_range fr_imm_preds(const ExecutionGraph &G, SAddr addr, Event load)
{
	return const_fr_range(fr_imm_pred_begin(G, addr, load),
				      fr_imm_pred_end(G, addr, load));
}
inline const_fr_range fr_imm_preds(const ExecutionGraph &G, Event e)
{
	return const_fr_range(fr_imm_pred_begin(G, e), fr_imm_pred_end(G, e));
}
inline const_fr_range fr_imm_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_imm_preds(G, lab->getPos());
}


inline const_fr_iterator fr_init_pred_begin(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_init_pred_begin(e);
}
inline const_fr_iterator fr_init_pred_begin(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_init_pred_begin(G, lab->getPos());
}

inline const_fr_iterator fr_init_pred_end(const ExecutionGraph &G, Event e)
{
	return G.getCoherenceCalculator()->fr_init_pred_end(e);
}
inline const_fr_iterator fr_init_pred_end(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_init_pred_end(G, lab->getPos());
}

inline const_fr_range fr_init_preds(const ExecutionGraph &G, Event e)
{
	return const_fr_range(fr_init_pred_begin(G, e), fr_init_pred_end(G, e));
}
inline const_fr_range fr_init_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return fr_init_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         po-iteration utilities
 ******************************************************************************/

using const_po_iterator = const_event_iterator;
using const_reverse_po_iterator = const_reverse_event_iterator;

using const_po_range = llvm::iterator_range<const_po_iterator>;
using const_reverse_po_range = llvm::iterator_range<const_reverse_po_iterator>;

inline const_po_iterator po_succ_begin(const ExecutionGraph &G, Event e)
{
	return const_event_iterator(G, e.next());
}

inline const_po_iterator po_succ_end(const ExecutionGraph &G, Event e)
{
	return e == G.getLastThreadEvent(e.thread) ? po_succ_begin(G, e) :
		const_event_iterator(G, G.getLastThreadEvent(e.thread).next());
}

inline const_po_range po_succs(const ExecutionGraph &G, Event e)
{
	return const_po_range(po_succ_begin(G, e), po_succ_end(G, e));
}

inline const_po_range po_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return po_succs(G, lab->getPos());
}

inline const_po_iterator po_imm_succ_begin(const ExecutionGraph &G, Event e)
{
	return po_succ_begin(G, e);
}

inline const_po_iterator po_imm_succ_end(const ExecutionGraph &G, Event e)
{
	return e == G.getLastThreadEvent(e.thread) ? po_imm_succ_begin(G, e) :
		const_event_iterator(G, e.next().next());
}

inline const_po_range po_imm_succs(const ExecutionGraph &G, Event e)
{
	return const_po_range(po_imm_succ_begin(G, e), po_imm_succ_end(G, e));
}
inline const_po_range po_imm_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return po_imm_succs(G, lab->getPos());
}


inline const_reverse_po_iterator po_pred_begin(const ExecutionGraph &G, Event e)
{
	return const_reverse_event_iterator(G, e.prev());
}

inline const_reverse_po_iterator po_pred_end(const ExecutionGraph &G, Event e)
{
	return e == G.getFirstThreadEvent(e.thread) ? po_pred_begin(G, e) :
		const_reverse_event_iterator(G, G.getFirstThreadEvent(e.thread).prev());
}

inline const_reverse_po_range po_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_po_range(po_pred_begin(G, e), po_pred_end(G, e));
}

inline const_reverse_po_range po_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return po_preds(G, lab->getPos());
}

inline const_reverse_po_iterator po_imm_pred_begin(const ExecutionGraph &G, Event e)
{
	return po_pred_begin(G, e);
}

inline const_reverse_po_iterator po_imm_pred_end(const ExecutionGraph &G, Event e)
{
	return e == G.getFirstThreadEvent(e.thread) ? po_imm_pred_begin(G, e) :
		const_reverse_event_iterator(G, e.prev().prev());
}

inline const_reverse_po_range po_imm_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_po_range(po_imm_pred_begin(G, e), po_imm_pred_end(G, e));
}
inline const_reverse_po_range po_imm_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return po_imm_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         ppo-iteration utilities
 ******************************************************************************/

using const_ppo_iterator = DepInfo::const_iterator;
using const_ppo_range = llvm::iterator_range<const_ppo_iterator>;

#define PPO_ITERATOR(name)						\
inline const_ppo_iterator name##_pred_begin(const ExecutionGraph &G, Event e) \
{									\
	return G.getEventLabel(e)->name##_begin();			\
}									\
									\
inline const_ppo_iterator name##_pred_end(const ExecutionGraph &G, Event e) \
{									\
	return G.getEventLabel(e)->name##_end();			\
}									\
									\
inline const_ppo_range name##_preds(const ExecutionGraph &G, Event e)	\
{									\
	return G.getEventLabel(e)->name();				\
}									\
inline const_ppo_range name##_preds(const ExecutionGraph &G, const EventLabel *lab) \
{									\
	return lab->name();						\
}

PPO_ITERATOR(data);
PPO_ITERATOR(addr);
PPO_ITERATOR(ctrl);


/*******************************************************************************
 **                         poloc-iteration utilities
 ******************************************************************************/

namespace detail {
	struct LocationFilter {
		LocationFilter() = delete;
		LocationFilter(const ExecutionGraph &g, const SAddr &a)
			: graph(g), addr(a) {}

		bool operator()(const Event &s) const {
			auto *lab = llvm::dyn_cast<MemAccessLabel>(graph.getEventLabel(s));
			return lab && lab->getAddr() == addr;
		}
	private:
		const ExecutionGraph &graph;
		const SAddr addr;
	};

	template<typename IterT>
	struct poloc_filter_iterator : public llvm::filter_iterator<IterT, LocationFilter> {
	public:
		using BaseT = llvm::filter_iterator<IterT, LocationFilter>;

		poloc_filter_iterator(IterT it, IterT end, LocationFilter filter)
			: BaseT(it, end, filter) {}

		poloc_filter_iterator& operator++() {
			return static_cast<poloc_filter_iterator&>(BaseT::operator++());
		}
		poloc_filter_iterator operator++(int) {
			auto tmp = *this; BaseT::operator++(); return tmp;
		}
	};

	static inline bool hasLocation(const EventLabel *lab)
	{
		return llvm::isa<MemAccessLabel>(lab) || llvm::isa<CLFlushLabel>(lab);
	}

	static inline SAddr getLocation(const EventLabel *lab)
	{
		if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab))
			return mLab->getAddr();
		if (auto *fLab = llvm::dyn_cast<CLFlushLabel>(lab))
			return fLab->getAddr();
		return SAddr();
	}
} /* namespace detail */

using const_poloc_iterator = ::detail::poloc_filter_iterator<const_event_iterator>;
using const_reverse_poloc_iterator = ::detail::poloc_filter_iterator<const_reverse_event_iterator>;

using const_poloc_range = llvm::iterator_range<const_poloc_iterator>;
using const_reverse_poloc_range = llvm::iterator_range<const_reverse_poloc_iterator>;

inline const_poloc_iterator poloc_succ_begin(const ExecutionGraph &G, Event e)
{
	using namespace ::detail;
	auto *lab = G.getEventLabel(e);
	return hasLocation(lab) ? const_poloc_iterator(po_succ_begin(G, e), po_succ_end(G, e),
						       LocationFilter(G, getLocation(lab))) :
		const_poloc_iterator(po_succ_end(G, e), po_succ_end(G, e),
				     LocationFilter(G, SAddr()));
}

inline const_poloc_iterator poloc_succ_end(const ExecutionGraph &G, Event e)
{
	using namespace ::detail;
	auto *lab = G.getEventLabel(e);
	auto addr = hasLocation(lab) ? getLocation(lab) : SAddr();
	return const_poloc_iterator(po_succ_end(G, e), po_succ_end(G, e),
				    LocationFilter(G, addr));
}

inline const_poloc_range poloc_succs(const ExecutionGraph &G, Event e)
{
	return const_poloc_range(poloc_succ_begin(G, e), poloc_succ_end(G, e));
}

inline const_poloc_range poloc_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return poloc_succs(G, lab->getPos());
}

inline const_poloc_iterator poloc_imm_succ_begin(const ExecutionGraph &G, Event e)
{
	return poloc_succ_begin(G, e);
}

inline const_poloc_iterator poloc_imm_succ_end(const ExecutionGraph &G, Event e)
{
	return poloc_succ_begin(G, e) == poloc_succ_end(G, e) ? poloc_imm_succ_begin(G, e) :
		++poloc_imm_succ_begin(G, e);
}

inline const_poloc_range poloc_imm_succs(const ExecutionGraph &G, Event e)
{
	return const_poloc_range(poloc_imm_succ_begin(G, e), poloc_imm_succ_end(G, e));
}
inline const_poloc_range poloc_imm_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return poloc_imm_succs(G, lab->getPos());
}


inline const_reverse_poloc_iterator poloc_pred_begin(const ExecutionGraph &G, Event e)
{
	using namespace ::detail;
	auto *lab = G.getEventLabel(e);
	return hasLocation(lab) ? const_reverse_poloc_iterator(po_pred_begin(G, e), po_pred_end(G, e),
							       LocationFilter(G, getLocation(lab))) :
		const_reverse_poloc_iterator(po_pred_end(G, e), po_pred_end(G, e),
					     LocationFilter(G, SAddr()));
}

inline const_reverse_poloc_iterator poloc_pred_end(const ExecutionGraph &G, Event e)
{
	using namespace ::detail;
	auto *lab = G.getEventLabel(e);
	auto addr = hasLocation(lab) ? getLocation(lab) : SAddr();
	return const_reverse_poloc_iterator(po_pred_end(G, e), po_pred_end(G, e),
					    LocationFilter(G, addr));
}

inline const_reverse_poloc_range poloc_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_poloc_range(poloc_pred_begin(G, e), poloc_pred_end(G, e));
}

inline const_reverse_poloc_range poloc_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return poloc_preds(G, lab->getPos());
}

inline const_reverse_poloc_iterator poloc_imm_pred_begin(const ExecutionGraph &G, Event e)
{
	return poloc_pred_begin(G, e);
}

inline const_reverse_poloc_iterator poloc_imm_pred_end(const ExecutionGraph &G, Event e)
{
	return poloc_pred_begin(G, e) == poloc_pred_end(G, e) ? poloc_imm_pred_begin(G, e) :
		++poloc_imm_pred_begin(G, e);
}

inline const_reverse_poloc_range poloc_imm_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_poloc_range(poloc_imm_pred_begin(G, e), poloc_imm_pred_end(G, e));
}
inline const_reverse_poloc_range poloc_imm_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return poloc_imm_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         detour-iteration utilities
 ******************************************************************************/

namespace detail {
	/* Filters out an event only --- assumes poloc iteration */
	struct RfIntFilter {
		RfIntFilter() = delete;
		RfIntFilter(const ExecutionGraph &g, const Event &w)
			: graph(g), write(w) {}

		bool operator()(const Event &s) const {
			auto *lab = graph.getReadLabel(s);
			return lab && lab->getRf() != write;
		}
	private:
		const ExecutionGraph &graph;
		const Event write;
	};

	struct RfInvIntFilter {
		RfInvIntFilter() = delete;
		RfInvIntFilter(const ExecutionGraph &g, const Event &w)
			: graph(g), write(w) {}

		bool operator()(const Event &s) const {
			auto *lab = graph.getWriteLabel(s);
			return lab && lab->getPos() != write;
		}
	private:
		const ExecutionGraph &graph;
		const Event write;
	};

	template<typename IterT, typename FilterT>
	struct detour_filter_iterator : public llvm::filter_iterator<IterT, FilterT> {
	public:
		using BaseT = llvm::filter_iterator<IterT, FilterT>;

		detour_filter_iterator(IterT it, IterT end, FilterT filter)
			: BaseT(it, end, filter) {}

		detour_filter_iterator& operator++() {
			return static_cast<detour_filter_iterator&>(BaseT::operator++());
		}
		detour_filter_iterator operator++(int) {
			auto tmp = *this; BaseT::operator++(); return tmp;
		}
	};
} /* namespace detail */

using const_detour_iterator =
	::detail::detour_filter_iterator<const_poloc_iterator, ::detail::RfIntFilter>;
using const_detour_range = llvm::iterator_range<const_detour_iterator>;

using const_reverse_detour_iterator =
	::detail::detour_filter_iterator<const_reverse_poloc_iterator, ::detail::RfInvIntFilter>;
using const_reverse_detour_range = llvm::iterator_range<const_reverse_detour_iterator>;

inline const_detour_iterator detour_succ_begin(const ExecutionGraph &G, Event e)
{
	auto *lab = G.getWriteLabel(e);
	return lab ? const_detour_iterator(poloc_succ_begin(G, e), poloc_succ_end(G, e),
					   ::detail::RfIntFilter(G, e)) :
		const_detour_iterator(poloc_succ_end(G, e), poloc_succ_end(G, e),
				      ::detail::RfIntFilter(G, e));
}

inline const_detour_iterator detour_succ_end(const ExecutionGraph &G, Event e)
{
	auto *lab = G.getWriteLabel(e);
	return const_detour_iterator(poloc_succ_end(G, e), poloc_succ_end(G, e),
				     ::detail::RfIntFilter(G, e));
}

inline const_detour_range detour_succs(const ExecutionGraph &G, Event e)
{
	return const_detour_range(detour_succ_begin(G, e), detour_succ_end(G, e));
}

inline const_detour_range detour_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return detour_succs(G, lab->getPos());
}


inline const_reverse_detour_iterator detour_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *lab = G.getReadLabel(e);
	return lab ? const_reverse_detour_iterator(poloc_pred_begin(G, e), poloc_pred_end(G, e),
						   ::detail::RfInvIntFilter(G, lab->getRf())) :
		const_reverse_detour_iterator(poloc_pred_end(G, e), poloc_pred_end(G, e),
					      ::detail::RfInvIntFilter(G, Event::getInitializer()));
}

inline const_reverse_detour_iterator detour_pred_end(const ExecutionGraph &G, Event e)
{
	auto *lab = G.getReadLabel(e);
	auto pos = lab && !lab->getRf().isBottom() ? lab->getRf() : Event::getInitializer();
	return const_reverse_detour_iterator(poloc_pred_end(G, e), poloc_pred_end(G, e),
					    ::detail::RfInvIntFilter(G, pos));
}

inline const_reverse_detour_range detour_preds(const ExecutionGraph &G, Event e)
{
	return const_reverse_detour_range(detour_pred_begin(G, e), detour_pred_end(G, e));
}

inline const_reverse_detour_range detour_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return detour_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         rf-iteration utilities
 ******************************************************************************/

using const_rf_iterator = WriteLabel::const_rf_iterator;
using const_rf_range = llvm::iterator_range<const_rf_iterator>;

namespace detail {
	inline const_rf_iterator sentinel;
};

inline const_rf_iterator rf_succ_begin(const ExecutionGraph &G, Event e)
{
	auto *wLab = G.getWriteLabel(e);
	return wLab ? wLab->readers_begin() : ::detail::sentinel;
}

inline const_rf_iterator rf_succ_end(const ExecutionGraph &G, Event e)
{
	auto *wLab = G.getWriteLabel(e);
	return wLab ? wLab->readers_end() : ::detail::sentinel;
}


inline const_rf_range rf_succs(const ExecutionGraph &G, Event e)
{
	return const_rf_range(rf_succ_begin(G, e), rf_succ_end(G, e));
}
inline const_rf_range rf_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return rf_succs(G, lab->getPos());
}


using const_rf_inv_iterator = const_event_iterator;
using const_rf_inv_range = llvm::iterator_range<const_event_iterator>;

inline const_rf_inv_iterator rf_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *rLab = G.getReadLabel(e);
	return (!rLab || rLab->getRf().isBottom()) ? event_end(G) :
		const_event_iterator(G, rLab->getRf());
}

inline const_rf_inv_iterator rf_pred_end(const ExecutionGraph &G, Event e)
{
	auto *rLab = G.getReadLabel(e);
	return (!rLab || rLab->getRf().isBottom()) ? event_end(G) :
		const_event_iterator(G, rLab->getRf().next());
}

inline const_rf_inv_range rf_preds(const ExecutionGraph &G, Event e)
{
	return const_rf_inv_range(rf_pred_begin(G, e), rf_pred_end(G, e));
}


/*******************************************************************************
 **                         rfe-iteration utilities
 ******************************************************************************/

namespace detail {
	struct DiffThreadFilter {
		DiffThreadFilter() = delete;
		DiffThreadFilter(const ExecutionGraph &g, int t)
			: graph(g), thread(t) {}

		bool operator()(const Event &e) const {
			return !e.isInitializer() && !e.isBottom() && e.thread != thread;
		}
	private:
		const ExecutionGraph &graph;
		const int thread;
	};

	template<typename IterT>
	struct rfe_filter_iterator : public llvm::filter_iterator<IterT, DiffThreadFilter> {
	public:
		using BaseT = llvm::filter_iterator<IterT, DiffThreadFilter>;

		rfe_filter_iterator(IterT it, IterT end, DiffThreadFilter filter)
			: BaseT(it, end, filter) {}

		rfe_filter_iterator& operator++() {
			return static_cast<rfe_filter_iterator&>(BaseT::operator++());
		}
		rfe_filter_iterator operator++(int) {
			auto tmp = *this; BaseT::operator++(); return tmp;
		}
	};
} /* namespace detail */

using const_rfe_iterator = ::detail::rfe_filter_iterator<const_rf_iterator>;
using const_rfe_range = llvm::iterator_range<const_rfe_iterator>;

inline const_rfe_iterator rfe_succ_begin(const ExecutionGraph &G, Event e)
{
	return const_rfe_iterator(rf_succ_begin(G, e), rf_succ_end(G, e),
				  ::detail::DiffThreadFilter(G, e.thread));
}

inline const_rfe_iterator rfe_succ_end(const ExecutionGraph &G, Event e)
{
	return const_rfe_iterator(rf_succ_end(G, e), rf_succ_end(G, e),
				  ::detail::DiffThreadFilter(G, e.thread));
}


inline const_rfe_range rfe_succs(const ExecutionGraph &G, Event e)
{
	return const_rfe_range(rfe_succ_begin(G, e), rfe_succ_end(G, e));
}
inline const_rfe_range rfe_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return rfe_succs(G, lab->getPos());
}

using const_rfe_inv_iterator = const_rf_inv_iterator;
using const_rfe_inv_range = llvm::iterator_range<const_rfe_inv_iterator>;

inline const_rfe_inv_iterator rfe_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *rLab = G.getReadLabel(e);
	return (rLab && rLab->readsExt()) ? rf_pred_begin(G, e) : rf_pred_end(G, e);
}

inline const_rfe_inv_iterator rfe_pred_end(const ExecutionGraph &G, Event e)
{
	return rf_pred_end(G, e);
}

inline const_rfe_inv_range rfe_preds(const ExecutionGraph &G, Event e)
{
	return const_rfe_inv_range(rfe_pred_begin(G, e), rfe_pred_end(G, e));
}

inline const_rfe_inv_range rfe_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return const_rfe_inv_range(rfe_pred_begin(G, lab->getPos()), rfe_pred_end(G, lab->getPos()));
}


/*******************************************************************************
 **                         rfi-iteration utilities
 ******************************************************************************/

namespace detail {
	struct SameThreadFilter {
		SameThreadFilter() = delete;
		SameThreadFilter(const ExecutionGraph &g, int t)
			: graph(g), thread(t) {}

		bool operator()(const Event &e) const {
			return !e.isBottom() && (e.isInitializer() || e.thread == thread);
		}
	private:
		const ExecutionGraph &graph;
		const int thread;
	};

	template<typename IterT>
	struct rfi_filter_iterator : public llvm::filter_iterator<IterT, SameThreadFilter> {
	public:
		using BaseT = llvm::filter_iterator<IterT, SameThreadFilter>;

		rfi_filter_iterator(IterT it, IterT end, SameThreadFilter filter)
			: BaseT(it, end, filter) {}

		rfi_filter_iterator& operator++() {
			return static_cast<rfi_filter_iterator&>(BaseT::operator++());
		}
		rfi_filter_iterator operator++(int) {
			auto tmp = *this; BaseT::operator++(); return tmp;
		}
	};
} /* namespace detail */

using const_rfi_iterator = ::detail::rfi_filter_iterator<const_rf_iterator>;
using const_rfi_range = llvm::iterator_range<const_rfi_iterator>;

inline const_rfi_iterator rfi_succ_begin(const ExecutionGraph &G, Event e)
{
	return const_rfi_iterator(rf_succ_begin(G, e), rf_succ_end(G, e),
				  ::detail::SameThreadFilter(G, e.thread));
}

inline const_rfi_iterator rfi_succ_end(const ExecutionGraph &G, Event e)
{
	return const_rfi_iterator(rf_succ_end(G, e), rf_succ_end(G, e),
				  ::detail::SameThreadFilter(G, e.thread));
}


inline const_rfi_range rfi_succs(const ExecutionGraph &G, Event e)
{
	return const_rfi_range(rfi_succ_begin(G, e), rfi_succ_end(G, e));
}
inline const_rfi_range rfi_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return rfi_succs(G, lab->getPos());
}

using const_rfi_inv_iterator = const_rf_inv_iterator;
using const_rfi_inv_range = llvm::iterator_range<const_rfi_inv_iterator>;

inline const_rfi_inv_iterator rfi_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *rLab = G.getReadLabel(e);
	return (rLab && rLab->readsInt()) ? rf_pred_begin(G, e) : rf_pred_end(G, e);
}

inline const_rfi_inv_iterator rfi_pred_end(const ExecutionGraph &G, Event e)
{
	return rf_pred_end(G, e);
}

inline const_rfi_inv_range rfi_preds(const ExecutionGraph &G, Event e)
{
	return const_rfi_inv_range(rfi_pred_begin(G, e), rfi_pred_end(G, e));
}


/*******************************************************************************
 **                         tcreate-iteration utilities
 ******************************************************************************/

using const_tc_iterator = const_event_iterator;
using const_tc_range = llvm::iterator_range<const_tc_iterator>;

inline const_tc_iterator tc_succ_begin(const ExecutionGraph &G, Event e)
{
	auto *tcLab = llvm::dyn_cast<ThreadCreateLabel>(G.getEventLabel(e));
	return tcLab ? const_event_iterator(G, Event(tcLab->getChildId(), 0)) :
		event_end(G);
}

inline const_tc_iterator tc_succ_end(const ExecutionGraph &G, Event e)
{
	auto *tcLab = llvm::dyn_cast<ThreadCreateLabel>(G.getEventLabel(e));
	return tcLab ? const_event_iterator(G, Event(tcLab->getChildId(), 1)) :
		event_end(G);
}

inline const_tc_range tc_succs(const ExecutionGraph &G, Event e)
{
	return const_tc_range(tc_succ_begin(G, e), tc_succ_end(G, e));
}

inline const_tc_range tc_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return tc_succs(G, lab->getPos());
}


inline const_tc_iterator tc_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *tsLab = llvm::dyn_cast<ThreadStartLabel>(G.getEventLabel(e));
	return tsLab ? const_event_iterator(G, tsLab->getParentCreate()) :
		event_end(G);
}

inline const_tc_iterator tc_pred_end(const ExecutionGraph &G, Event e)
{
	auto *tsLab = llvm::dyn_cast<ThreadStartLabel>(G.getEventLabel(e));
	return tsLab ? const_event_iterator(G, tsLab->getParentCreate().next()) :
		event_end(G);
}

inline const_tc_range tc_preds(const ExecutionGraph &G, Event e)
{
	return const_tc_range(tc_pred_begin(G, e), tc_pred_end(G, e));
}
inline const_tc_range tc_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return tc_preds(G, lab->getPos());
}


/*******************************************************************************
 **                         tjoin-iteration utilities
 ******************************************************************************/

using const_tj_iterator = const_event_iterator;
using const_tj_range = llvm::iterator_range<const_tj_iterator>;

inline const_tj_iterator tj_succ_begin(const ExecutionGraph &G, Event e)
{
	auto *eLab = llvm::dyn_cast<ThreadFinishLabel>(G.getEventLabel(e));
	return (eLab && !eLab->getParentJoin().isInitializer()) ?
		const_tj_iterator(G, eLab->getParentJoin()) :
		event_end(G);
}

inline const_tj_iterator tj_succ_end(const ExecutionGraph &G, Event e)
{
	auto *eLab = llvm::dyn_cast<ThreadFinishLabel>(G.getEventLabel(e));
	return (eLab && !eLab->getParentJoin().isInitializer()) ?
		const_tj_iterator(G, eLab->getParentJoin().next()) :
		event_end(G);
}

inline const_tj_range tj_succs(const ExecutionGraph &G, Event e)
{
	return const_tj_range(tj_succ_begin(G, e), tj_succ_end(G, e));
}
inline const_tj_range tj_succs(const ExecutionGraph &G, const EventLabel *lab)
{
	return tj_succs(G, lab->getPos());
}


inline const_tj_iterator tj_pred_begin(const ExecutionGraph &G, Event e)
{
	auto *tjLab = llvm::dyn_cast<ThreadJoinLabel>(G.getEventLabel(e));
	return (tjLab && llvm::isa<ThreadFinishLabel>(G.getLastThreadLabel(tjLab->getChildId()))) ?
		const_tj_iterator(G, G.getLastThreadEvent(tjLab->getChildId())) :
		event_end(G);
}

inline const_tj_iterator tj_pred_end(const ExecutionGraph &G, Event e)
{
	auto *tjLab = llvm::dyn_cast<ThreadJoinLabel>(G.getEventLabel(e));
	return (tjLab && llvm::isa<ThreadFinishLabel>(G.getLastThreadLabel(tjLab->getChildId()))) ?
		const_tj_iterator(G, G.getLastThreadEvent(tjLab->getChildId()).next()) :
		event_end(G);
}

inline const_tj_range tj_preds(const ExecutionGraph &G, Event e)
{
	return const_tj_range(tj_pred_begin(G, e), tj_pred_end(G, e));
}
inline const_tj_range tj_preds(const ExecutionGraph &G, const EventLabel *lab)
{
	return tj_preds(G, lab->getPos());
}

#endif /* __GRAPH_ITERATORS_HPP__ */
