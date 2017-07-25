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

#include "View.hpp"

View::View() {}

View::View(int size) : view_(size, 0) {}


bool View::empty() const
{
	return view_.empty();
}

unsigned int View::size() const
{
	return view_.size();
}

View View::getCopy(int numThreads)
{
	if (this->empty())
		return View(numThreads);
	return *this;
}

void View::updateMax(View &v)
{
	if (v.empty())
		return;

	for (auto i = 0u; i < this->size(); i++)
		if ((*this)[i] < v[i])
			(*this)[i] = v[i];
	return;
}

View View::getMax(View &v)
{
	if (this->empty())
		return v;
	if (v.empty())
		return *this;

	View result(*this);
	for (auto i = 0u; i < this->size(); i++)
		if (result[i] < v[i])
			result[i] = v[i];
	return result;
}
