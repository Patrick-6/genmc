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

class DepInfo {

protected:
	using Set = VSet<Event>;

public:
	DepInfo() : set_() {}
	DepInfo(Event e) : set_({ e }) {}

	DepInfo depUnion(const DepInfo& dep) const;
	void update(const DepInfo& dep);
	void clear();

	bool contains(Event e) const { return set_.count(e); }
	bool empty() const;

	using iterator = typename Set::iterator;
	using const_iterator = typename Set::const_iterator;
	using reverse_iterator = typename Set::reverse_iterator;
	using const_reverse_iterator = typename Set::const_reverse_iterator;

	iterator begin() { return set_.begin(); };
	iterator end() { return set_.end(); };
	const_iterator begin() const { return set_.begin(); };
	const_iterator end() const { return set_.end(); };

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const DepInfo &dep);

private:
	Set set_;

};

#endif /* __DEP_INFO_HPP__ */
