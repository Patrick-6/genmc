#include "Saturation.hpp"

bool shouldSaturateStateID(NFA &nfa, NFA::State *s)
{
	return nfa.isStarting(s) || nfa.isAccepting(s) ||
		(std::any_of(s->in_begin(), s->in_end(), [&](auto &t){
			return nfa.isStarting(t.dest) || !t.label.isPredicate();
		}) &&
		std::any_of(s->out_begin(), s->out_end(), [&](auto &t){
			return nfa.isAccepting(t.dest) || !t.label.isPredicate();
		}));
}

bool isComposablePredicate(const TransLabel &lab, const TransLabel &pred)
{
	assert(pred.isPredicate());
	return lab.isPredicate() &&
		lab.getPreChecks().composes(pred.getPreChecks());
}

bool isComposableRelation(const TransLabel &lab, const TransLabel &pred)
{
	assert(pred.isPredicate());
	return !lab.isPredicate() && lab.isBuiltin() &&
		pred.getPreChecks().composes(lab.getRelation()->getDomain());
}

bool canSaturateStateWithID(NFA &nfa, NFA::State *s, const TransLabel &lab)
{
	return nfa.isStarting(s) || nfa.isAccepting(s) ||
		(std::any_of(s->out_begin(), s->out_end(), [&](auto &t){
			return isComposablePredicate(t.label, lab) ||
				isComposableRelation(t.label, lab);
		}) &&
		std::any_of(s->in_begin(), s->in_end(), [&](auto &t){
			return isComposablePredicate(t.label, lab) ||
				isComposableRelation(t.label, lab);
		}));
}

void saturateID(NFA &nfa, const std::vector<TransLabel> &labs)
{
	assert(std::all_of(labs.begin(), labs.end(), [&](auto &lab){ return lab.isPredicate(); }));

	for (auto it = nfa.states_begin(), ie = nfa.states_end(); it != ie; ++it) {
		auto &s = *it;

		if (!shouldSaturateStateID(nfa, &*s))
			continue;

		std::vector<TransLabel> toAdd;
		std::for_each(labs.begin(), labs.end(), [&](auto &lab){
			if (canSaturateStateWithID(nfa, &*s, lab))
				toAdd.push_back(lab);
		});
		std::for_each(toAdd.begin(), toAdd.end(), [&](auto &t){
			nfa.addSelfTransition(&*s, t);
		});
	}
}

std::vector<TransLabel> collectLabels(const NFA &nfa)
{
	std::vector<TransLabel> labels;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			labels.push_back(t.label);
		});
	});
	return labels;
}

void saturateEmpty(NFA &start, NFA &&empty)
{
	auto labels = collectLabels(start);
	auto eLabs = collectLabels(empty);

	labels.insert(labels.end(), eLabs.begin(), eLabs.end());
	std::sort(labels.begin(), labels.end());
	labels.erase(std::unique(labels.begin(), labels.end()), labels.end());

	std::for_each(empty.start_begin(), empty.start_end(), [&](auto &s){
		std::for_each(labels.begin(), labels.end(), [&](auto &lab){
			empty.addSelfTransition(s, lab);
		});
	});
	std::for_each(empty.accept_begin(), empty.accept_end(), [&](auto &a){
		std::for_each(labels.begin(), labels.end(), [&](auto &lab){
			empty.addSelfTransition(a, lab);
		});
	});

	start.alt(std::move(empty));
}

void saturateTotal(NFA &nfa, const Relation &rel)
{
	TransLabel lab(rel);
	std::vector<NFA::State *> toDuplicate;

	lab.flip();
	for (auto it = nfa.states_begin(); it != nfa.states_end(); ++it) {
		auto *s = it->get();
		if (std::any_of(s->in_begin(), s->in_end(),
				[&](auto &t){ return t.label == lab; }))
			toDuplicate.push_back(s);
	}

	std::for_each(toDuplicate.begin(), toDuplicate.end(), [&](auto *s) {
		if (std::any_of(s->in_begin(), s->in_end(),
				[&](auto &t){ return t.label != lab; })) {
			auto *d = nfa.createState();
			if (nfa.isAccepting(s))
				nfa.makeAccepting(d);
			nfa.addTransitions(d, s->out_begin(), s->out_end());
			nfa.addInvertedTransitions(d, s->in_begin(), s->in_end());
			nfa.removeInvertedTransitionsIf(d, [&](auto &t){ return t.label == lab; });
			nfa.removeInvertedTransitionsIf(s, [&](auto &t){ return t.label != lab; });
		}
		std::vector<NFA::Transition> ins(s->in_begin(), s->in_end());
		std::for_each(ins.begin(), ins.end(), [&](auto &t){
			assert(t.label == lab);
			nfa.addEpsilonTransitionPred(s, t.dest);
		});
	});
}

void saturateDomains(NFA &nfa)
{
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		/* toAdd: temp workaround due to std::set exposing const_iters only */
		std::vector<NFA::Transition> toAdd;
		std::vector<NFA::Transition> toRemove;
		for (auto it = s->out_begin(), ie = s->out_end(); it != ie; ++it) {
			auto lab = it->label;
			if (lab.isPredicate() || !lab.getRelation()->isBuiltin())
				continue;

			if (lab.getPreChecks().merge(lab.getRelation()->getDomain()) &&
			    lab.getPostChecks().merge(lab.getRelation()->getCodomain()))
				toAdd.push_back(NFA::Transition(lab, it->dest));
			toRemove.push_back(*it);
		}
		nfa.removeTransitions(&*s, toRemove.begin(), toRemove.end());
		nfa.addTransitions(&*s, toAdd.begin(), toAdd.end());
	});
}

void saturateRotate(NFA &nfa)
{
	NFA result;

	for (auto it = nfa.states_begin(); it != nfa.states_end(); ++it) {
		auto &s = *it;
		if (nfa.isStarting(&*s) ||
		    (nfa.isAccepting(&*s) && s->getNumOutgoing()))
		    continue;

		std::unordered_map<NFA::State *, NFA::State *> m1, m2;

		auto rot1 = nfa.copy(&m1);
		auto rot2 = nfa.copy(&m2);

		rot1.clearAllStarting();
		rot1.makeStarting(m1[&*s]);

		rot2.clearAllAccepting();
		rot2.makeAccepting(m2[&*s]);

		rot1.seq(std::move(rot2));
		result.alt(std::move(rot1));
	}
	nfa.alt(std::move(result));
	nfa.removeDeadStates();
}
