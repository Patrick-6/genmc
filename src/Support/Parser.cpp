/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#include "Parser.hpp"
#include <cstdio>

auto Parser::readFile(const string &fileName) -> string
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

auto Parser::getFileLineByNumber(const std::string &absPath, int line) -> std::string
{
	std::ifstream ifs(absPath);
	std::string s;
	int curLine = 0;

	while (ifs.good() && curLine < line) {
		std::getline(ifs, s);
		++curLine;
	}
	return s;
}

void Parser::stripWhitespace(std::string &s)
{
	s.erase(s.begin(),
		std::find_if(s.begin(), s.end(), [](int c) { return !std::isspace(c); }));
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); }).base(),
		s.end());
}

void Parser::stripSlashes(std::string &absPath)
{
	auto i = absPath.find_last_of('/');
	if (i != std::string::npos)
		absPath = absPath.substr(i + 1);
}

void Parser::parseInstFromMData(std::pair<int, std::string> &locAndFile, std::string functionName,
				llvm::raw_ostream &os /* llvm::outs() */)
{
	int line = locAndFile.first;
	std::string absPath = locAndFile.second;

	/* If line is default-valued or malformed, skip... */
	if (line <= 0)
		return;

	auto s = getFileLineByNumber(absPath, line);
	stripWhitespace(s);
	stripSlashes(absPath);

	if (functionName != "")
		os << "[" << functionName << "] ";
	os << absPath << ": " << line << ": ";
	os << s << "\n";
}
