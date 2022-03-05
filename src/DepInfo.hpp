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
n * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#ifndef __DEP_INFO_HPP__
#define __DEP_INFO_HPP__

#include "Event.hpp"
#include "VSet.hpp"
#include <llvm/Support/raw_ostream.h>

/*******************************************************************************
 **                               Deps Class
 ******************************************************************************/

/*
 * A class to model the dependencies (of some kind) of an event. Each Deps
 * objects holds a collection of events on which some events depend on. In
 * principle, such an object should be used for each dependency kind of
 * a particular event.
 */
class Deps {

protected:
	using Set = VSet<Event>;

public:
	/* Constructors */
	Deps() : set_() {}
	Deps(Event e) : set_({ e }) {}

	template<typename ITER>
	Deps(ITER begin, ITER end) : set_(begin, end) {}

	/* Iterators */
	using const_iterator = typename Set::const_iterator;
	using const_reverse_iterator = typename Set::const_reverse_iterator;

	const_iterator begin() const { return set_.begin(); };
	const_iterator end() const { return set_.end(); };

	/* Updates this object based on the dependencies of dep (union) */
	void update(const Deps& dep) { set_.insert(dep.set_); }

	/* Clears all the stored dependencies */
	void clear() { set_.clear(); }

	/* Returns true if e is contained in the dependencies */
	bool contains(Event e) const { return set_.count(e); }

	/* Returns true if there are no dependencies */
	bool empty() const { return set_.empty(); }

	/* Printing */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const Deps &dep);

private:
	Set set_;
};


/*******************************************************************************
 **                             DepInfo Class
 ******************************************************************************/

/*
 * Packs together some information for the dependencies of a given event in
 * the form of ranges. Assuming that Deps objects are stored elsewhere, this
 * class can be used to communicate the dependencies a given event has.
 *
 * Dependencies have one of the following types:
 *     addr, data, ctrl, addr;po, cas.
 * Models are free to ignore some of these if they are of no use.
 */

struct DepInfo {

protected:
	static Deps::const_iterator sentinel;

public:
	using dep_range = llvm::iterator_range<Deps::const_iterator>;

	DepInfo() : addr(sentinel, sentinel), data(sentinel, sentinel),
		 ctrl(sentinel, sentinel), addrPo(sentinel, sentinel),
		 cas(sentinel, sentinel) {}

	dep_range addr;
	dep_range data;
	dep_range ctrl;
	dep_range addrPo;
	dep_range cas;
};


/*******************************************************************************
 **                             EventDeps Class
 ******************************************************************************/

/*
 * Packs together all ctrl,data, andaddr dependencies of a given event.
 */
struct EventDeps {

	Deps addr;
	Deps data;
	Deps ctrl;
	Deps addrPo;
	Deps cas;
};

#endif /* __DEP_INFO_HPP__ */
