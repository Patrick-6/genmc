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

struct Path {
	Path(NFA::State *s) : s(s), ts() {}

	NFA::State *s;
	std::vector<NFA::Transition> ts;
};

void dfsFindPathsFrom(NFA::State *s, Path p,
		      const NFA &pattern, NFA::State *ps,
		      std::vector<Path> &collected)
{
	if (pattern.isAccepting(ps)) {
		if (!p.ts.empty())
			collected.push_back(std::move(p));
		return;
	}

	std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
		std::for_each(ps->out_begin(), ps->out_end(), [&](auto &pt){
			if (t.label == pt.label) {
				Path p1(p);
				p1.ts.push_back(t);
				dfsFindPathsFrom(t.dest, p1, pattern, pt.dest, collected);
			}
		});
	});
}

std::vector<Path>
findAllMatchingPaths(const NFA &nfa, const NFA &pattern)
{
	std::vector<Path> result;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		dfsFindPathsFrom(&*s, Path(&*s), pattern, *pattern.start_begin(), result);
	});
	return result;
}

void expandAssumption(NFA &nfa, const std::unique_ptr<Constraint> &assm)
{
	assert(&*assm);
	auto *ec = dynamic_cast<SubsetConstraint *>(&*assm);
	if (!ec) {
		std::cout << "Ignoring unsupported local assumption " << *assm << "\n";
		return;
	}

	auto *lRE = &*ec->getKid(0);
	assert(typeid(*lRE) == typeid(CharRE) ||
	       (typeid(*lRE) == typeid(SeqRE) &&
		std::all_of(lRE->kid_begin(), lRE->kid_end(), [&](auto &k){
				return typeid(*k) == typeid(CharRE);
			})));
	auto lNFA = lRE->toNFA();
	// lNFA.simplify();
	lNFA.breakToParts();

	auto rNFA = ec->getKid(1)->toNFA();
	rNFA.simplify();

	std::vector<NFA::State *> inits(lNFA.start_begin(), lNFA.start_end());
	std::vector<NFA::State *> finals(lNFA.accept_begin(), lNFA.accept_end());

	auto paths = findAllMatchingPaths(nfa, rNFA);

	lNFA.clearAllAccepting();
	lNFA.clearAllStarting();

	nfa.alt(std::move(lNFA));
	std::for_each(paths.begin(), paths.end(), [&](auto &p){
		std::for_each(inits.begin(), inits.end(), [&](auto *i){
			nfa.addEpsilonTransitionSucc(p.s, i);
		});
		std::for_each(finals.begin(), finals.end(), [&](auto *f){
			nfa.addEpsilonTransitionSucc(f, p.ts.back().dest);
		});
	});
}

void pruneNFA(NFA &nfa, const NFA &other)
{
	std::unordered_set<Relation, RelationHasher> orels;
	std::vector<PredicateSet> opreds;
	std::for_each(other.states_begin(), other.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			if (!t.label.isPredicate())
				orels.insert(*t.label.getRelation());
			else
				opreds.push_back(t.label.getPreChecks());
		});
	});

	std::sort(opreds.begin(), opreds.end());
	opreds.erase(std::unique(opreds.begin(), opreds.end()), opreds.end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		nfa.removeTransitionsIf(&*s, [&](auto &t){
			return (!t.label.isPredicate() &&
				!orels.count(*t.label.getRelation())) ||
				(t.label.isPredicate() &&
				 !std::binary_search(opreds.begin(), opreds.end(), t.label.getPreChecks()));
		});
	});
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

	std::sort(opreds.begin(), opreds.end());
	opreds.erase(std::unique(opreds.begin(), opreds.end()), opreds.end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::vector<NFA::Transition> toAdd;
		std::for_each(s->out_begin(), s->out_end(), [&](const auto &t){
			if (!t.label.isPredicate())
				return;
			std::for_each(opreds.begin(), opreds.end(), [&](const auto &lab){
				if (t.label.getPreChecks().includes(lab.getPreChecks()))
					toAdd.push_back(NFA::Transition(lab, t.dest));
			});
		});
		nfa.addTransitions(&*s, toAdd.begin(), toAdd.end());
	});

	saturateID(nfa, opreds);
}

void removeConsecutivePredicates(NFA &nfa)
{
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		nfa.removeTransitionsIf(&*s, [&](auto &t){
			return t.dest == &*s && t.label.isPredicate();
		});
	});

	/* Collect states w/ incoming both preds + rels */
	std::vector<NFA::State *> toDuplicate;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		if (std::any_of(s->in_begin(), s->in_end(), [&](auto &t){
					return t.label.isPredicate();
				}) &&
			std::any_of(s->in_begin(), s->in_end(), [&](auto &t){
					return !t.label.isPredicate();
				}))
			toDuplicate.push_back(&*s);
	});

	std::for_each(toDuplicate.begin(), toDuplicate.end(), [&](auto &s){
		auto *d = nfa.createState();
		if (nfa.isAccepting(s))
			nfa.makeAccepting(d);
		nfa.addTransitions(d, s->out_begin(), s->out_end());
		nfa.addInvertedTransitions(d, s->in_begin(), s->in_end());
		nfa.removeInvertedTransitionsIf(d, [&](auto &t){ return t.label.isPredicate(); });
		nfa.removeInvertedTransitionsIf(s, [&](auto &t){ return !t.label.isPredicate(); });
	});

	nfa.addTransitivePredicateEdges();

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		nfa.removeTransitionsIf(&*s, [&](auto &t){
			return t.dest == &*s && t.label.isPredicate();
		});
	});
	return;
}

bool checkStaticInclusion(const RegExp *re1, const RegExp *re2,
			  const std::vector<std::unique_ptr<Constraint>> &assms,
			  std::string &cex, Constraint::ValidFunT vfun)
{
	auto nfa1 = re1->toNFA();
	nfa1.simplify(vfun);
	saturateDomains(nfa1);
	nfa1.breakToParts();
	nfa1.removeDeadStates();
	removeConsecutivePredicates(nfa1);
	nfa1.removeDeadStates();

	auto lhs = nfa1.to_DFA().first;
	// std::cerr << "LHS " << lhs << "\n";

	auto nfa2 = re2->toNFA();
	if (!assms.empty()) {
		std::for_each(assms.begin(), assms.end(), [&](auto &assm){
			expandAssumption(nfa2, assm);
			nfa2.breakToParts();
			nfa2.removeDeadStates();
		});
	}
	nfa2.simplify(vfun);
	nfa2.breakToParts();
	nfa2.removeDeadStates();
	removeConsecutivePredicates(nfa2);
	nfa2.removeDeadStates();
	saturateNFA(nfa2, nfa1);
	pruneNFA(nfa2, nfa1);

	// std::cerr << "RHS/sat" << nfa2 << "\n";
	auto rhs = nfa2.to_DFA().first;
	// std::cerr << "RHS " << rhs << "\n";
	return lhs.isSubLanguageOfDFA(rhs, cex, vfun);
}

bool SubsetConstraint::checkStatically(const std::vector<std::unique_ptr<Constraint>> &assms,
				       std::string &cex, Constraint::ValidFunT vfun) const
{
	return checkStaticInclusion(getKid(0), getKid(1), assms, cex, vfun);
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

bool EqualityConstraint::checkStatically(const std::vector<std::unique_ptr<Constraint>> &assms,
					 std::string &cex, Constraint::ValidFunT vfun) const
{
	return checkStaticInclusion(getKid(0), getKid(1), assms, cex, vfun) &&
		checkStaticInclusion(getKid(1), getKid(0), assms, cex, vfun);
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
