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

#ifndef __IMM_MO_DRIVER_HPP__
#define __IMM_MO_DRIVER_HPP__

#include "GenMCDriver.hpp"

class IMMDriver : public GenMCDriver {

public:
	IMMDriver(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		  std::unique_ptr<ModuleInfo> MI);

	void updateLabelViews(EventLabel *lab) override;
	void changeRf(Event read, Event store) override;
	void initConsCalculation() override;

	bool isConsistent(const Event &e) override;
	VerificationError checkErrors(const Event &e) override {
		return VerificationError::VE_OK;
	}
	std::unique_ptr<VectorClock>
	getPrefixView(Event e) override;

	std::vector<Event>
	getCoherentRevisits(const WriteLabel *sLab, const VectorClock &pporf) override;
	std::vector<Event>
	getCoherentStores(SAddr addr, Event read) override;
	llvm::iterator_range<ExecutionGraph::co_iterator>
	getCoherentPlacings(SAddr addr, Event read, bool isRMW)	override;

	const View &
	getHbView(const Event &e) override;
};

#endif /* __IMM_WB_DRIVER_HPP__ */
