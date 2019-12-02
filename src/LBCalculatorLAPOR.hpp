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

#ifndef __LB_CALCULATOR_LAPOR_HPP__
#define __LB_CALCULATOR_LAPOR_HPP__

#include "ExecutionGraph.hpp"
#include <unordered_map>
#include <vector>

class LBCalculatorLAPOR {

private:
	struct Relations {
		typedef std::unordered_map<const llvm::GenericValue *, Matrix2D<Event> > WbOrder;

		Relations() = default;
		Relations (std::vector<Event> &&events, std::vector<Event> &&locks)
			: hbRelation(std::move(events)),
			  lbRelation(std::move(locks)), wbRelation() {};

		void clear();

		Matrix2D<Event> hbRelation;
		Matrix2D<Event> lbRelation;
		WbOrder wbRelation;
	};

public:
	/* Constructor */
	LBCalculatorLAPOR(ExecutionGraph &g) : g(g) {}

	/* Returns the first memory access event in the critical section
	 * that "lock" opens */
	Event getFirstMemAccessInCS(const Event lock) const;

	/* Returns the last memory access event in the critical section
	 * that "lock" opens */
	Event getLastMemAccessInCS(const Event lock) const;

	/* Returns a linear extension of LB */
	std::vector<Event> getLbOrdering() const;

	/* Returns whether the associated graph is LB-consistent */
	bool isLbConsistent();

	void addLockToList(const Event lock);
	void removeLocksAfter(const VectorClock &preds);

	/* TODO: Add comments for relations and move to protected */
	void calcHbRelation();
	Matrix2D<Event> calcWbRelation(const llvm::GenericValue *addr);
	void calcLbFromLoad(const ReadLabel *rLab, const LockLabelLAPOR *lLab);
	void calcLbFromStore(const WriteLabel *wLab, const LockLabelLAPOR *lLab);
	void calcLbRelation();
	bool addLbConstraints();
	bool calcLbFixpoint();

protected:
	std::vector<Event> collectEvents() const;
	std::vector<Event> collectLocks() const;

private:
	/* The execution graph to the lifetime of which the calculator is bound */
	ExecutionGraph &g;

	/* A list of all locks currently present in g */
	std::vector<Event> locks;

	/* The necessary relations stored by the LB calculator */
	Relations relations;
};

#endif /* __LB_CALCULATOR_LAPOR_HPP__ */
