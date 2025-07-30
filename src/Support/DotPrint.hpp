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

#ifndef GENMC_DOTPRINT_HPP
#define GENMC_DOTPRINT_HPP

#include <llvm/Support/raw_ostream.h>

#include <map>

template <typename T>
inline void printDotEdge(llvm::raw_ostream &os, const T &from, const T &to,
			 std::map<std::string, std::string> &&attrs = {})
{
	os << "\"" << from << "\"" << "->" << "\"" << to << "\"";
	if (!attrs.empty()) {
		os << "[";
		for (auto const &it : attrs) {
			os << it.first << "=" << it.second << " ";
		}
		os << "]";
	}
	os << " ";
};

template <typename T>
inline void printlnDotEdge(llvm::raw_ostream &os, const T &from, const T &to,
			   std::map<std::string, std::string> &&attrs = {})
{
	printDotEdge(os, from, to, std::move(attrs));
	os << "\n";
};

template <typename T>
inline void printDotEdge(llvm::raw_ostream &os, const std::pair<T, T> &e,
			 std::map<std::string, std::string> &&attrs = {})
{
	printDotEdge(os, e.first, e.second, std::move(attrs));
}

template <typename T>
inline void printlnDotEdge(llvm::raw_ostream &os, const std::pair<T, T> &e,
			   std::map<std::string, std::string> &&attrs = {})
{
	printlnDotEdge(os, e.first, e.second, std::move(attrs));
}

#endif /* GENMC_DOTPRINT_HPP */
