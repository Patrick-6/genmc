#ifndef _KATER_PRINTER_HPP_
#define _KATER_PRINTER_HPP_

#include "Driver.hpp"
#include "NFA.hpp"
#include <string>
#include <vector>

class Printer {
public:
	Printer(const NFAs *arg);
	void output();

private:
	const NFAs *res;
	std::string className;
	std::string guardName;

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
};

#endif /* _KATER_PRINTER_HPP_ */
