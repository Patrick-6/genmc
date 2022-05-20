#include "Saturation.hpp"

void saturateID(NFA &nfa, const std::vector<TransLabel> &labs)
{
	assert(std::all_of(labs.begin(), labs.end(), [&](auto &lab){ return lab.isPredicate(); }));
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(labs.begin(), labs.end(), [&](auto &lab){
			nfa.addSelfTransition(&*s, lab);
		});
	});
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

void saturateTotal(NFA &nfa, const TransLabel &lab)
{
	assert(!lab.isPredicate());
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			if (t.label == lab)
				nfa.addEpsilonTransitionSucc(t.dest, &*s);
		});
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
