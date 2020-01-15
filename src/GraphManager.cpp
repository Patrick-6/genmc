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

#include "GraphManager.hpp"
#include "MOCoherenceCalculator.hpp"
#include "WBCoherenceCalculator.hpp"

void GraphManager::addCalculator(std::unique_ptr<Calculator> cc,
				 RelationId r /* = other */,
				 bool partial /* = false */)
{
	auto size = getCalcs().size();
	consistencyCalculators.push_back(std::move(cc));

	/* Check whether it is part of the checks at each step */
	if (partial)
		partialConsCalculators.push_back(size);
	if (r != RelationId::other)
		relIndices[r] = size;
}

CoherenceCalculator *GraphManager::getCoherenceCalculator()
{
	return static_cast<CoherenceCalculator *>(consistencyCalculators[relIndices[RelationId::co]].get());
};

CoherenceCalculator *GraphManager::getCoherenceCalculator() const
{
	return static_cast<CoherenceCalculator *>(consistencyCalculators.at(relIndices.at(RelationId::co)).get());
};

LBCalculatorLAPOR *GraphManager::getLbCalculatorLAPOR()
{
	return static_cast<LBCalculatorLAPOR *>(consistencyCalculators[relIndices[RelationId::lb]].get());
};

std::vector<Event> GraphManager::getLbOrderingLAPOR()
{
	return getLbCalculatorLAPOR()->getLbOrdering();
}

const std::vector<const Calculator *> GraphManager::getCalcs() const
{
	std::vector<const Calculator *> result;

	for (auto i = 0u; i < consistencyCalculators.size(); i++)
		result.push_back(consistencyCalculators[i].get());
	return result;
}

const std::vector<const Calculator *> GraphManager::getPartialCalcs() const
{
	std::vector<const Calculator *> result;

	for (auto i = 0u; i < partialConsCalculators.size(); i++)
		result.push_back(consistencyCalculators[partialConsCalculators[i]].get());
	return result;
}

void GraphManager::doInits(bool full /* = false */)
{
	auto events = getGraph().collectAllEvents([&](const EventLabel *lab)
						  { return llvm::isa<MemAccessLabel>(lab) ||
						    llvm::isa<FenceLabel>(lab); });

	hb = Matrix2D<Event>(std::move(events));
	getGraph().populateHbEntries(hb);
	hb.transClosure();

	auto &calcs = consistencyCalculators;
	auto &partial = partialConsCalculators;
	for (auto i = 0u; i < calcs.size(); i++) {
		if (!full && std::find(partial.begin(), partial.end(), i) == partial.end())
			continue;

		calcs[i]->initCalc();
	}
	return;
}

Calculator::CalculationResult GraphManager::doCalcs(bool full /* = false */)
{
	Calculator::CalculationResult result;

	auto &calcs = consistencyCalculators;
	auto &partial = partialConsCalculators;
	for (auto i = 0u; i < calcs.size(); i++) {
		if (!full && std::find(partial.begin(), partial.end(), i) == partial.end())
			continue;

		result |= calcs[i]->doCalc();

		/* If an inconsistency was spotted, no reason to call
		 * the other calculators */
		if (!result.cons)
			return result;
	}
	return result;
}

const ReadLabel *GraphManager::addReadLabelToGraph(std::unique_ptr<ReadLabel> lab, Event rf)
{
	return getGraph().addReadLabelToGraph(std::move(lab), rf);
}

const WriteLabel *GraphManager::addWriteLabelToGraph(std::unique_ptr<WriteLabel> lab,
						     unsigned int offsetMO)
{
	getCoherenceCalculator()->addStoreToLoc(lab->getAddr(), lab->getPos(), offsetMO);
	return static_cast<const WriteLabel *>(addOtherLabelToGraph(std::move(lab)));
}

const WriteLabel *GraphManager::addWriteLabelToGraph(std::unique_ptr<WriteLabel> lab,
						     Event pred)
{
	getCoherenceCalculator()->addStoreToLocAfter(lab->getAddr(), lab->getPos(), pred);
	return static_cast<const WriteLabel *>(addOtherLabelToGraph(std::move(lab)));
}

const LockLabelLAPOR *GraphManager::addLockLabelToGraphLAPOR(std::unique_ptr<LockLabelLAPOR> lab)
{
	getLbCalculatorLAPOR()->addLockToList(lab->getLockAddr(), lab->getPos());
	return static_cast<const LockLabelLAPOR *>(addOtherLabelToGraph(std::move(lab)));
}

const EventLabel *GraphManager::addOtherLabelToGraph(std::unique_ptr<EventLabel> lab)
{
	return getGraph().addOtherLabelToGraph(std::move(lab));
}

void GraphManager::trackCoherenceAtLoc(const llvm::GenericValue *addr)
{
	return getCoherenceCalculator()->trackCoherenceAtLoc(addr);
}

const std::vector<Event>&
GraphManager::getStoresToLoc(const llvm::GenericValue *addr) const
{
	return getCoherenceCalculator()->getStoresToLoc(addr);
}

const std::vector<Event>&
GraphManager::getStoresToLoc(const llvm::GenericValue *addr)
{
	return getCoherenceCalculator()->getStoresToLoc(addr);
}

std::pair<int, int>
GraphManager::getCoherentPlacings(const llvm::GenericValue *addr,
				    Event pos, bool isRMW) {
	return getCoherenceCalculator()->getPossiblePlacings(addr, pos, isRMW);
};

std::vector<Event>
GraphManager::getCoherentStores(const llvm::GenericValue *addr, Event pos)
{
	return getCoherenceCalculator()->getCoherentStores(addr, pos);
}

std::vector<Event>
GraphManager::getCoherentRevisits(const WriteLabel *wLab)
{
	return getCoherenceCalculator()->getCoherentRevisits(wLab);
}

void GraphManager::changeStoreOffset(const llvm::GenericValue *addr,
				     Event s, int newOffset)
{
	BUG_ON(!llvm::isa<MOCoherenceCalculator>(getCoherenceCalculator()));
	auto *cohTracker = static_cast<MOCoherenceCalculator *>(
		getCoherenceCalculator());

	cohTracker->changeStoreOffset(addr, s, newOffset);
}

std::vector<std::pair<Event, Event> >
GraphManager::saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &prefix,
				  const ReadLabel *rLab) const
{
	return getCoherenceCalculator()->saveCoherenceStatus(prefix, rLab);
}

void GraphManager::cutToLabel(const EventLabel *rLab)
{
	auto preds = getGraph().getPredsView(rLab->getPos());

	/* Inform all calculators about the events cutted */
	auto &calcs = consistencyCalculators;
	for (auto i = 0u; i < calcs.size() ; i++)
		calcs[i]->removeAfter(*preds);

	getGraph().cutToStamp(rLab->getStamp());
}

void GraphManager::restoreStorePrefix(const ReadLabel *rLab,
				      std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				      std::vector<std::pair<Event, Event> > &moPlacings)
{
	auto &calcs = consistencyCalculators;
	for (auto i = 0u; i < calcs.size() ; i++)
		calcs[i]->restorePrefix(rLab, storePrefix, moPlacings);

	getGraph().restoreStorePrefix(rLab, storePrefix, moPlacings);
}
