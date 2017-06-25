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

#ifndef __MOD_ORDER_HPP__
#define __MOD_ORDER_HPP__

#include "Event.hpp"
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>

/*
 * ModOrder class - This class represents the modification order
 */
class ModOrder {
protected:
	typedef std::unordered_map<llvm::GenericValue *, std::vector<Event> > ModifOrder;
	ModifOrder mo_;

public:
	/* Constructors */
	ModOrder();

	/* Iterators */
	typedef ModifOrder::iterator iterator;
	typedef ModifOrder::const_iterator const_iterator;
	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();

	/* Basic getter/setter methods  */
	llvm::GenericValue *getAddrAtPos(ModOrder::iterator it);
	std::vector<Event> getAtLoc(llvm::GenericValue *addr);
	void addAtLocEnd(llvm::GenericValue *addr, Event e);
	void addAtLocPos(llvm::GenericValue *addr, std::vector<Event>::iterator it, Event e);
	void setLoc(llvm::GenericValue *addr, std::vector<Event> &locMO);
	std::vector<Event>::iterator getRMWPos(llvm::GenericValue *addr, Event rf);

	/* Overloaded operators */
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const ModOrder &rev);
};

#endif /* __REVISIT_SET_HPP__ */
