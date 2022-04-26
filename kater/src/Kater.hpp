#ifndef __KATER_HPP__
#define __KATER_HPP__

#include "CNFAs.hpp"
#include "KatModule.hpp"

/*
 * Given a KatModule resulted from parsing, transforms all regexps to
 * NFAs that will be used for code printing
 */
class Kater {

public:
	Kater(std::unique_ptr<KatModule> mod) : module(std::move(mod)) {}

	const KatModule &getModule() const { return *module; }

	/* Check any user assertions and report errors.
	 * Returns whether any assertion was violated */
	bool checkAssertions();

	void generateNFAs();
	void exportCode(std::string &dirPrefix, std::string &outPrefix);

private:
	CNFAs &getCNFAs() { return cnfas; }

	void expandSavedVars(URE &r);

	std::unique_ptr<KatModule> module;

	CNFAs cnfas;
};

#endif /* __KATER_HPP__ */
