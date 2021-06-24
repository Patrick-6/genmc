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

#include "DriverFactory.hpp"
#include "IMMDriver.hpp"
#include "RC11Driver.hpp"

std::unique_ptr<GenMCDriver>
DriverFactory::create(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
		      clock_t start)
{
	return DriverFactory::create(nullptr, std::move(conf), std::move(mod), start);
}

std::unique_ptr<GenMCDriver>
DriverFactory::create(ThreadPool *pool, std::shared_ptr<const Config> conf,
		      std::unique_ptr<llvm::Module> mod, clock_t start)
{
	GenMCDriver *driver = nullptr;
	switch (conf->model) {
	case ModelType::rc11:
		driver = new RC11Driver(conf, std::move(mod), start);
		break;
	case ModelType::imm:
		driver = new IMMDriver(conf, std::move(mod), start);
		break;
	default:
		BUG();
	}
	driver->setThreadPool(pool);
	return std::unique_ptr<GenMCDriver>(driver);
}
