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

#include "config.h"
#include "RC11Driver.hpp"
#include "Interpreter.h"
#include "ExecutionGraph.hpp"
#include "GraphIterators.hpp"
#include "PersistencyChecker.hpp"
#include "RC11Checker.hpp"

RC11Driver::RC11Driver(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		       std::unique_ptr<ModuleInfo> MI)
	: GenMCDriver(conf, std::move(mod), std::move(MI)) {}

void RC11Driver::updateLabelViews(EventLabel *lab)
{
	const auto &g = getGraph();

	lab->setCalculated(RC11Checker(getGraph()).calculateSaved(lab));
	lab->setViews(RC11Checker(getGraph()).calculateViews(lab));
}

void RC11Driver::changeRf(Event read, Event store)
{
	auto &g = getGraph();

	/* Change the reads-from relation in the graph */
	g.changeRf(read, store);
	return;
}

bool RC11Driver::isConsistent(const EventLabel *lab)
{
	return RC11Checker(getGraph()).isConsistent(lab);
}

VerificationError RC11Driver::checkErrors(const EventLabel *lab)
{
	return RC11Checker(getGraph()).checkErrors(lab);
}

std::vector<Event>
RC11Driver::getCoherentRevisits(const WriteLabel *sLab,
				const VectorClock &pporf)
{
	return RC11Checker(getGraph()).getCoherentRevisits(sLab, pporf);
}

std::vector<Event>
RC11Driver::getCoherentStores(SAddr addr, Event read)
{
	return RC11Checker(getGraph()).getCoherentStores(addr, read);
}

llvm::iterator_range<ExecutionGraph::co_iterator>
RC11Driver::getCoherentPlacings(SAddr addr, Event read, bool isRMW)
{
	return RC11Checker(getGraph()).getCoherentPlacings(addr, read, isRMW);
}

std::unique_ptr<VectorClock>
RC11Driver::getPrefixView(const EventLabel *lab)
{
	return RC11Checker(getGraph()).getPPoRfBefore(lab);
}

const View &
RC11Driver::getHbView(const EventLabel *lab)
{
	return RC11Checker(getGraph()).getHbView(lab);
}
