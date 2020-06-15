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

#include "PersistenceChecker.hpp"

bool writeSameBlock(const DskWriteLabel *dw1, const DskWriteLabel *dw2,
		    unsigned int blockSize)
{
	if (dw1->getMapping() != dw2->getMapping())
		return false;

	ptrdiff_t off1 = (char *) dw1->getAddr() - (char *) dw1->getMapping();
	ptrdiff_t off2 = (char *) dw2->getAddr() - (char *) dw2->getMapping();
	return (off1 / blockSize) == (off2 / blockSize);
}

std::vector<Event> getDskWriteOperations(const ExecutionGraph &g)
{
	auto recId = g.getRecoveryRoutineId();
	return g.collectAllEvents([&](const EventLabel *lab) {
			BUG_ON(llvm::isa<DskWriteLabel>(lab) &&
			       lab->getThread() == recId);
			return llvm::isa<DskWriteLabel>(lab);
		});
}

void PersistenceChecker::calcPb()
{
	auto &g = getGraph();
	auto &pbRelation = getPbRelation();

	pbRelation = Calculator::GlobalRelation(getDskWriteOperations(g));

	/* Add all co edges to pb */
	g.getCoherenceCalculator()->initCalc();
	auto &coRelation = g.getPerLocRelation(ExecutionGraph::RelationId::co);
	for (auto &coLoc : coRelation) {
		/* We must only consider file operations */
		if (coLoc.second.empty() ||
		    !llvm::isa<DskWriteLabel>(g.getEventLabel(*coLoc.second.begin())))
			continue;
		for (auto &s1 : coLoc.second) {
			for (auto &s2 : coLoc.second) {
				if (coLoc.second(s1, s2))
					pbRelation.addEdge(s1, s2);
			}
		}
	}

	/* Add all sync + same-block edges
	 * FIXME: optimize */
	const auto &pbs = pbRelation.getElems();
	for (auto &d1 : pbs) {
		auto *lab1 = llvm::dyn_cast<DskWriteLabel>(g.getEventLabel(d1));
		BUG_ON(!lab1);
		for (auto &d2 : pbs) {
			if (d1 == d2)
				continue;
			auto *lab2 = llvm::dyn_cast<DskWriteLabel>(g.getEventLabel(d2));
			BUG_ON(!lab2);
			if (lab1->getPbView().contains(d2))
				pbRelation.addEdge(d2, d1);
			if (g.getHbBefore(d1).contains(d2)) {
				if (writeSameBlock(lab1, lab2, getBlockSize()))
					pbRelation.addEdge(d2, d1);
			}
		}

	}

	BUG_ON(!pbRelation.isIrreflexive());
	return;
}

bool PersistenceChecker::isStoreReadFromRecRoutine(Event s)
{
	auto &g = getGraph();
	auto recId = g.getRecoveryRoutineId();
	auto recLast = g.getLastThreadEvent(recId);

	BUG_ON(!llvm::isa<WriteLabel>(g.getEventLabel(s)));
	auto *wLab = static_cast<const WriteLabel *>(g.getEventLabel(s));
	auto &readers = wLab->getReadersList();
	return std::any_of(readers.begin(), readers.end(), [&](Event r)
			   { return r.thread == recId &&
				    r.index <= recLast.index; });
}

bool PersistenceChecker::isRecFromReadValid(const DskReadLabel *rLab)
{
	auto &g = getGraph();

	/* co should already be calculated due to calcPb() */
	auto &pbRelation = getPbRelation();
	auto &coRelation = g.getPerLocRelation(ExecutionGraph::RelationId::co);
	auto &coLoc = coRelation[rLab->getAddr()];
	auto &stores = coLoc.getElems();

	/* Check the existence of an rb;pb?;rf;rec cycle */
	for (auto &s : stores) {
		if (!rLab->getRf().isInitializer() && !coLoc(rLab->getRf(), s))
			continue;

		/* check for rb;rf;rec cycle */
		if (isStoreReadFromRecRoutine(s))
			return false;

		/* check for rb;pb;rf;rec cycle */
		for (auto &p : pbRelation.getElems()) {
			/* only consider pb-later elements */
			if (!pbRelation(s, p))
				continue;

			auto *pLab = g.getEventLabel(p);
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(pLab)) {
				if (isStoreReadFromRecRoutine(p))
					return false;
			}
		}
	}
	return true;
}

bool PersistenceChecker::isRecAcyclic()
{
	auto &g = getGraph();
	auto recId = g.getRecoveryRoutineId();
	BUG_ON(recId == -1);

	/* Calculate pb (and propagate co relation) */
	calcPb();

	for (auto j = 1u; j < g.getThreadSize(recId); j++) {
		const EventLabel *lab = g.getEventLabel(Event(recId, j));
		if (auto *rLab = llvm::dyn_cast<DskReadLabel>(lab)) {
			if (!isRecFromReadValid(rLab))
				return false;
		}
	}
	return true;
}
