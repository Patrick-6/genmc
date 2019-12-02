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
	return g.collectAllEvents([](const EventLabel *lab)
				  { return llvm::isa<MemAccessLabel>(lab); });
}

std::vector<Event> LBCalculatorLAPOR::collectLocks() const
{
	return locks;
}

Event LBCalculatorLAPOR::getFirstMemAccessInCS(const Event lock) const
{
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
	std::vector<Event> threadPrios = relations.lbRelation.topoSort();

	/* Remove threads that are unordered with all other threads */
	threadPrios.
		erase(std::remove_if(threadPrios.begin(), threadPrios.end(),
				     [&](Event e){ return relations.lbRelation.hasNoEdges(e); }),
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
		auto lIndex = relations.lbRelation.getIndex(l);
		for (auto &o : locks) {
			if (!relations.lbRelation(lIndex, o))
				continue;
			auto o1 = getFirstMemAccessInCS(o);
			if (o1 != Event::getInitializer() &&
			    !relations.hbRelation(ul, o1)) {
				changed = true;
				relations.hbRelation(ul, o1) = true;
			}
		}
	}
	return changed;
}

void LBCalculatorLAPOR::calcHbRelation()
{
	BUG_ON(relations.hbRelation.getElems().empty());
	g.populateHbEntries(relations.hbRelation);
}

Matrix2D<Event> LBCalculatorLAPOR::calcWbRelation(const llvm::GenericValue *addr)
{
	return Matrix2D<Event>();
}

void LBCalculatorLAPOR::calcLbFromLoad(const ReadLabel *rLab,
				       const LockLabelLAPOR *lLab)
{
	const auto &wb = calcWbRelation(rLab->getAddr());
	Event lock = lLab->getPos();

	Event rfLock = g.getLastThreadLockAtLocLAPOR(rLab->getRf(), lLab->getLockAddr());
	if (rfLock != Event::getInitializer() && rfLock != lock &&
	    rLab->getRf().index >= rfLock.index) {
		relations.lbRelation(rfLock, lock) = true;
	}

	for (auto &s : wb.getElems()) {
		if (!rLab->getRf().isInitializer() && !wb(rLab->getRf(), s))
			continue;
		auto sLock = g.getLastThreadLockAtLocLAPOR(s, lLab->getLockAddr());
		if (sLock.isInitializer() || sLock == lock)
			continue;

		relations.lbRelation(lock, sLock) = true;
	}
}

void LBCalculatorLAPOR::calcLbFromStore(const WriteLabel *wLab,
					const LockLabelLAPOR *lLab)
{
	const auto &wb = calcWbRelation(wLab->getAddr());
	Event lock = lLab->getPos();

	/* Add an lb-edge if there exists a wb-later store
	 * in a different critical section of lLab */
	auto labIndex = wb.getIndex(wLab->getPos());
	for (auto &s : wb.getElems()) {
		if (!wb(labIndex, s))
			continue;
		auto sLock = g.getLastThreadLockAtLocLAPOR(s, lLab->getLockAddr());
		if (sLock.isInitializer() || sLock == lock)
			continue;

		relations.lbRelation(lock, sLock) = true;
	}
}

void LBCalculatorLAPOR::calcLbRelation()
{
	for (auto &l : relations.lbRelation) {
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
	relations.lbRelation.transClosure();
}

bool LBCalculatorLAPOR::calcLbFixpoint()
{
	/* Fixpoint calculation */
	do {
		relations.wbRelation.clear();
		relations.hbRelation.transClosure();
		if (!relations.hbRelation.isIrreflexive())
			return false;
		calcLbRelation();
		if (!relations.lbRelation.isIrreflexive())
			return false;
	} while (addLbConstraints());

	/* Check that no reads read from overwritten initialization write
	 * and compute WB for all locations in view */
	const auto &elems = relations.hbRelation.getElems();
	for (auto r = 0u; r < elems.size(); r++) {
		const EventLabel *lab = g.getEventLabel(elems[r]);
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab)) {
			calcWbRelation(wLab->getAddr());
			continue;
		}

		if (!llvm::isa<ReadLabel>(lab))
			continue;

		auto *rLab = static_cast<const ReadLabel *>(lab);
		if (rLab->getRf() != Event::getInitializer())
			continue;

		auto &stores = calcWbRelation(rLab->getAddr()).getElems();
		for (auto s : stores) {
			if (relations.hbRelation(s, r))
				return false;
		}
	}

	/* Check that WB is acyclic */
	return std::all_of(relations.wbRelation.begin(), relations.wbRelation.end(),
			[](std::pair<const llvm::GenericValue *, Matrix2D<Event> > p)
			   { return p.second.isIrreflexive(); });
}

bool LBCalculatorLAPOR::isLbConsistent()
{
	auto es = collectEvents();
	auto ls = collectLocks();

	relations = Relations(std::move(es), std::move(ls));
	calcHbRelation();
	return calcLbFixpoint();
}

void LBCalculatorLAPOR::addLockToList(const Event lock)
{
	locks.push_back(lock);
}

void LBCalculatorLAPOR::removeLocksAfter(const VectorClock &preds)
{
	locks.erase(std::remove_if(locks.begin(), locks.end(), [&](Event &e)
				   { return !preds.contains(e); }),
		    locks.end());
}
