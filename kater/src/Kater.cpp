#include "Kater.hpp"
#include "Config.hpp"
#include "Printer.hpp"
#include <numeric>

void Kater::expandSavedVars(URE &r)
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

bool Kater::checkAssertions()
{
	/* Ensure that ppo is implied by the acyclicity constraints */
	auto ppo = module->getRegisteredID("ppo");
	if (!ppo) {
		std::cerr << "[Error] No ppo definition provided!\n";
		exit(EXIT_FAILURE);
	}
	auto pporf = PlusRE::createOpt(AltRE::createOpt(std::move(ppo), module->getRegisteredID("rfe")));
	auto acycDisj = std::accumulate(module->acyc_begin(), module->acyc_end(),
					RegExp::createFalse(), [&](URE &re1, URE &re2){
						return AltRE::createOpt(re1->clone(), re2->clone());
					});
	module->registerAssert(SubsetConstraint::create(std::move(pporf), std::move(acycDisj)), yy::location());

	bool status = true;
	std::for_each(module->assert_begin(), module->assert_end(), [&](auto &p){
		if (config.verbose >= 2)
			std::cout << "Checking assertion " << *p.first << std::endl;
		for (int i = 0; i < p.first->getNumKids(); i++)
			expandSavedVars(p.first->getKid(i));
		std::string cex;
		if (!p.first->checkStatically(cex)) {
			std::cerr << p.second << ": [Error] Assertion does not hold." << std::endl;
			if (!cex.empty())
				std::cerr << "Counterexample: " << cex << std::endl;
			status = false;
		}
	});
	return status;
}

void Kater::generateNFAs()
{
	auto &module = getModule();
	auto &cnfas = getCNFAs();

	auto i = 0u;
	std::for_each(module.svar_begin(), module.svar_end(), [&](auto &v){
		NFA n = v.exp->toNFA();
		// FIXME: Use polymorphism
		if (v.status == VarStatus::Reduce) {
			if (config.verbose >= 3)
				std::cout << "Generating NFA for reduce[" << i << "] = "
					  << *v.exp << std::endl;

			assert(v.red);
			n.simplify().reduce(v.redT);

			NFA rn = v.red->toNFA();
			rn.star().simplify().seq(std::move(n)).simplify();

			if (config.verbose >= 3)
				std::cout << "Generated NFA for reduce[" << i << "]: " << rn << std::endl;
			cnfas.addReduced(std::move(rn));
		} else if (v.status == VarStatus::View) {
			if (config.verbose >= 3)
				std::cout << "Generating NFA for view[" << i << "] = " << *v.exp << std::endl;
			n.simplify();

			if (config.verbose >= 3)
				std::cout << "Generated NFA for view[" << i << "]: " << n << std::endl;
			cnfas.addView(std::move(n));
		} else {
			if (config.verbose >= 3)
				std::cout << "Generating NFA for save[" << i << "] = "
					  << *v.exp << std::endl;
			n.simplify();
			if (config.verbose >= 3)
				std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
			cnfas.addSaved(std::move(n));
		}
		++i;
	});

	std::for_each(module.acyc_begin(), module.acyc_end(), [&](auto &r){
		if (config.verbose >= 3)
			std::cout << "Generating NFA for acyclic " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		n.star();
		if (config.verbose >= 4)
			std::cout << "Non-simplified NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify();
		if (config.verbose >= 3 && module.getAcyclicNum() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		cnfas.addAcyclic(std::move(n));
	});
	if (config.verbose >= 3)
		std::cout << "Generated NFA: " << cnfas.getAcyclic() << std::endl;
}

void Kater::exportCode(std::string &dirPrefix, std::string &outPrefix)
{
	generateNFAs();

	Printer p(dirPrefix, outPrefix);
	p.output(getCNFAs());
}
