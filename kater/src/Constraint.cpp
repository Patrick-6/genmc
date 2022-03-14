#include "Constraint.hpp"

std::unique_ptr<Constraint>
EmptyConstraint::createOpt(std::unique_ptr<RegExp> re)
{
	auto res = RegExp::optimize(std::move(re));

	/* Convert `A \ B = 0` to `A <= B` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*res))
		return SubsetConstraint::createOpt(minusRE->releaseKid(0),
						   minusRE->releaseKid(1));

	return create(std::move(res));
}

std::unique_ptr<Constraint>
SubsetConstraint::createOpt(std::unique_ptr<RegExp> e1,
			    std::unique_ptr<RegExp> e2)
{
	auto lhs = RegExp::optimize(std::move(e1));
	auto rhs = RegExp::optimize(std::move(e2));

	/* Convert `A \ B <= C` to `A <= B | C` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*lhs)) {
		return SubsetConstraint::createOpt(minusRE->releaseKid(0),
						   AltRE::create(std::move(rhs), minusRE->releaseKid(1)));
	}

	return create(std::move(lhs), std::move(rhs));
}

std::unique_ptr<Constraint>
AcyclicConstraint::createOpt(std::unique_ptr<RegExp> exp)
{
	auto re = RegExp::optimize(std::move(exp));

	/* Convert `acyclic (re+)` to `acyclic (re)` */
	if (auto *plusRE = dynamic_cast<PlusRE *>(&*re))
		return AcyclicConstraint::createOpt(plusRE->releaseKid(0));

	return create(std::move(re));
}

std::unique_ptr<Constraint>
CoherenceConstraint::createOpt(std::unique_ptr<RegExp> exp)
{
	auto re = RegExp::optimize(std::move(exp));

	return create(std::move(re));
}
