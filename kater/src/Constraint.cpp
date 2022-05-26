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

template<typename ITER>
bool checksInclude(ITER &&b1, ITER &&e1, ITER &&b2, ITER &&e2)
{
	unsigned mask1 = ~0;
	std::for_each(b1, e1, [&mask1](auto &id){
		mask1 &= builtinPredicates[id].bitmask;
	});
	unsigned mask2 = ~0;
	std::for_each(b2, e2, [&mask2](auto &id){
		mask2 &= builtinPredicates[id].bitmask;
	});
	return (mask1 | mask2) == mask1;
}

void saturateNFA(NFA &nfa, const NFA &other)
{
	std::vector<TransLabel> opreds;
	std::for_each(other.states_begin(), other.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			if (t.label.isPredicate())
				opreds.push_back(t.label);
		});
	});

	saturateID(nfa, opreds);

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::vector<NFA::Transition> toAdd;
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			if (!t.label.isPredicate())
				return;
			std::for_each(opreds.begin(), opreds.end(), [&](auto &lab){
				if (checksInclude(lab.pre_begin(), lab.pre_end(),
						  t.label.pre_begin(),t.label.pre_end())) {
					toAdd.push_back(NFA::Transition(lab, t.dest));
				}
			});
		});
		nfa.addTransitions(&*s, toAdd.begin(), toAdd.end());
	});
}

bool checkStaticInclusion(const RegExp *re1, const RegExp *re2, std::string &cex, Constraint::ValidFunT vfun)
{
	auto nfa1 = re1->toNFA();
	nfa1.simplify();
	nfa1.breakToParts();
	nfa1.removeDeadStates();
	auto lhs = nfa1.to_DFA().first;

	auto nfa2 = re2->toNFA();
	nfa2.simplify();
	nfa2.breakToParts();
	nfa2.removeDeadStates();
	saturateNFA(nfa2, nfa1);
	nfa2.addTransitivePredicateEdges();
	auto rhs = nfa2.to_DFA().first;
	return lhs.isSubLanguageOfDFA(rhs, cex, vfun);
}

bool SubsetConstraint::checkStatically(std::string &cex, Constraint::ValidFunT vfun) const
{
	return checkStaticInclusion(getKid(0), getKid(1), cex, vfun);
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

bool EqualityConstraint::checkStatically(std::string &cex, Constraint::ValidFunT vfun) const
{
	return checkStaticInclusion(getKid(0), getKid(1), cex, vfun) &&
		checkStaticInclusion(getKid(1), getKid(0), cex, vfun);
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
