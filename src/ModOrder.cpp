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

#include "Error.hpp"
#include "ModOrder.hpp"
#include <algorithm>


/************************************************************
 ** Class Constructors
 ***********************************************************/

ModOrder::ModOrder() {}


/************************************************************
 ** Iterators
 ***********************************************************/

ModOrder::iterator ModOrder::begin() { return mo_.begin(); }
ModOrder::iterator ModOrder::end()   { return mo_.end(); }

ModOrder::const_iterator ModOrder::cbegin() { return mo_.cbegin(); }
ModOrder::const_iterator ModOrder::cend()   { return mo_.cend(); }


/************************************************************
 ** Basic getter and setter methods
 ***********************************************************/

llvm::GenericValue *ModOrder::getAddrAtPos(ModOrder::iterator it)
{
	return it->first;
}

std::vector<Event> ModOrder::getMoAfter(llvm::GenericValue *addr, const Event &e)
{
	std::vector<Event> res;

	/* All stores are mo-after INIT */
	if (e.isInitializer())
		return mo_[addr];

	for (auto rit = mo_[addr].rbegin(); rit != mo_[addr].rend(); ++rit) {
		if (*rit == e) {
			std::reverse(res.begin(), res.end());
			return res;
		}
		res.push_back(*rit);
	}
	BUG();
}

void ModOrder::addAtLocEnd(llvm::GenericValue *addr, const Event &e)
{
       mo_[addr].push_back(e);
}

void ModOrder::addAtLocAfter(llvm::GenericValue *addr, const Event &pred, const Event &e)
{
	/* If there is no predecessor, put the store in the beginning */
	if (pred == Event::getInitializer()) {
		mo_[addr].insert(mo_[addr].begin(), e);
		return;
	}

	/* Otherwise, place it in the appropriate place */
	for (auto it = mo_[addr].begin(); it != mo_[addr].end(); ++it) {
		if (*it == pred) {
			mo_[addr].insert(it + 1, e);
			return;
		}
	}
	/* Pred has to be either INIT or in this location's MO */
	BUG();
}

bool ModOrder::locContains(llvm::GenericValue *addr, const Event &e)
{
	return e == Event::getInitializer() ||
	      std::any_of(mo_[addr].begin(), mo_[addr].end(), [&e](Event s){ return s == e; });
}

int ModOrder::getStoreOffset(llvm::GenericValue *addr, const Event &e)
{
	if (e == Event::getInitializer())
		return -1;

	for (auto it = mo_[addr].begin(); it != mo_[addr].end(); ++it) {
		if (*it == e)
			return std::distance(mo_[addr].begin(), it);
	}
	BUG();
}


/************************************************************
 ** Overloaded operators
 ***********************************************************/

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ModOrder &mo)
{
	s << "Modification Order:\n";
	for (auto &kvp : mo.mo_) {
		s << "\t" << kvp.first << ": [ ";
		for (auto &e : kvp.second)
			s << e << " ";
		s << "]\n";
	}
	return s;
}
