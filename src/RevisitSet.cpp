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

#include "RevisitSet.hpp"
#include <algorithm>


/************************************************************
 ** Class Constructors
 ***********************************************************/

RevisitSet::RevisitSet() : rev_({}) {}


/************************************************************
 ** Iterators
 ***********************************************************/

RevisitSet::iterator RevisitSet::begin() { return rev_.begin(); }
RevisitSet::iterator RevisitSet::end()   { return rev_.end(); }

RevisitSet::const_iterator RevisitSet::cbegin() { return rev_.cbegin(); }
RevisitSet::const_iterator RevisitSet::cend()   { return rev_.cend(); }


/************************************************************
 ** Basic getter/setter methods and existential checks
 ***********************************************************/

void RevisitSet::add(std::vector<Event> &es)
{
	rev_.push_back(es);
}

bool RevisitSet::contains(std::vector<Event> &es)
{
	for (auto &v : rev_)
		if (v == es)
			return true;
	return false;
}

/************************************************************
 ** Overloaded Operators
 ***********************************************************/

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const RevisitSet &rev)
{
	s << "Revisit Set:\n";
	for (auto &r : rev.rev_) {
		for (auto &e : r)
			s << e << " ";
		s << "\n";
	}
	return s;
}
