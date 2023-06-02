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
#include "IMMDriver.hpp"
#include "Interpreter.h"
#include "ExecutionGraph.hpp"
#include "IMMChecker.hpp"

IMMDriver::IMMDriver(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		     std::unique_ptr<ModuleInfo> MI)
	: GenMCDriver(conf, std::move(mod), std::move(MI)) {}

void IMMDriver::updateLabelViews(EventLabel *lab)
{
	const auto &g = getGraph();

	lab->setCalculated(IMMChecker(getGraph()).calculateSaved(lab->getPos()));
	lab->setViews(IMMChecker(getGraph()).calculateViews(lab->getPos()));
}

void IMMDriver::changeRf(Event read, Event store)
{
	auto &g = getGraph();

	/* Change the reads-from relation in the graph */
	g.changeRf(read, store);
}

void IMMDriver::initConsCalculation()
{
	return;
}

bool IMMDriver::isConsistent(const Event &e)
{
	return IMMChecker(getGraph()).isConsistent(e);
}

std::unique_ptr<VectorClock>
IMMDriver::getPrefixView(Event e)
{
	return IMMChecker(getGraph()).getPPoRfBefore(e);
}

std::vector<Event>
IMMDriver::getCoherentStores(SAddr addr, Event read)
{
	return IMMChecker(getGraph()).getCoherentStores(addr, read);
}

std::vector<Event>
IMMDriver::getCoherentRevisits(const WriteLabel *sLab,
				const VectorClock &pporf)
{
	return IMMChecker(getGraph()).getCoherentRevisits(sLab, pporf);
}

llvm::iterator_range<ExecutionGraph::co_iterator>
IMMDriver::getCoherentPlacings(SAddr addr, Event read, bool isRMW)
{
	return IMMChecker(getGraph()).getCoherentPlacings(addr, read, isRMW);
}

const View &
IMMDriver::getHbView(const Event &e)
{
	return IMMChecker(getGraph()).getHbView(e);
}
