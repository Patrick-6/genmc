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

#ifndef __SC_DRIVER_HPP__
#define __SC_DRIVER_HPP__

#include "GenMCDriver.hpp"

class SCDriver : public GenMCDriver {

public:
	SCDriver(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		 std::unique_ptr<ModuleInfo> MI);

	void updateLabelViews(EventLabel *lab, const DepInfo *deps) override;
	Event findDataRaceForMemAccess(const MemAccessLabel *mLab) override;
	void changeRf(Event read, Event store) override;
	void updateStart(Event create, Event start) override;
	bool updateJoin(Event join, Event childLast) override;
	void initConsCalculation() override;

	bool isConsistent(const Event &e);

private:
	View calcBasicHbView(Event e) const;
	View calcBasicPorfView(Event e) const;
	void calcWriteMsgView(WriteLabel *lab);
	void calcRMWWriteMsgView(WriteLabel *lab);

	void calcBasicViews(EventLabel *lab);
	void calcReadViews(ReadLabel *lab);
	void calcWriteViews(WriteLabel *lab);
	void calcFenceViews(FenceLabel *lab);
	void calcStartViews(ThreadStartLabel *lab);
	void calcJoinViews(ThreadJoinLabel *lab);
	void calcFenceRelRfPoBefore(Event last, View &v);

	/* Returns true if aLab and bLab are in an RC11 data race*/
	bool areInDataRace(const MemAccessLabel *aLab, const MemAccessLabel *bLab);

	/* Returns an event that is racy with rLab, or INIT if none is found */
	Event findRaceForNewLoad(const ReadLabel *rLab);

	/* Returns an event that is racy with wLab, or INIT if none is found */
	Event findRaceForNewStore(const WriteLabel *wLab);

	enum class NodeStatus { unseen, entered, left };

	std::vector<NodeStatus> visited0;
	bool visit0(const Event &e);
};

#endif /* __SC_DRIVER_HPP__ */
