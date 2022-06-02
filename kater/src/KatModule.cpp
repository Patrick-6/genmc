#include "KatModule.hpp"

std::ostream &operator<<(std::ostream &s, const KatModule::DbgInfo &dbg)
{
	return s << dbg.filename << ":" << dbg.line;
}

void KatModule::registerAssume(UCO c)
{
	assert(c->isEmpty() && dynamic_cast<CharRE *>(&*c->getKid(0)));
	auto *charRE = dynamic_cast<CharRE *>(&*c->getKid(0));
	assumes.push_back(charRE->getLabel());
}

void KatModule::addConstraint(UCO c, const std::string &s, const yy::location &loc)
{

	if (dynamic_cast<AcyclicConstraint *>(&*c) && c->getType() == Constraint::Type::Consistency)
		acyclicityConstraints.push_back(c->getKid(0)->clone());
	if (dynamic_cast<RecoveryConstraint *>(&*c) && c->getType() == Constraint::Type::Consistency)
		recoveryConstraints.push_back(c->getKid(0)->clone());
	else if (dynamic_cast<SubsetConstraint *>(&*c))
		inclusionConstraints.push_back({c->getKid(0)->clone(),
					        c->getKid(1)->clone(), c->getType(), s});
	else if (dynamic_cast<EqualityConstraint *>(&*c)) {
		inclusionConstraints.push_back({c->getKid(0)->clone(),
					        c->getKid(1)->clone(), c->getType(), s});
		inclusionConstraints.push_back({c->getKid(1)->clone(),
					        c->getKid(0)->clone(), c->getType(), s});
	} else {
		std::cerr << loc << ": [Warning] Ignoring the unsupported constraint:" << *c << "\n";
	}
}
