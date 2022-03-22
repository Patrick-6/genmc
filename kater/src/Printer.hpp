#ifndef __KATER_PRINTER_HPP__
#define __KATER_PRINTER_HPP__

#include "Driver.hpp"
#include "CNFAs.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Printer {

public:
	Printer(const std::string &prefix);

	/* Outputs RES */
	void output(const CNFAs &cnfas);

private:
//	void printInclusionError(const std::string &s, const NFA &lhs, const NFA &rhs);
//	void printInclusionWarning(const std::string &s, const NFA &lhs, const NFA &rhs);

	void printPredLabel(std::ostream &ostr, const PredLabel *p, std::string res, std::string arg);
	void printRelLabel(std::ostream &ostr, const RelLabel *p, std::string res, std::string arg);
	void printTransLabel(const TransLabel *t, std::string res, std::string arg);

	void printCalculatorHpp(const NFA &nfa, unsigned id);
	void printCalculatorCpp(const NFA &nfa, unsigned id, VarStatus reduce);

	void printInclusionHpp(const NFA &lhs, const NFA &rhs, unsigned id);
	void printInclusionCpp(const NFA &lhs, const NFA &rhs, unsigned id);

	void printAcyclicHpp(const NFA &nfa);
	void printAcyclicCpp(const NFA &nfa);

	void printHppHeader();
	void printCppHeader();

	void printHppFooter();
	void printCppFooter();

	void outputHpp(const CNFAs &nfas);
	void outputCpp(const CNFAs &cnfas);

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
};

#endif /* __KATER_PRINTER_HPP__ */
