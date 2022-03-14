#include "Constraint.hpp"

std::unique_ptr<Constraint>
EmptyConstraint::createOpt(std::unique_ptr<RegExp> re)
{
	/* Convert `A \ B = 0` to `A <= B` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*re))
		return SubsetConstraint::createOpt(minusRE->releaseKid(0),
						   minusRE->releaseKid(1));

	return create(std::move(re));
}

bool EmptyConstraint::checkStatically() const
{
	return getKid(0)->toNFA().acceptsNoString();
}

std::unique_ptr<Constraint>
SubsetConstraint::createOpt(std::unique_ptr<RegExp> lhs,
			    std::unique_ptr<RegExp> rhs)
{
	/* Convert `A \ B <= C` to `A <= B | C` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*lhs)) {
		return SubsetConstraint::createOpt(minusRE->releaseKid(0),
						   AltRE::create(std::move(rhs), minusRE->releaseKid(1)));
	}

	return create(std::move(lhs), std::move(rhs));
}

bool SubsetConstraint::checkStatically() const
{
	auto lhs = getKid(0)->toNFA().to_DFA().first;
	auto rhs = getKid(1)->toNFA().to_DFA().first;
	return lhs.isSubLanguageOfDFA(rhs);
}

std::unique_ptr<Constraint>
EqualityConstraint::createOpt(std::unique_ptr<RegExp> lhs,
			      std::unique_ptr<RegExp> rhs)
{
	if (lhs->isFalse())
		return EmptyConstraint::createOpt(std::move(rhs));
	if (rhs->isFalse())
		return EmptyConstraint::createOpt(std::move(lhs));

	return create(std::move(lhs), std::move(rhs));
}

bool EqualityConstraint::checkStatically() const
{
	auto lhs = getKid(0)->toNFA().to_DFA().first;
	auto rhs = getKid(1)->toNFA().to_DFA().first;
	return lhs.isSubLanguageOfDFA(rhs) &&
	       rhs.isSubLanguageOfDFA(lhs);
}

std::unique_ptr<Constraint>
AcyclicConstraint::createOpt(std::unique_ptr<RegExp> re)
{
	/* Convert `acyclic (re+)` to `acyclic (re)` */
	if (auto *plusRE = dynamic_cast<PlusRE *>(&*re))
		return AcyclicConstraint::createOpt(plusRE->releaseKid(0));

	return create(std::move(re));
}

std::unique_ptr<Constraint>
CoherenceConstraint::createOpt(std::unique_ptr<RegExp> re)
{
	return create(std::move(re));
}
