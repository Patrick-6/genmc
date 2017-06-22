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

#ifndef __REVISIT_SET_HPP__
#define __REVISIT_SET_HPP__

#include "Event.hpp"

/*
 * RevisitSet class - This class represents the revisit set of an ExecutionGraph
 */
class RevisitSet {
protected:
	typedef std::vector<Event> RevSet;
	RevSet rev_;

public:
	/* Constructors */
	RevisitSet();

	/* Iterators */
	typedef RevSet::iterator iterator;
	typedef RevSet::const_iterator const_iterator;
	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();

	/* Basic getter/setters and existential checks */
	void add(Event &e);
	Event& getAtPos(RevisitSet::iterator &it);
	bool contains(Event &e);
	bool containsPorfBefore(std::vector<int> &before);

	/* Set modification methods */
	void removePorfBefore(std::vector<int> &before);
	void removePorfAfter(std::vector<int> &before);

	/* Overloaded operators */
	friend std::ostream& operator<<(std::ostream &s, const RevisitSet &rev);
};

#endif /* __REVISIT_SET_HPP__ */
