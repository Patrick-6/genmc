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

#include "LBCalculatorLAPOR.hpp"

std::vector<Event> LBCalculatorLAPOR::collectEvents() const
{
	const auto &g = getGraphManager().getGraph();
	return g.collectAllEvents([](const EventLabel *lab)
				  { return llvm::isa<MemAccessLabel>(lab); });
}

std::vector<Event> LBCalculatorLAPOR::collectLocks() const
{
	return locks;
}

Event LBCalculatorLAPOR::getFirstMemAccessInCS(const Event lock) const
{
	const auto &g = getGraphManager().getGraph();
	const EventLabel *lab = g.getEventLabel(lock);
	BUG_ON(!llvm::isa<LockLabelLAPOR>(lab));
	auto *lLab = static_cast<const LockLabelLAPOR *>(lab);

	for (auto i = lLab->getIndex() + 1u; i < g.getThreadSize(lLab->getThread()); i++) {
		const EventLabel *eLab = g.getEventLabel(Event(lLab->getThread(), i));

		if (llvm::isa<MemAccessLabel>(eLab))
			return eLab->getPos();
		if (auto *uLab = llvm::dyn_cast<UnlockLabelLAPOR>(eLab)) {
			if (uLab->getLockAddr() == lLab->getLockAddr())
				return Event::getInitializer();
		}
	}
	return Event::getInitializer();
}

Event LBCalculatorLAPOR::getLastMemAccessInCS(const Event lock) const
{
	const auto &g = getGraphManager().getGraph();
	const EventLabel *lab = g.getEventLabel(lock);
	BUG_ON(!llvm::isa<LockLabelLAPOR>(lab));
	auto *lLab = static_cast<const LockLabelLAPOR *>(lab);

	auto i = 0u;
	for (i = lLab->getIndex() + 1u; i < g.getThreadSize(lLab->getThread()); i++) {
		const EventLabel *eLab = g.getEventLabel(Event(lLab->getThread(), i));

		if (auto *uLab = llvm::dyn_cast<UnlockLabelLAPOR>(eLab)) {
			if (uLab->getLockAddr() == lLab->getLockAddr())
				break;
		}
	}
	for (--i; i > lLab->getIndex(); i--) {
		const EventLabel *eLab = g.getEventLabel(Event(lLab->getThread(), i));

		if (llvm::isa<MemAccessLabel>(eLab))
			return eLab->getPos();
	}
	return Event::getInitializer();
}

std::vector<Event> LBCalculatorLAPOR::getLbOrdering() const
{
	std::vector<Event> threadPrios = lbRelation.topoSort();

	/* Remove threads that are unordered with all other threads */
	threadPrios.
		erase(std::remove_if(threadPrios.begin(), threadPrios.end(),
				     [&](Event e){ return lbRelation.hasNoEdges(e); }),
		      threadPrios.end());
	return threadPrios;
}

bool LBCalculatorLAPOR::addLbConstraints()
{
	bool changed = false;

	for (auto &l : locks) {
		auto ul = getLastMemAccessInCS(l);
		if (ul == Event::getInitializer())
			continue;
		auto lIndex = lbRelation.getIndex(l);
		for (auto &o : locks) {
			if (!lbRelation(lIndex, o))
				continue;
			auto o1 = getFirstMemAccessInCS(o);
			if (o1 != Event::getInitializer() &&
			    !hbRelation(ul, o1)) {
				changed = true;
				hbRelation(ul, o1) = true;
			}
		}
	}
	return changed;
}

void LBCalculatorLAPOR::calcLbFromLoad(const ReadLabel *rLab,
				       const LockLabelLAPOR *lLab)
{
	const auto &g = getGraphManager().getGraph();
	const auto &co = coRelation[rLab->getAddr()];
	Event lock = lLab->getPos();

	Event rfLock = g.getLastThreadLockAtLocLAPOR(rLab->getRf(), lLab->getLockAddr());
	if (rfLock != Event::getInitializer() && rfLock != lock &&
	    rLab->getRf().index >= rfLock.index) {
		lbRelation(rfLock, lock) = true;
	}

	for (auto &s : co.getElems()) {
		if (!rLab->getRf().isInitializer() && !co(rLab->getRf(), s))
			continue;
		auto sLock = g.getLastThreadLockAtLocLAPOR(s, lLab->getLockAddr());
		if (sLock.isInitializer() || sLock == lock)
			continue;

		lbRelation(lock, sLock) = true;
	}
}

