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

#include "ARCalculator.hpp"

bool ARCalculator::addArConstraints()
{
	auto &gm = getGraphManager();
	auto &g = gm.getGraph();
	auto &ar = gm.getGlobalRelation(GraphManager::RelationId::ar);
	auto &psc = gm.getGlobalRelation(GraphManager::RelationId::psc);
	Calculator::CalculationResult result;

	auto scs = g.getSCs();
	auto &fcs = scs.second;

	bool changed = false;
	for (auto &f1 : fcs) {
		for (auto &f2 : fcs) {
			if (psc(f1, f2) && !ar(f1, f2)) {
				changed = true;
				ar(f1, f2) = true;
			}
		}
	}
	return changed;
}

void ARCalculator::initCalc()
{
	auto &gm = getGraphManager();
	const auto &g = gm.getGraph();
	auto &ar = gm.getGlobalRelation(GraphManager::RelationId::ar);

	auto events = g.collectAllEvents([&](const EventLabel *lab)
					 { return llvm::isa<MemAccessLabel>(lab) ||
					   llvm::isa<FenceLabel>(lab) ||
					   llvm::isa<LockLabelLAPOR>(lab) ||
					   llvm::isa<UnlockLabelLAPOR>(lab); });
	ar = Matrix2D<Event>(std::move(events));
	g.populatePPoRfEntries(ar);
	ar.transClosure();
	return;
}

Calculator::CalculationResult ARCalculator::doCalc()
{
	auto &gm = getGraphManager();
	auto &arRelation = gm.getGlobalRelation(GraphManager::RelationId::ar);

	auto changed = addArConstraints();
	arRelation.transClosure();

	return Calculator::CalculationResult(changed, arRelation.isIrreflexive());
}

void ARCalculator::removeAfter(const VectorClock &preds)
{
	/* We do not track anything specific for AR */
	return;
}

void ARCalculator::restorePrefix(const ReadLabel *rLab,
				  const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				  const std::vector<std::pair<Event, Event> > &status)
{
	/* We do not track anything specific for AR */
	return;
}
