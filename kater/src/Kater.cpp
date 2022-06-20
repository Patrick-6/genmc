#include "Kater.hpp"
#include "Config.hpp"
#include "Printer.hpp"
#include <numeric>

void Kater::expandSavedVars(URE &r)
{
	for (int i = 0; i < r->getNumKids(); i++)
		expandSavedVars(r->getKid(i));
	if (auto *re = dynamic_cast<CharRE *>(&*r)) {
		if (re->getLabel().isBuiltin() || !module->isSavedID(re))
			return;
		r = module->getSavedID(re);
		expandSavedVars(r);
	}
}

void Kater::expandRfs(URE &r)
{
	for (int i = 0; i < r->getNumKids(); i++)
		expandRfs(r->getKid(i));

	auto rf = module->getRegisteredID("rf");
	auto *re = dynamic_cast<const CharRE *>(&*r);
	if (!re || re->getLabel().getRelation() != static_cast<const CharRE *>(&*rf)->getLabel().getRelation())
		return;

	auto rfe = module->getRegisteredID("rfe");
	auto rfi = module->getRegisteredID("rfi");

	auto re1 = re->clone();
	auto re2 = re->clone();
	auto l1 = TransLabel(static_cast<CharRE *>(&*rfe)->getLabel().getRelation(),
			     re->getLabel().getPreChecks(),
			     re->getLabel().getPostChecks());
	auto l2 = TransLabel(static_cast<CharRE *>(&*rfi)->getLabel().getRelation(),
			     re->getLabel().getPreChecks(),
			     re->getLabel().getPostChecks());
	static_cast<CharRE *>(&*re1)->setLabel(l1);
	static_cast<CharRE *>(&*re2)->setLabel(l2);

	r = AltRE::createOpt(std::move(re1), std::move(re2));
}

// void expandMos(KatModule *module, URE &r)
// {
// 	for (int i = 0; i < r->getNumKids(); i++)
// 		expandMos(module, r->getKid(i));

// 	auto rf = module->getRegisteredID("mo-imm");
// 	auto *re = dynamic_cast<const CharRE *>(&*r);
// 	if (!re || re->getLabel().getId() != static_cast<const CharRE *>(&*rf)->getLabel().getId())
// 		return;

// 	auto rfe = module->getRegisteredID("moe");
// 	auto rfi = module->getRegisteredID("moi");

// 	auto re1 = re->clone();
// 	auto re2 = re->clone();
// 	auto l1 = TransLabel(static_cast<CharRE *>(&*rfe)->getLabel().getId(),
// 			     re->getLabel().getPreChecks(),
// 			     re->getLabel().getPostChecks());
// 	auto l2 = TransLabel(static_cast<CharRE *>(&*rfi)->getLabel().getId(),
// 			     re->getLabel().getPreChecks(),
// 			     re->getLabel().getPostChecks());
// 	static_cast<CharRE *>(&*re1)->setLabel(l1);
// 	static_cast<CharRE *>(&*re2)->setLabel(l2);

// 	r = AltRE::createOpt(std::move(re1), std::move(re2));
// }

