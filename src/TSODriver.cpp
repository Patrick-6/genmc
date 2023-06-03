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
#include "TSODriver.hpp"
#include "Interpreter.h"
#include "TSOChecker.hpp"

TSODriver::TSODriver(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		   std::unique_ptr<ModuleInfo> MI)
	: GenMCDriver(conf, std::move(mod), std::move(MI)) {}

void TSODriver::updateLabelViews(EventLabel *lab)
{
	BUG();
}

bool TSODriver::isConsistent(const EventLabel *lab)
{
	BUG();
	// if (lab->getThread() == getGraph().getRecoveryRoutineId())
	// 	return TSOChecker(getGraph()).isRecoveryValid(lab);
	// return TSOChecker(getGraph()).isConsistent(lab);
}

bool TSODriver::isRecoveryValid(const EventLabel *lab)
{
	BUG();
}
