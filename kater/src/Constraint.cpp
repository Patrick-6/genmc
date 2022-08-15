#include "Constraint.hpp"
#include "Saturation.hpp"

std::unique_ptr<Constraint> Constraint::createEmpty(std::unique_ptr<RegExp> re)
{
	return SubsetConstraint::createOpt(std::move(re), RegExp::createFalse());
}

bool Constraint::isEmpty() const
{
	return dynamic_cast<const SubsetConstraint *>(this) && getNumKids() == 2 && getKid(1)->isFalse();
}

std::unique_ptr<Constraint>
Constraint::createEquality(std::unique_ptr<RegExp> lhs,
			   std::unique_ptr<RegExp> rhs)
{
	if (lhs->isFalse())
		return createEmpty(std::move(rhs));
	if (rhs->isFalse())
		return createEmpty(std::move(lhs));
	auto c1 = SubsetConstraint::createOpt(lhs->clone(), rhs->clone());
	auto c2 = SubsetConstraint::createOpt(std::move(rhs), std::move(lhs));
	return ConjunctiveConstraint::create(std::move(c1), std::move(c2));
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
	/* Convert `A ; A <= A` to `transitive A` */
	if (auto *seqRE = dynamic_cast<SeqRE *>(&*lhs)) {
		if (*rhs == *seqRE->getKid(0) && *rhs == *seqRE->getKid(1))
			return TransitivityConstraint::createOpt(std::move(rhs));
	}
	/* Convert `A+ <= A` to `transitive A` */
	if (auto *plusRE = dynamic_cast<PlusRE *>(&*lhs)) {
		if (*rhs == *plusRE->getKid(0))
			return TransitivityConstraint::createOpt(std::move(rhs));
	}
	/* Convert `A & id <= B` to `SubsetSameEnds(A,B)` */
	if (auto *andRE = dynamic_cast<AndRE *>(&*lhs)) {
		if (*andRE->getKid(1) == *RegExp::createId())
			return SubsetSameEndsConstraint::createOpt(andRE->releaseKid(0), std::move(rhs));
	}

	return create(std::move(lhs), std::move(rhs));
}

std::unique_ptr<Constraint>
SubsetSameEndsConstraint::createOpt(std::unique_ptr<RegExp> lhs,
				    std::unique_ptr<RegExp> rhs)
{
	/* Convert `A \ B <=&id C` to `A <=&id B | C` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*lhs)) {
		return SubsetSameEndsConstraint::createOpt(
			minusRE->releaseKid(0), AltRE::create(std::move(rhs), minusRE->releaseKid(1)));
	}
	/* Convert `A <=&id B & id` to `A <=&id B` */
	if (auto *andRE = dynamic_cast<AndRE *>(&*rhs)) {
		if (*andRE->getKid(1) == *RegExp::createId())
			return SubsetSameEndsConstraint::createOpt(std::move(lhs), andRE->releaseKid(0));
	}
	return create(std::move(lhs), std::move(rhs));
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
