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

#include "RelationsManager.hpp"

void RelationsManager::addCalculator(std::unique_ptr<Calculator> cc,
				     bool partial /* = false */)
{
	auto size = getCalcs().size();
	consistencyCalculators.push_back(std::move(cc));

	/* Check whether it is part of the checks at each step */
	if (partial)
		partialConsCalculators.push_back(size);
}

void RelationsManager::addCoherenceCalculator(std::unique_ptr<CoherenceCalculator> cc)
{
	setCoherenceCalculator(std::move(cc));
}

const std::vector<unique_ptr<Calculator> > getCalcs() const
{
	std::vector<const Calculator *> result;

	for (auto i = 0u; i < consistencyCalculators.size(); i++)
		result.push_back(consistencyCalculators[i].get());
	return result;
}

const std::vector<unique_ptr<Calculator> > getPartialCalcs() const
{
	std::vector<const Calculator *> result;

	for (auto i = 0u; i < partialConsCalculators.size(); i++)
		result.push_back(consistencyCalculators[partialConsCalculators[i]].get());
	return result;
}

Calculator::CalculationResult RelationsManager::doOneStep(bool full /* = false */)
{
	Calculator::CalculationResult result;

	auto &calcs = getCalcs();
	auto &partial = getPartialCalcs();
	for (auto i = 0u; i < calcs().size(); i++) {
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
