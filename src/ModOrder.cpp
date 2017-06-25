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

std::vector<Event> ModOrder::getAtLoc(llvm::GenericValue *addr)
{
	return mo_[addr];
}

void ModOrder::addAtLoc(llvm::GenericValue *addr, Event e)
{
	mo_[addr].push_back(e);
}

void ModOrder::setLoc(llvm::GenericValue *addr, std::vector<Event> &locMO)
{
	mo_[addr] = locMO;
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
