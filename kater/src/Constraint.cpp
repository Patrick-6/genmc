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

std::unique_ptr<Constraint>
SubsetIDConstraint::createOpt(std::unique_ptr<RegExp> lhs,
			      std::unique_ptr<RegExp> rhs)
{
	/* Convert `A \ B <= C` to `A <= B | C` */
	if (auto *minusRE = dynamic_cast<MinusRE *>(&*lhs)) {
		return SubsetIDConstraint::createOpt(minusRE->releaseKid(0),
						     AltRE::create(std::move(rhs), minusRE->releaseKid(1)));
	}

	return create(std::move(lhs), std::move(rhs));
}


struct Path {
	Path(NFA::State *s, NFA::State *e,
	     const PredicateSet &f, const PredicateSet &l) : start(s), end(e), fst(f), lst(l) {}

	bool operator==(const Path &other) {
		return start == other.start && end == other.end && fst == other.fst && lst == other.lst;
	}
	bool operator<(const Path &other) {
		return start < other.start ||
		       (start == other.start && end < other.end) ||
			(start == other.start && end == other.end && fst < other.fst) ||
			(start == other.start && end == other.end && fst == other.fst && lst < other.lst);
	}

	NFA::State *start;
	NFA::State *end;
	PredicateSet fst;
	PredicateSet lst;
};

