#include "Saturation.hpp"

void saturateIDs(NFA &nfa, const TransLabel &lab)
{
	assert(lab.isPredicate());
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(lab.pre_begin(), lab.pre_end(), [&](auto &p){
			nfa.addSelfTransition(&*s, TransLabel(std::nullopt, {p}));
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
				nfa.addEpsilonTransition(t.dest, &*s);
		});
	});
}
