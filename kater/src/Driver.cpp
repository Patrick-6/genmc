#include "Driver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

Driver::Driver() : module(new KatModule)
{
	for (auto i = 0u; i < builtinPredicates.size(); i++)
		registerID(builtinPredicates[i].name, PredRE::create(i));
	for (auto i = 0u; i < builtinRelations.size(); i++)
		registerID(builtinRelations[i].name, RelRE::create(i));
}


int Driver::parse()
{
	extern FILE* yyin;

	if (config.verbose > 0)
		std::cout << "Parsing file " << config.inputFile << "...";

	if (config.inputFile.empty ()) {
		std::cerr << "no input file provided" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (!(yyin = fopen(config.inputFile.c_str(), "r"))) {
		std::cerr << "cannot open " << config.inputFile
			  << ": " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	location.initialize(&config.inputFile);

	yy::parser parser(*this);
	if (config.debug)
		parser.set_debug_level(2);

	auto res = parser.parse();

	fclose(yyin);

	if (config.verbose > 0)
		std::cout << "Done." << std::endl;
	return res;
}

void Driver::expandSavedVars(URE &r)
{
	for (int i = 0; i < r->getNumKids(); i++)
		expandSavedVars(r->getKid(i));
	if (auto *re = dynamic_cast<RelRE *>(&*r)) {
		if (re->getLabel().isBuiltin())
			return;
		r = module->getSavedID(re);
		expandSavedVars(r);
	}
}

void Driver::checkAssertions()
{
	std::for_each(module->assert_begin(), module->assert_end(), [&](auto &p){
		if (config.verbose > 0)
			std::cout << "Checking assertion " << *p.first << std::endl;
		for (int i = 0; i < p.first->getNumKids(); i++)
			expandSavedVars(p.first->getKid(i));
		std::string cex;
		if (!p.first->checkStatically(cex)) {
			std::cerr << p.second << ": [Error] Assertion does not hold." << std::endl;
			if (!cex.empty())
				std::cerr << "Counterexample: " << cex << std::endl;

		}
	});
}
