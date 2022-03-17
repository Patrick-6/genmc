#include "KatModule.hpp"

void KatModule::registerAssume(UCO c, const yy::location &loc)
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

void KatModule::addConstraint(UCO c, const std::string &s, const yy::location &loc)
{

	if (dynamic_cast<AcyclicConstraint *>(&*c) && c->getType() == Constraint::Type::Consistency)
		acyclicityConstraints.push_back(c->getKid(0)->clone());
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
