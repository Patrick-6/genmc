#include "RegExp.hpp"

std::unique_ptr<RegExp>
RegExp::optimize(std::unique_ptr<RegExp> re)
{
	if (!re->hasKids())
		return re;

	for (auto i = 0u; i < re->getNumKids(); i++)
		re->setKid(i, optimize(std::move(re->kids[i])));

	/* The only expression we actually optimize is SeqRE */
	if (auto *seqRE = dynamic_cast<const SeqRE *>(&*re))
		return SeqRE::createOpt(std::move(re->kids[0]), std::move(re->kids[1]));
	return re;
}

std::unique_ptr<Constraint>
EmptyConstraint::create(std::unique_ptr<RegExp> re)
{
	auto res = RegExp::optimize(std::move(re));

	/* Convert `A \ B = 0` to `A <= B` */
	if (auto *minusRE = dynamic_cast<const MinusRE *>(&*res))
		return SubsetConstraint::create(std::move(res->kids[0]),
						std::move(res->kids[1]));

	return std::unique_ptr<Constraint>(new EmptyConstraint(std::move(res)));
}

std::unique_ptr<Constraint>
SubsetConstraint::create(std::unique_ptr<RegExp> e1,
			 std::unique_ptr<RegExp> e2)
{
	auto lhs = RegExp::optimize(std::move(e1));
	auto rhs = RegExp::optimize(std::move(e2));

	/* Convert `A \ B <= C` to `A <= B | C` */
	if (auto *minusRE = dynamic_cast<const MinusRE *>(&*lhs)) {
		auto new_rhs = AltRE::create(std::move(rhs), std::move(lhs->kids[1]));
		return SubsetConstraint::create(std::move(lhs->kids[0]), std::move(new_rhs));
	}

	return std::unique_ptr<Constraint>(new SubsetConstraint(std::move(lhs), std::move(rhs)));
}

std::unique_ptr<Constraint>
AcyclicConstraint::create(std::unique_ptr<RegExp> exp)
{
	auto re = RegExp::optimize(std::move(exp));

	/* Convert `acyclic (re+)` to `acyclic (re)` */
	if (auto *plusRE = dynamic_cast<const PlusRE *>(&*re)) {
		return AcyclicConstraint::create(std::move(re->kids[0]));
	}

	return std::unique_ptr<Constraint>(new AcyclicConstraint(std::move(re)));
}



