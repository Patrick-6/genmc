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

#ifndef __DEP_VIEW_HPP__
#define __DEP_VIEW_HPP__

#include "Error.hpp"
#include "View.hpp"
#include "VSet.hpp"
#include <llvm/ADT/IndexedMap.h>
#include <llvm/Support/raw_ostream.h>

class DepView {
private:
	using Holes = VSet<int>;

	class HoleView {
		llvm::IndexedMap<Holes> hs_;

	public:
		HoleView() : hs_(Holes()) {}

		unsigned int size() const { return hs_.size(); }

		inline const Holes& operator[](int idx) const {
			if (idx < hs_.size())
				return hs_[idx];
			BUG();
		}

		inline Holes& operator[](int idx) {
			hs_.grow(idx);
			return hs_[idx];
		}
	};

public:
	/* Constructors */
	DepView() : view_(), holes_() {}

	/* Basic operations on Views */
	unsigned int size() const { return view_.size(); }
	bool empty() const { return size() == 0; }
	bool contains(const Event e) const;
	void addHole(const Event e);
	DepView& update(const DepView &v);

	/* Overloaded operators */
	inline int operator[](int idx) const {
		return view_[idx];
	}
	inline int &operator[](int idx) {
		holes_[idx]; /* grow both */
		return view_[idx];
	}

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const DepView &v);

private:
	View view_;
	HoleView holes_;
};

#endif /* __DEP_VIEW_HPP__ */
