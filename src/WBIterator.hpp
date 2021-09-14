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

#ifndef __WB_ITERATOR_HPP__
#define __WB_ITERATOR_HPP__

#include "config.h"
#include "AdjList.hpp"
#include "Event.hpp"
#include <iterator>
#include <llvm/ADT/iterator_range.h>

/*******************************************************************************
 **                           WBIterator Class
 ******************************************************************************/

using WbT = AdjList<Event, EventHasher>;

/*
 * This class implements some convenient iterators for a location's WB relation.
 *
 * Note:
 *    - This class does not provide any increment operators, as this
 *      is delegated to its subclasses.
 *
 * (This could perhaps be generalized to handle AdjLists in general.)
 */
class WBIterator {

public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = Event;
	using difference_type = signed;
	using pointer = const Event *;
	using reference = const Event &;

protected:
	/*** Constructors/destructor ***/
	WBIterator() = delete;

	/* begin() constructor */
	WBIterator(const WbT &wb, value_type e) : wb(wb), elems(wb.getElems()), curr(elems.begin()), store(e) {}

	/* end() constructor -- dummy parameter */
	WBIterator(const WbT &wb, value_type e, bool) : wb(wb), elems(wb.getElems()), curr(elems.end()), store(e) {}

public:

	/*** Operators ***/
	inline reference operator*() const { return *curr; }
	inline pointer operator->() const { return &operator*(); }

	inline bool operator==(const WBIterator &other) const {
		return curr == other.curr && store == other.store;
	}
	inline bool operator!=(const WBIterator& other) const {
		return !operator==(other);
	}

protected:
	const WbT &wb;
	const std::vector<Event> &elems;
	WbT::const_iterator curr;
	value_type store;
};


/*
 * Iterator for the wb successors of a given event, where successors are
 * all stores that are reachable from the event.
 *
 * Note:
 *     - It does not iterate over the successors in any particular order
 */
class WBSuccIterator : public WBIterator {

public:
	WBSuccIterator() = delete;
	WBSuccIterator(const WbT &wb, value_type e) : WBIterator(wb, e) { advanceToSucc(); }
	WBSuccIterator(const WbT &wb, value_type e, bool) : WBIterator(wb, e, true) {}

	WBSuccIterator& operator++() {
		++curr;
		advanceToSucc();
		return *this;
	}
	inline WBSuccIterator operator++(int) {
		auto tmp = *this; ++*this; return tmp;
	}

private:
	bool isSuccessor() const {
		return *curr != store && (store.isInitializer() || wb(store, *curr));
	}

	void advanceToSucc() {
		while (curr != elems.end() && !isSuccessor())
			++curr;
	}
};


/*
 * Analogous to WBSuccIterator for predecessors.
 *
 * Note:
 *    - The initializer will _NOT_ be looped over.
 */
class WBPredIterator : public WBIterator {

public:
	WBPredIterator() = delete;
	WBPredIterator(const WbT &wb, value_type e) : WBIterator(wb, e) {
		if (store.isInitializer()) {
			curr = elems.end();
			return;
		}
		advanceToPred();
	}
	WBPredIterator(const WbT &wb, value_type e, bool) : WBIterator(wb, e, true) {}

	WBPredIterator& operator++() {
		++curr;
		advanceToPred();
		return *this;
	}
	inline WBPredIterator operator++(int) {
		auto tmp = *this; ++*this; return tmp;
	}

private:
	bool isPredecessor() const {
		return *curr != store && wb(*curr, store);
	}

	void advanceToPred() {
		while (curr != elems.end() && !isPredecessor())
			++curr;
	}
};

/* Successors --- only const versions */
using const_wb_succ_iterator = const WBSuccIterator;

using const_wb_succ_range = llvm::iterator_range<const_wb_succ_iterator>;

inline const_wb_succ_iterator wb_succ_begin(const WbT *wb, Event e)
{
	return const_wb_succ_iterator(*wb, e);
}
inline const_wb_succ_iterator wb_succ_begin(const WbT &wb, Event e)
{
	return const_wb_succ_iterator(wb, e);
}
inline const_wb_succ_iterator wb_succ_end(const WbT *wb, Event e)
{
	return const_wb_succ_iterator(*wb, e, true);
}
inline const_wb_succ_iterator wb_succ_end(const WbT &wb, Event e)
{
	return const_wb_succ_iterator(wb, e, true);
}

inline const_wb_succ_range wb_succs(const WbT *wb, Event e) {
	return const_wb_succ_range(wb_succ_begin(wb, e), wb_succ_end(wb, e));
}
inline const_wb_succ_range wb_succs(const WbT &wb, Event e) {
	return const_wb_succ_range(wb_succ_begin(wb, e), wb_succ_end(wb, e));
}

/* Predecessors --- only const versions */
using const_wb_pred_iterator = const WBPredIterator;

using const_wb_pred_range = llvm::iterator_range<const_wb_pred_iterator>;

inline const_wb_pred_iterator wb_pred_begin(const WbT *wb, Event e)
{
	return const_wb_pred_iterator(*wb, e);
}
inline const_wb_pred_iterator wb_pred_begin(const WbT &wb, Event e)
{
	return const_wb_pred_iterator(wb, e);
}
inline const_wb_pred_iterator wb_pred_end(const WbT *wb, Event e)
{
	return const_wb_pred_iterator(*wb, e, true);
}
inline const_wb_pred_iterator wb_pred_end(const WbT &wb, Event e)
{
	return const_wb_pred_iterator(wb, e, true);
}

inline const_wb_pred_range wb_preds(const WbT *wb, Event e) {
	return const_wb_pred_range(wb_pred_begin(wb, e), wb_pred_end(wb, e));
}
inline const_wb_pred_range wb_preds(const WbT &wb, Event e) {
	return const_wb_pred_range(wb_pred_begin(wb, e), wb_pred_end(wb, e));
}

#endif /* __WB_ITERATOR_HPP__ */
