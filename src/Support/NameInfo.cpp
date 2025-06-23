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

#include "NameInfo.hpp"

#include <algorithm>

/* Mark name at offset OFFSET as NAME */
void NameInfo::addOffsetInfo(unsigned int offset, std::string name)
{
	info.emplace_back(offset, name);

	/* Check if info needs sorting */
	if (info.size() <= 1)
		return;
	if (info[info.size() - 2].first < info[info.size() - 1].first)
		std::ranges::sort(info);
}

/* Returns name at offset O */
auto NameInfo::getNameAtOffset(unsigned int offset) const -> std::string
{
	if (info.empty())
		return "";

	for (auto i = 0U; i < info.size(); i++) {
		if (info[i].first > offset) {
			BUG_ON(i == 0);
			return info[i - 1].second;
		}
	}
	return info.back().second;
}

auto operator<<(llvm::raw_ostream &rhs, const NameInfo &info) -> llvm::raw_ostream &
{
	for (const auto &kv : info.info)
		rhs << "" << kv.first << ": " << kv.second << "\n";
	return rhs;
}
