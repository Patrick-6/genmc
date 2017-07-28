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

#ifndef __VIEW_HPP__
#define __VIEW_HPP__

#include <llvm/Support/raw_ostream.h>

#include <vector>

class View {
protected:
	typedef std::vector<int> EventView;
	EventView view_;

public:
	/* Constructors */
	View();
	View(int size);

	/* Iterators */
	typedef EventView::iterator iterator;
	typedef EventView::const_iterator const_iterator;
	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();

	/* Basic operation on Views */
	bool empty() const;
	unsigned int size() const;
	View getCopy(int numThreads);
	void updateMax(View &v);
	View getMax(View &v);

	std::vector<int> toVector();

	/* Overloaded operators */
	inline int operator[](int idx) const {
		return view_[idx];
	}
	inline int &operator[](int idx) {
		return view_[idx];
	}
	inline bool operator<=(const View &v) const {
		for (auto i = 0u; i < this->size(); i++)
			if ((*this)[i] > v[i])
				return false;
		return true;
	}
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const View &v);
};

#endif /* __VIEW_HPP__ */
