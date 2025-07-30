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

#ifndef GENMC_PARSER_HPP
#define GENMC_PARSER_HPP

#include <fstream>
#include <iostream>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

using namespace std;

class Parser {
public:
	auto readFile(const string &fileName) -> string;
	static auto getFileLineByNumber(const std::string &absPath, int line) -> std::string;
	static void stripWhitespace(std::string &s);
	static void stripSlashes(std::string &absPath);
	static void parseInstFromMData(std::pair<int, std::string> &locAndFile,
				       std::string functionName,
				       llvm::raw_ostream &os = llvm::dbgs());
};

#endif /* GENMC_PARSER_HPP */
