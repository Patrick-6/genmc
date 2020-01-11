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

#ifndef __PSC_CALCULATOR_HPP__
#define __PSC_CALCULATOR_HPP__

#include "Calculator.hpp"
#include "DriverGraphEnumAPI.hpp"
#include "EventLabel.hpp"
#include "Error.hpp"
#include "GraphManager.hpp"

class PSCCalculator : public Calculator {

public:
	PSCCalculator(GraphManager &m, GlobalCalcMatrix &hb,
		      GlobalCalcMatrix &psc, PerLocCalcMatrix &co)
		: Calculator(m), hbRelation(hb),
		  pscRelation(psc), coRelation(co) {}

	/* Checks whether the provided condition "cond" holds for PSC.
	 * The calculation type (e.g., weak, full, etc) is determined by "t" */
	template <typename F>
	bool checkPscCondition(CheckPSCType t, F cond) const;

	/* Returns true if PSC is acyclic */
	bool isPscAcyclic(CheckPSCType t) const;

	/* Overrided Calculator methods */

	/* Initialize necessary matrices */
	void initCalc() override { BUG(); };

	/* Performs a step of the LB calculation */
	Calculator::CalculationResult doCalc() override { BUG(); };

	/* The calculator is informed about the removal of some events */
	void removeAfter(const VectorClock &preds) override { BUG(); };

	/* The calculator is informed about the restoration of some events */
	void restorePrefix(const ReadLabel *rLab,
			   const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
			   const std::vector<std::pair<Event, Event> > &status) override { BUG(); };

private:
	/* Returns a list with all accesses that are accessed at least twice */
	std::vector<const llvm::GenericValue *> getDoubleLocs() const;

	std::vector<Event> calcSCFencesSuccs(const std::vector<Event> &fcs,
					     const Event e) const;
	std::vector<Event> calcSCFencesPreds(const std::vector<Event> &fcs,
					     const Event e) const;
	std::vector<Event> calcSCSuccs(const std::vector<Event> &fcs,
				       const Event e) const;
	std::vector<Event> calcSCPreds(const std::vector<Event> &fcs,
				       const Event e) const;
	std::vector<Event> calcRfSCSuccs(const std::vector<Event> &fcs,
					 const Event e) const;
	std::vector<Event> calcRfSCFencesSuccs(const std::vector<Event> &fcs,
					       const Event e) const;

	void addRbEdges(const std::vector<Event> &fcs,
			const std::vector<Event> &moAfter,
			const std::vector<Event> &moRfAfter,
			Matrix2D<Event> &matrix, const Event &e) const;
	void addMoRfEdges(const std::vector<Event> &fcs,
			  const std::vector<Event> &moAfter,
			  const std::vector<Event> &moRfAfter,
			  Matrix2D<Event> &matrix, const Event &e) const;
	void addSCEcos(const std::vector<Event> &fcs,
		       const std::vector<Event> &mo,
		       Matrix2D<Event> &matrix) const;
	void addSCEcos(const std::vector<Event> &fcs,
		       Matrix2D<Event> &wbMatrix,
		       Matrix2D<Event> &pscMatrix) const;

	template <typename F>
	bool addSCEcosMO(const std::vector<Event> &fcs,
			 const std::vector<const llvm::GenericValue *> &scLocs,
			 Matrix2D<Event> &psc, F cond) const;
	template <typename F>
	bool addSCEcosWBWeak(const std::vector<Event> &fcs,
			     const std::vector<const llvm::GenericValue *> &scLocs,
			     Matrix2D<Event> &psc, F cond) const;
	template <typename F>
	bool addSCEcosWB(const std::vector<Event> &fcs,
			 const std::vector<const llvm::GenericValue *> &scLocs,
			 Matrix2D<Event> &matrix, F cond) const;
	template <typename F>
	bool addSCEcosWBFull(const std::vector<Event> &fcs,
			     const std::vector<const llvm::GenericValue *> &scLocs,
			     Matrix2D<Event> &matrix, F cond) const;

	void addInitEdges(const std::vector<Event> &fcs,
			  Matrix2D<Event> &matrix) const;
	void addSbHbEdges(Matrix2D<Event> &matrix) const;
	template <typename F>
	bool addEcoEdgesAndCheckCond(CheckPSCType t,
				     const std::vector<Event> &fcs,
				     Matrix2D<Event> &psc, F cond) const;

	/* Relation matrices participating in PSC */
	GlobalCalcMatrix &hbRelation;
	GlobalCalcMatrix &pscRelation;
	PerLocCalcMatrix &coRelation;
};

#endif /* __PSC_CALCULATOR_HPP__ */
