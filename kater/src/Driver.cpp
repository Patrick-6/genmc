#include "Driver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

Driver::Driver() {
	int preds = builtinPredicates.size();
	int rels = builtinRelations.size();
	for (int i = 0; i < preds; i++)
		registerID(builtinPredicates[i].name, PredRE::create(i));
	for (int i = 0; i < rels; i++)
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

void Driver::expandSavedVars(std::unique_ptr<RegExp> &r)
{
	for (int i = 0; i < r->getNumKids(); i++)
		expandSavedVars(r->getKid(i));
	if (auto *re = dynamic_cast<RelRE *>(&*r)) {
		if (re->getLabel().isBuiltin())
			return;
		r = std::move(savedVariables[re->getLabel().getCalcIndex()].first->clone());
		expandSavedVars(r);
	}
}

void Driver::registerErrorUnless(std::string &s, std::unique_ptr<Constraint> c, const yy::location &loc)
{
// TODO
	std::cerr << loc << ": [Warning] Ignoring unsupported error constraint " << *c << std::endl;
}

void Driver::registerWarningUnless(std::string &s, std::unique_ptr<Constraint> c, const yy::location &loc)
{
// TODO
	std::cerr << loc << ": [Warning] Ignoring unsupported warning constraint " << *c << std::endl;
}

void Driver::registerAssert(std::unique_ptr<Constraint> c, const yy::location &loc)
{
	if (config.verbose > 0)
		std::cout << "Checking assertion " << *c << std::endl;
	for (int i = 0; i < c->getNumKids(); i++)
		expandSavedVars(c->getKid(i));
	std::string cex;
	if (!c->checkStatically(cex)) {
		std::cerr << loc << ": [Error] Assertion does not hold." << std::endl;
		if (!cex.empty()) std::cerr << "Counterexample: " << cex << std::endl;

	}
}

void Driver::registerAssume(std::unique_ptr<Constraint> c, const yy::location &loc)
{
//	if (auto *empC = dynamic_cast<EmptyConstraint *>(&*c)) {
//		auto *charRE = dynamic_cast<const CharRE *>(empC->getKid(0));
//		if (!charRE) {
//			std::cerr << loc << ": [Warning] Ignoring the unsupported assumption "
//				  << *empC->getKid(0) << std::endl;
//			return;
//		}
//		std::cout << "Registering assumption " << *empC->getKid(0) << "." << std::endl;
//		TransLabel::register_invalid(charRE->getLabel());
//	}
}

void Driver::addConstraint(std::unique_ptr<Constraint> c, const yy::location &loc)
{
	if (auto *acycC = dynamic_cast<AcyclicConstraint *>(&*c)) {
		acyclicityConstraints.push_back(acycC->getKid(0)->clone());
		return;
	}
	std::cerr << loc << ": [Warning] Ignoring the unsupported constraint " << *c << std::endl;
}

void Driver::generate_NFAs (NFAs &res)
{
	for (auto i = 0u; i < savedVariables.size(); i++) {
		auto &v = savedVariables[i];
		if (config.verbose > 0)
			std::cout << "Generating NFA for save[" << i << "] = "
				  << *v.first << std::endl;
		NFA n = v.first->toNFA();
		if (v.second == VarStatus::Reduce)
			n.simplifyReduce();
		else
			n.simplify();
		if (config.verbose > 0)
			std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
		res.nsaved.push_back({std::move(n), v.second});
	}

        for (auto &r : acyclicityConstraints) {
		if (config.verbose > 0)
			std::cout << "Generating NFA for acyclic " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		n.star();
		if (config.verbose > 1)
			std::cout << "Non-simplified NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify();
		if (config.verbose > 0 && acyclicityConstraints.size() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		res.nfa_acyc.alt(std::move(n));
        }
	if (config.verbose > 0)
		std::cout << "Generated NFA: " << res.nfa_acyc << std::endl;
}
