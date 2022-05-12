#include "Constraint.hpp"
#include "Saturation.hpp"

std::unique_ptr<Constraint> Constraint::createEmpty(std::unique_ptr<RegExp> re)
{
	return SubsetConstraint::createOpt(std::move(re), RegExp::createFalse());
}

bool Constraint::isEmpty() const
{
	auto *sc = dynamic_cast<const SubsetConstraint *>(this);
	auto *ec = dynamic_cast<const EqualityConstraint *>(this);
	return (sc || ec) && getNumKids() == 2 && getKid(1)->isFalse();
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

void saturateNFA(NFA &nfa)
{
	auto i = 0; // FIXME: Implicit idx-based ID is horrible.
	std::for_each(TransLabel::builtin_pred_begin(), TransLabel::builtin_pred_end(), [&](auto &pi){
		saturateIDs(nfa, TransLabel(std::nullopt, {i++}));
	});
}

bool checkStaticInclusion(const RegExp *re1, const RegExp *re2, std::string &cex)
{
	auto nfa1 = re1->toNFA();
	nfa1.breakToParts();
	auto lhs = nfa1.to_DFA().first;

	auto nfa2 = re2->toNFA();
	nfa2.breakToParts();
	saturateNFA(nfa2);
	auto rhs = nfa2.to_DFA().first;
	return lhs.isSubLanguageOfDFA(rhs, cex);
}

bool SubsetConstraint::checkStatically(std::string &cex) const
{
	return checkStaticInclusion(getKid(0), getKid(1), cex);
}

std::unique_ptr<Constraint>
EqualityConstraint::createOpt(std::unique_ptr<RegExp> lhs,
			      std::unique_ptr<RegExp> rhs)
{
	if (lhs->isFalse())
		return Constraint::createEmpty(std::move(rhs));
	if (rhs->isFalse())
		return Constraint::createEmpty(std::move(lhs));
	return create(std::move(lhs), std::move(rhs));
}

bool EqualityConstraint::checkStatically(std::string &cex) const
{
	return checkStaticInclusion(getKid(0), getKid(1), cex) &&
		checkStaticInclusion(getKid(1), getKid(0), cex);
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
