#ifndef __SATURATION_HPP__
#define __SATURATION_HPP__

#include "NFA.hpp"

/*
 * Saturates the NFA given a collection of predicate labels LABS s.t.
 * for all LAB \in LABS. LAB <= ID
 *
 * Pre: NFA is in normal form
 */
void saturatePreds(NFA &nfa, const std::vector<TransLabel> &labs);

/*
 * Saturates the NFA by (1) conjuncting all outgoing
 * predicates from the initial states with Q (where Q is
 * the disjunction of all incoming predicates to final states),
 * and (2) conjuncting all incoming predicates to final
 * states with P (where P is the disjunction of all outgoing
 * predicates from initial states).
 *
 * Pre: NFA is in normal form
 */
void saturateInitFinalPreds(NFA &nfa);

/*
 * Saturates the NFA given an NFA EMPTY that corresponds
 * to a relation R = 0
 */
void saturateEmpty(NFA &nfa, NFA &&empty);

/*
 * Saturates the NFA given a relation REL. total(REL)
 */
void saturateTotal(NFA &nfa, const Relation &rel);

/*
 * Saturates by adding domains/codomains for all builtins.
 * (If a dom/codom does not compose with the existing
 * pre/post-checks of a label, the label is removed.)
 *
 */
void saturateDomains(NFA &nfa);

/*
 * Saturates the NFA by rotating it.
 */
void saturateRotate(NFA &nfa);

#endif /* __SATURATION_HPP__ */
