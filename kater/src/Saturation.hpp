#ifndef __SATURATION_HPP__
#define __SATURATION_HPP__

#include "NFA.hpp"

/*
 * Saturates the NFA given a collection of predicate labels LABS s.t.
 * for all LAB \in LABS. LAB <= ID
 */
void saturateID(NFA &nfa, const std::vector<TransLabel> &labs);

/*
 * Saturates the NFA given an NFA EMPTY that corresponds
 * to a relation R = 0
 */
void saturateEmpty(NFA &nfa, NFA &&empty);

/*
 * Saturates the NFA given a relation label LAB. total(LAB)
 */
void saturateTotal(NFA &nfa, const TransLabel &lab);

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