bool Kater::checkAssertions()
{
	auto isValidLabel = [&](auto &lab){ return true; };

	bool status = true;
	std::for_each(module->assert_begin(), module->assert_end(), [&](auto &p){
		if (getConf().verbose >= 2)
			std::cout << "Checking assertion " << *p.co << std::endl;
		for (int i = 0; i < p.co->getNumKids(); i++) {
			expandSavedVars(p.co->getKid(i));
			expandRfs(p.co->getKid(i));
			// expandMos(&*module, p.co->getKid(i));
		}

		std::string cex;
		if (!p.co->checkStatically(module->getAssumes(), cex, isValidLabel)) {
			std::cerr << p.loc << ": [Error] Assertion does not hold." << std::endl;
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

	/* Ensure that ppo is implied by the acyclicity constraints */
	auto ppo = module.getPPO();
	if (!ppo) {
		std::cerr << "[Error] No top-level ppo definition provided!\n";
		exit(EXIT_FAILURE);
	}
	auto pporf = PlusRE::createOpt(AltRE::createOpt(ppo->clone(), module.getRegisteredID("rf")));
	auto acycDisj = std::accumulate(module.acyc_begin(), module.acyc_end(),
					RegExp::createFalse(), [&](URE &re1, URE &re2){
						return AltRE::createOpt(re1->clone(), re2->clone());
					});
	module.registerAssert(SubsetConstraint::create(pporf->clone(), StarRE::createOpt(std::move(acycDisj))),
			       yy::location());

	/* Ensure that all saved relations are included in pporf and are transitive */
	std::for_each(module.svar_begin(), module.svar_end(), [&](auto &kv){
			auto &sv = kv.second;
			module.registerAssert(
				SubsetConstraint::create(
					sv.exp->clone(),
					StarRE::createOpt(SeqRE::createOpt(StarRE::createOpt(pporf->clone()),
									   ppo->clone()))),
				yy::location());

			if (sv.red) {
				auto seqExp = SeqRE::createOpt(sv.red->clone(), sv.exp->clone());
				module.registerAssert(SubsetConstraint::createOpt(std::move(seqExp),
										  sv.exp->clone()), yy::location());
			}
	});

	auto isValidLabel = [&](auto &lab){ return true; };

	auto i = 0u;
	std::for_each(module.svar_begin(), module.svar_end(), [&](auto &kv){
		auto &v = kv.second;
		NFA n = v.exp->toNFA();
		// FIXME: Use polymorphism
		if (v.status == VarStatus::Reduce) {
			if (getConf().verbose >= 3)
				std::cout << "Generating NFA for reduce[" << i << "] = "
					  << *v.exp << std::endl;

			assert(v.red);
			n.simplify(isValidLabel).reduce(v.redT);

			NFA rn = v.red->toNFA();
			rn.star().simplify(isValidLabel).seq(std::move(n)).simplify(isValidLabel);

			if (getConf().verbose >= 3)
				std::cout << "Generated NFA for reduce[" << i << "]: " << rn << std::endl;
			cnfas.addReduced(std::move(rn));
		} else if (v.status == VarStatus::View) {
			if (getConf().verbose >= 3)
				std::cout << "Generating NFA for view[" << i << "] = " << *v.exp << std::endl;
			n.simplify(isValidLabel);

			if (getConf().verbose >= 3)
				std::cout << "Generated NFA for view[" << i << "]: " << n << std::endl;
			cnfas.addView(std::move(n));
		} else {
			if (getConf().verbose >= 3)
				std::cout << "Generating NFA for save[" << i << "] = "
					  << *v.exp << std::endl;
			n.simplify(isValidLabel);
			if (getConf().verbose >= 3)
				std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
			cnfas.addSaved(std::move(n));
		}
		++i;
	});

	std::for_each(module.acyc_begin(), module.acyc_end(), [&](auto &r){
		if (getConf().verbose >= 3)
			std::cout << "Generating NFA for acyclic " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		n.star();
		if (getConf().verbose >= 4)
			std::cout << "Non-simplified NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify(isValidLabel);
		if (getConf().verbose >= 3 && module.getAcyclicNum() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		cnfas.addAcyclic(std::move(n));
	});
	if (getConf().verbose >= 3)
		std::cout << "Generated NFA: " << cnfas.getAcyclic() << std::endl;

	NFA rec;
	std::for_each(module.rec_begin(), module.rec_end(), [&](auto &r){
		if (getConf().verbose >= 3)
			std::cout << "Generating NFA for recovery " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		if (getConf().verbose >= 4)
			std::cout << "Non-star, non-simplified rec NFA: " << n << std::endl;
		n.star();
		if (getConf().verbose >= 4)
			std::cout << "Non-simplified rec NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify(isValidLabel);
		if (getConf().verbose >= 3)
			std::cout << "Generated rec NFA: " << n << std::endl;
		rec.alt(std::move(n));
	});
	if (module.getRecoveryNum()) {
		auto rf = module.getRegisteredID("rf");
		auto recov = module.getRegisteredID("REC");
		auto po = module.getRegisteredID("po");
		auto fr = module.getRegisteredID("fr");
		auto poInv = module.getRegisteredID("po");
		poInv->flip();

		auto rfRecovPoFr = SeqRE::createOpt(rf->clone(), recov->clone(),
						    po->clone(), fr->clone());

		auto rfRecovPoInvFr = SeqRE::createOpt(rf->clone(), recov->clone(),
						       poInv->clone(), fr->clone());

		rec.alt(rfRecovPoFr->toNFA());
		rec.alt(rfRecovPoInvFr->toNFA());

		rec.star();

		if (getConf().verbose >= 3)
			std::cout << "Generated full rec NFA: " << rec << std::endl;

		rec.simplify(isValidLabel);

		cnfas.addRecovery(std::move(rec));
		if (getConf().verbose >= 3)
			std::cout << "Generated full rec NFA simplified: " << cnfas.getRecovery() << std::endl;
	}

	auto rf = module.getRegisteredID("rfe");
	auto tc = module.getRegisteredID("tc");
	auto tj = module.getRegisteredID("tj");
	auto pporfNFA = StarRE::createOpt(AltRE::createOpt(ppo->clone(), std::move(rf),
							   std::move(tc), std::move(tj)))->toNFA();
	pporfNFA.simplify(isValidLabel);
	cnfas.addPPoRf(std::move(pporfNFA), *ppo != *module.getRegisteredID("po"));
	if (getConf().verbose >= 3)
		std::cout << "Generated pporf NFA simplified: " << cnfas.getPPoRf().first << std::endl;
}

void Kater::exportCode(std::string &dirPrefix, std::string &outPrefix)
{
	generateNFAs();

	Printer p(dirPrefix, outPrefix);
	p.output(getCNFAs());
}