void LBCalculatorLAPOR::calcLbFromStore(const WriteLabel *wLab,
					const LockLabelLAPOR *lLab)
{
	const auto &g = getGraphManager().getGraph();
	const auto &co = coRelation[wLab->getAddr()];
	Event lock = lLab->getPos();

	/* Add an lb-edge if there exists a co-later store
	 * in a different critical section of lLab */
	auto labIndex = co.getIndex(wLab->getPos());
	for (auto &s : co.getElems()) {
		if (!co(labIndex, s))
			continue;
		auto sLock = g.getLastThreadLockAtLocLAPOR(s, lLab->getLockAddr());
		if (sLock.isInitializer() || sLock == lock)
			continue;

		lbRelation(lock, sLock) = true;
	}
}

void LBCalculatorLAPOR::calcLbRelation()
{
	const auto &g = getGraphManager().getGraph();
	for (auto &l : lbRelation) {
		const EventLabel *lab = g.getEventLabel(l);
		BUG_ON(!llvm::isa<LockLabelLAPOR>(lab));
		auto *lLab = static_cast<const LockLabelLAPOR *>(lab);

		auto ul = getLastMemAccessInCS(l);
		if (ul == Event::getInitializer())
			continue;

		for (auto i = l.index; i <= ul.index; i++) { /* l.index >= 0 */
			const EventLabel *lab = g.getEventLabel(Event(l.thread, i));
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
				calcLbFromLoad(rLab, lLab);
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab))
				calcLbFromStore(wLab, lLab);
		}
	}
	lbRelation.transClosure();
}

void LBCalculatorLAPOR::initCalc()
{
	lbRelation = Matrix2D<Event>(locks);
	return;
}

Calculator::CalculationResult LBCalculatorLAPOR::doCalc()
{
	const auto &g = getGraphManager().getGraph();

	if (!hbRelation.isIrreflexive())
		return Calculator::CalculationResult(false, false);
	calcLbRelation();
	if (!lbRelation.isIrreflexive())
		return Calculator::CalculationResult(false, false);
	bool changed = addLbConstraints();
	hbRelation.transClosure();

	/* Check that no reads read from overwritten initialization write */
	const auto &elems = hbRelation.getElems();
	for (auto r = 0u; r < elems.size(); r++) {
		const EventLabel *lab = g.getEventLabel(elems[r]);

		if (!llvm::isa<ReadLabel>(lab))
			continue;

		auto *rLab = static_cast<const ReadLabel *>(lab);
		if (rLab->getRf() != Event::getInitializer())
			continue;

		auto &stores = coRelation[rLab->getAddr()].getElems();
		for (auto s : stores) {
			if (hbRelation(s, r))
				return Calculator::CalculationResult(changed, false);
		}
	}

	/* Check that co is acyclic */
	if (std::any_of(coRelation.begin(), coRelation.end(),
			[](std::pair<const llvm::GenericValue *, Matrix2D<Event> > p)
			{ return !p.second.isIrreflexive(); }))
		return Calculator::CalculationResult(changed, false);
	return Calculator::CalculationResult(changed, true);
}

void LBCalculatorLAPOR::removeAfter(const VectorClock &preds)
{
	locks.erase(std::remove_if(locks.begin(), locks.end(), [&](Event &e)
				   { return !preds.contains(e); }),
		    locks.end());
}

void LBCalculatorLAPOR::addLockToList(const Event lock)
{
	locks.push_back(lock);
}

void LBCalculatorLAPOR::restorePrefix(const ReadLabel *rLab,
				      const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				      const std::vector<std::pair<Event, Event> > &status)
{
	for (const auto &lab : storePrefix) {
		if (auto *lLab = llvm::dyn_cast<LockLabelLAPOR>(lab.get()))
			addLockToList(lLab->getPos());
	}
}
