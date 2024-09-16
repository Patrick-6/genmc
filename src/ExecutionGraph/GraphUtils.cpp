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

#include "ExecutionGraph/GraphUtils.hpp"
#include "ExecutionGraph/ExecutionGraph.hpp"
#include "ExecutionGraph/GraphIterators.hpp"

bool isHazptrProtected(const MemAccessLabel *mLab)
{
	auto &g = *mLab->getParent();
	BUG_ON(!mLab->getAddr().isDynamic());

	auto *aLab = mLab->getAlloc();
	BUG_ON(!aLab);

	auto mpreds = po_preds(g, mLab);
	auto pLabIt = std::ranges::find_if(mpreds, [&](auto &lab) {
		auto *pLab = llvm::dyn_cast<HpProtectLabel>(&lab);
		return pLab && aLab->contains(pLab->getProtectedAddr());
	});
	if (pLabIt == mpreds.end() || !llvm::isa<HpProtectLabel>(&*pLabIt))
		return false;

	auto *pLab = llvm::dyn_cast<HpProtectLabel>(&*pLabIt);
	for (auto j = pLab->getIndex() + 1; j < mLab->getIndex(); j++) {
		auto *lab = g.getEventLabel(Event(mLab->getThread(), j));
		if (auto *oLab = llvm::dyn_cast<HpProtectLabel>(lab))
			if (oLab->getHpAddr() == pLab->getHpAddr())
				return false;
	}
	return true;
}
