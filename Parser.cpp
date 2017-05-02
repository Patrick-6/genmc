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

#include "Parser.hpp"

Parser::Parser()
{
}

string Parser::readFile(const string &fileName)
{
	ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);
	/* TODO: Error check here? */
	ifstream::pos_type fileSize = ifs.tellg();
	ifs.seekg(0, ios::beg);
	/* TODO: Does tellg work on all platforms? */
	vector<char> bytes(fileSize);
	ifs.read(&bytes[0], fileSize);
	
	return string(&bytes[0], fileSize);
}


	
