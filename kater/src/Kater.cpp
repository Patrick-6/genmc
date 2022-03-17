#include "Kater.hpp"
#include "Printer.hpp"

void Kater::generateNFAs()
{
	auto &module = getModule();
	auto &cnfas = getCNFAs();

	auto i = 0u;
	std::for_each(module.svar_begin(), module.svar_end(), [&](auto &v){
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
		if (v.second == VarStatus::Reduce)
			cnfas.addReduced(std::move(n));
		else
			cnfas.addSaved(std::move(n));
		++i;
	});

	std::for_each(module.acyc_begin(), module.acyc_end(), [&](auto &r){
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
		if (config.verbose > 0 && module.getAcyclicNum() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		cnfas.addAcyclic(std::move(n));
	});
	if (config.verbose > 0)
		std::cout << "Generated NFA: " << cnfas.getAcyclic() << std::endl;
}

void Kater::exportCode(std::string &prefix)
{
	Printer p(config.outPrefix);
	p.output(getCNFAs());
}