template<typename T>
inline void hash_combine(std::size_t& seed, std::size_t v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

bool hasMatchingPathDFS(const NFA &nfa1, const NFA &nfa2, PredicateSet &fst, PredicateSet &lst)
{
	struct SPair {
		NFA::State *s1;
		NFA::State *s2;

		bool operator==(const SPair &other) const {
			return s1 == other.s1 && s2 == other.s2;
		}
	};
	// XXX: FIXME
	struct SPairHasher {
		std::size_t operator()(SPair p) const {
			std::size_t hash = 0;
			hash_combine<unsigned>(hash, p.s1->getId());
			hash_combine<unsigned>(hash, p.s2->getId());
			return hash;
		}
	};

	struct SimState {
		NFA::State *s1;
		NFA::State *s2;
		std::optional<PredicateSet> first;
		std::optional<PredicateSet> last;
	};

	std::unordered_set<SPair, SPairHasher> visited;
	std::vector<SimState> workList;

	assert(nfa2.getNumStarting() == 1);
	std::for_each(nfa1.start_begin(), nfa1.start_end(), [&](auto *s1){
		std::for_each(nfa2.start_begin(), nfa2.start_end(), [&](auto *s2){
			visited.insert({s1, s2});
			workList.push_back({s1, s2, std::nullopt, std::nullopt});
		});
	});

	while (!workList.empty()) {
		auto [s1, s2, f, l] = workList.back();
		workList.pop_back();

		if (nfa1.isAccepting(s1) && nfa2.isAccepting(s2)) {
			fst = f.value_or(PredicateSet());
			lst = l.value_or(PredicateSet());
			return true;
		}

		for (auto it = s1->out_begin(); it != s1->out_end(); ++it) {
			for (auto oit = s2->out_begin(); oit != s2->out_end(); ++oit) {
				if (it->label != oit->label &&
				    !(it->label.isPredicate() && oit->label.isPredicate() &&
				      oit->label.getPreChecks().includes(it->label.getPreChecks())) &&
				    !(!f.has_value() && it->label.isPredicate() && oit->label.isPredicate() &&
				      it->label.composesWith(oit->label)) &&
				    !(nfa1.isAccepting(it->dest) && nfa2.isAccepting(oit->dest) &&
				      it->label.isPredicate() && oit->label.isPredicate() &&
				      it->label.composesWith(oit->label))
					)
					continue;
				if (visited.count({it->dest, oit->dest}))
					continue;

				auto lab = oit->label.getPreChecks();
				lab.minus(it->label.getPreChecks());
				if (!f.has_value())
					f = oit->label.isPredicate() ? lab : PredicateSet();
				l = oit->label.isPredicate() ? lab : PredicateSet();

				visited.insert({it->dest, oit->dest});
				workList.push_back({it->dest, oit->dest, f, l});
			}
		}
	}
	return false;
}

void normalize(NFA &nfa, Constraint::ValidFunT vfun);

std::vector<std::vector<Path>>
findAllMatchingPaths(const NFA &pattern, const NFA &nfa)
{
	std::vector<std::vector<Path>> result;
	std::unordered_map<NFA::State *, NFA::State *> m, rm;

	auto nfac = nfa.copy(&m);
	std::for_each(m.begin(), m.end(), [&](auto &kv){
		rm[kv.second] = kv.first;
	});

	auto patc = pattern.copy();

	std::for_each(nfac.states_begin(), nfac.states_end(), [&](auto &is){
		nfac.clearAllStarting();
		nfac.makeStarting(&*is);

		std::vector<Path> ps;
		std::for_each(nfac.states_begin(), nfac.states_end(), [&](auto &fs){
			nfac.clearAllAccepting();
			nfac.makeAccepting(&*fs);

			PredicateSet f, l;
			if (hasMatchingPathDFS(patc, nfac, f, l))
				ps.push_back({rm[&*is], rm[&*fs], f, l});
		});
		result.push_back(ps);
	});
	return result;
}

void expandAssumption(NFA &nfa, const std::unique_ptr<Constraint> &assm)
{
	assert(&*assm);
	if (auto *tc = dynamic_cast<TotalityConstraint *>(&*assm)) {
		saturateTotal(nfa, *static_cast<CharRE *>(tc->getRelation())->getLabel().getRelation());
		return;
	}

	auto *ec = dynamic_cast<SubsetConstraint *>(&*assm);
	if (!ec) {
		std::cout << "Ignoring unsupported local assumption " << *assm << "\n";
		return;
	}

	auto *lRE = &*ec->getKid(0);
	auto *rRE = &*ec->getKid(1);
	auto lNFA = lRE->toNFA();
	normalize(lNFA, [](auto &t){ return true; });

	auto rNFA = rRE->toNFA();
	normalize(rNFA, [](auto &t){ return true; });

	auto paths = findAllMatchingPaths(rNFA, nfa);

	std::vector<NFA::State *> inits(lNFA.start_begin(), lNFA.start_end());
	std::vector<NFA::State *> finals(lNFA.accept_begin(), lNFA.accept_end());

	lNFA.clearAllAccepting();
	lNFA.clearAllStarting();

	std::for_each(paths.begin(), paths.end(), [&](auto &ps){
		std::unordered_map<NFA::State *, NFA::State *> m;

		auto lcopy = lNFA.copy(&m);
		nfa.alt(std::move(lcopy));

		std::for_each(ps.begin(), ps.end(), [&](auto &p){
			std::for_each(inits.begin(), inits.end(), [&](auto *i){
				if (!p.fst.empty())
					nfa.addTransition(p.start, NFA::Transition(
								  TransLabel(std::nullopt, p.fst), m[i]));
				else
					nfa.addEpsilonTransitionSucc(p.start, m[i]);
			});
			std::for_each(finals.begin(), finals.end(), [&](auto *f){
				if (!p.lst.empty())
					nfa.addTransition(m[f], NFA::Transition(
								  TransLabel(std::nullopt, p.lst), p.end));
				else
					nfa.addEpsilonTransitionPred(m[f], p.end);
			});
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

	saturatePreds(nfa, opreds);
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

void normalize(NFA &nfa, Constraint::ValidFunT vfun)
{
	nfa.simplify(vfun);
	saturateDomains(nfa);
	nfa.breakToParts();
	nfa.removeDeadStates();
	removeConsecutivePredicates(nfa);
	nfa.removeDeadStates();
}

bool checkStaticInclusion(const RegExp *re1, const RegExp *re2,
			  const std::vector<std::unique_ptr<Constraint>> &assms,
			  std::string &cex, Constraint::ValidFunT vfun,
			  bool satInitFinalPreds = false)
{
	auto nfa1 = re1->toNFA();
	normalize(nfa1, vfun);
	if (satInitFinalPreds)
		saturateInitFinalPreds(nfa1);
	auto lhs = nfa1.to_DFA().first;

	auto nfa2 = re2->toNFA();
	normalize(nfa2, vfun);
	if (!assms.empty()) {
		std::for_each(assms.begin(), assms.end(), [&](auto &assm){
			expandAssumption(nfa2, assm);
			normalize(nfa2, vfun);
		});
	}
	saturateNFA(nfa2, nfa1);
	pruneNFA(nfa2, nfa1);
	nfa2.removeDeadStates();

	auto rhs = nfa2.to_DFA().first;
	nfa2.simplify(vfun);
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

bool SubsetIDConstraint::checkStatically(const std::vector<std::unique_ptr<Constraint>> &assms,
					 std::string &cex, Constraint::ValidFunT vfun) const
{
	return checkStaticInclusion(getKid(0), getKid(1), assms, cex, vfun, true);
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
