/*
 * RCMC -- Model Checking for C11 programs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#include "Config.hpp"
#include "Driver.hpp"
#include "Error.hpp"

#include <cstdlib>
#include <fstream>

int main(int argc, char **argv)
{
	Config *conf = new Config(argc, argv);

	conf->getConfigOptions(); 
	if (system(conf->runCommand)) { /* TODO: Fix this! Fork process.. */
		WARN("Compilation failed! Aborting...");
		delete conf;
		return -ECOMPILE;
	}

	Driver *driver = new Driver(conf);
	
	driver->run();

	delete conf;
	delete driver;	
	/* TODO: Check globalContext.destroy() and llvm::shutdown() */

	return 0;
}
