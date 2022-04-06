#include "Kater.hpp"
#include "Printer.hpp"

void Kater::generateNFAs()
{
	auto &module = getModule();
	auto &cnfas = getCNFAs();

	auto i = 0u;
	std::for_each(module.svar_begin(), module.svar_end(), [&](auto &v){
		NFA n = v.exp->toNFA();
		// FIXME: Use polymorphism
		if (v.status == VarStatus::Reduce) {
			if (config.verbose > 0)
				std::cout << "Generating NFA for reduce[" << i << "] = "
					  << *v.exp << std::endl;

			assert(v.red);
			n.simplify().reduce(v.redT);

			NFA rn = v.red->toNFA();
			rn.star().simplify().seq(std::move(n)).simplify();

			if (config.verbose > 0)
				std::cout << "Generated NFA for reduce[" << i << "]: " << rn << std::endl;
			cnfas.addReduced(std::move(rn));
		} else if (v.status == VarStatus::View) {
			if (config.verbose > 0)
				std::cout << "Generating NFA for view[" << i << "] = " << *v.exp << std::endl;
			n.simplify();

			if (config.verbose > 0)
				std::cout << "Generated NFA for view[" << i << "]: " << n << std::endl;
			cnfas.addView(std::move(n));
		} else {
			if (config.verbose > 0)
				std::cout << "Generating NFA for save[" << i << "] = "
					  << *v.exp << std::endl;
			n.simplify();
			if (config.verbose > 0)
				std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
			cnfas.addSaved(std::move(n));
		}
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

void Kater::exportCode(std::string &dirPrefix, std::string &outPrefix)
{
	generateNFAs();

	Printer p(dirPrefix, outPrefix);
	p.output(getCNFAs());
}
