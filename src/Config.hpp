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

#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <llvm/Support/CommandLine.h>
#include <set>

enum ModelType { WeakRA, MO, WB };

class Config {

protected:
	int argc;
	char **argv;

public:
	std::vector<std::string> cflags;
	std::string inputFile;
	ModelType model;
	std::string transformFile;
	int unroll;
	bool spinAssume;
	bool inputFromBitcodeFile;
	bool validateExecGraphs;
	bool printExecGraphs;
	bool countDuplicateExecs;
	bool printErrorTrace;
	std::set<std::string> visibleOptions;

	Config(int argc, char **argv);
	void getConfigOptions(void);
};

#endif /* __CONFIG_HPP__ */
