#ifndef _KATER_PRINTER_HPP_
#define _KATER_PRINTER_HPP_

#include "Driver.hpp"
#include "NFA.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Printer {
public:
	Printer(std::string prefix);

	/* Outputs RES */
	void output(const NFAs *res);

private:
//	void printInclusionError(const std::string &s, const NFA &lhs, const NFA &rhs);
//	void printInclusionWarning(const std::string &s, const NFA &lhs, const NFA &rhs);

	void printCalculatorHeader(std::ostream &fout, const NFA &nfa, int n);
	void printCalculatorImpl(std::ostream &fout, const NFA &nfa, int n, VarStatus reduce);

	void printAcyclicHeader(std::ostream &fout, const NFA &nfa);
	void printAcyclicImpl(std::ostream &fout, const NFA &nfa);

	void printInclusionHeader(std::ostream &fout, const NFA &lhs, const NFA &rhs, int n);
	void printInclusionImpl(std::ostream &fout, const NFA &lhs, const NFA &rhs, int n);

	void outputHeader();
	void outputImpl();

	/* Prefix for the names to be printed (class name, filenames, etc) */
	std::string prefix;

	/* Class name for resulting files */
	std::string className;

	/* Include-guard name for resulting files */
	std::string guardName;

	/* Streams for the header file */
	std::ofstream foutHpp; /* only set if we're writing to a file */
	std::ostream* outHpp = &std::cout;

	/* Streams for the implementation file */
	std::ofstream foutCpp; /* only set if we're writing to a file */
	std::ostream* outCpp = &std::cout;

	/* NFAs to be printed */
	const NFAs *res;
};

#endif /* _KATER_PRINTER_HPP_ */
