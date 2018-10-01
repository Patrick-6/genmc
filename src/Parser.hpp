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

#include "Library.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class Parser {
public:
	string readFile(const string &fileName);
	static std::string getFileLineByNumber(const std::string &absPath, int line);
	static void stripWhitespace(std::string &s);
	static void stripSlashes(std::string &absPath);
	static void parseInstFromMData(std::stringstream &ss,
				       std::vector<std::pair<int, std::string> > &prefix,
				       std::string functionName);
	std::vector<Library> parseSpecs(const string &fileName);
};
