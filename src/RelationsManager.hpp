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

#ifndef __RELATIONS_MANAGER_HPP__
#define __RELATIONS_MANAGER_HPP__

#include "Calculator.hpp"
#include "CoherenceCalculator.hpp"
#include <unordered_map>

/*******************************************************************************
 **                         RelationsManager Class
 ******************************************************************************/

/*
 * An object that manages the different relations in the execution graph.
 * It is instantiated differently depending on the memory model, and is
 * largely responsible for performing the consistency checks. The relations
 * needed to be calculated may entail the initiation of one or more fixpoints.
 */
class RelationsManager {

protected:
	using CohMatrix = std::unordered_map<const llvm::GenericValue *,
					     std::vector<Event> >;

public:
	/* Adds the specified calculator to the list */
	void addCalculator(std::unique_ptr<Calculator> cc, bool partial = false);
	void addCoherenceCalculator(std::unique_ptr<CoherenceCalculator> cc);

	/* Returns the list of the calculators */
	const std::vector<const Calculator *> getCalcs() const;

	/* Returns the list of the partial calculators */
	const std::vector<const Calculator *> getPartialCalcs() const;

	/* Returns a reference to the graph's coherence calculator */
	CoherenceCalculator *getCoherenceCalculator() { return cohTracker.get(); };
	const CoherenceCalculator *getCoherenceCalculator() const { return cohTracker.get(); };

	/* Performs a step of all the specified calculations. Takes as
	 * a parameter whether a full calculation needs to be performed */
	Calculator::CalculationResult doOneStep(bool fullCalc = false);

	CohMatrix coherence;
	Matrix2D<Event> hb;
	Matrix2D<Event> lb;

private:
	void setCoherenceCalculator(std::unique_ptr<CoherenceCalculator> cc) {
		cohTracker = std::move(cc);
	};

	/* A list of all the calculations that need to be performed
	 * when checking for full consistency*/
	std::vector<std::unique_ptr<Calculator> > consistencyCalculators;

	/* The indices of all the calculations that need to be performed
	 * at each step of the algorithm (partial consistency check) */
	std::vector<int> partialConsCalculators;

	/* A coherence calculator for the graph */
	std::unique_ptr<CoherenceCalculator> cohTracker = nullptr;
};

#endif /* __RELATIONS_MANAGER_HPP__ */
