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

#include "Error.hpp"
#include "DepView.hpp"

bool DepView::contains(const Event e) const
{
	return e.index <= (*this)[e.thread] && e.thread <= holes_.size() &&
		!holes_[e.thread].count(e.index);
}

void DepView::addHole(const Event e)
{
	BUG_ON(e.index > (*this)[e.thread]);
	holes_[e.thread].insert(e.index);
}

void DepView::addHolesInRange(Event start, int endIdx)
{
	for (auto i = start.index; i < endIdx; i++)
		addHole(Event(start.thread, i));
}

void DepView::removeHole(const Event e)
{
	holes_[e.thread].erase(e.index);
}

void DepView::removeAllHoles(int thread)
{
	holes_[thread].clear();
}

void DepView::removeHolesInRange(Event start, int endIdx)
{
	for (auto i = start.index; i < endIdx; i++)
		removeHole(Event(start.thread, i));
}

DepView& DepView::update(const DepView &v)
{
	if (v.empty())
		return *this;

	for (auto i = 0u; i < v.size(); i++) {
		auto isec = holes_[i].intersectWith(v.holes_[i]);
		if ((*this)[i] < v[i]) {
			isec.insert(std::lower_bound(v.holes_[i].begin(),
						     v.holes_[i].end(),
						     (*this)[i] + 1),
				    v.holes_[i].end());
			(*this)[i] = v[i];
		} else {
			isec.insert(std::lower_bound(holes_[i].begin(),
						     holes_[i].end(),
						     v[i] + 1),
				    holes_[i].end());
			}
		holes_[i] = std::move(isec);
	}
	return *this;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const DepView &v)
{
	s << "[\n";
	for (auto i = 0u; i < v.size(); i++) {
		s << "\t" << i << ": " << v[i] << " ( ";
		for (auto &h : v.holes_[i])
			s << h << " ";
		s << ")\n";
	}

	s << "]";
	return s;
}
