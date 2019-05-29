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

#ifndef __IMM_WB_DRIVER_HPP__
#define __IMM_WB_DRIVER_HPP__

#include "GenMCDriver.hpp"

class IMMWBDriver : public GenMCDriver {

public:

	IMMWBDriver(std::unique_ptr<Config> conf, std::unique_ptr<llvm::Module> mod,
		    std::vector<Library> &granted, std::vector<Library> &toVerify,
		    clock_t start)
		: GenMCDriver(std::move(conf), std::move(mod), granted, toVerify, start) {};

	void updateGraphDependencies(Event pos,
				     const DepInfo &addr,
				     const DepInfo &data,
				     const DepInfo &ctrl,
				     const DepInfo &addrPo,
				     const DepInfo &casDep) override;
	const View &getPrefix(const Event e) { return getGraph().getPPoRfBefore(e).getView(); }
	void restrictGraph(unsigned int stamp) override;

	std::vector<Event> getStoresToLoc(const llvm::GenericValue *addr) override;
	std::pair<int, int> getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW) override;
	std::vector<Event> getRevisitLoads(const WriteLabel *lab) override;
	std::pair<std::vector<std::unique_ptr<EventLabel> >,
		  std::vector<std::pair<Event, Event> > >
	getPrefixToSaveNotBefore(const WriteLabel *wLab, const ReadLabel *rLab) override;
	bool checkPscAcyclicity() override;
	bool isExecutionValid() override;
};

#endif /* __IMM_WB_DRIVER_HPP__ */
