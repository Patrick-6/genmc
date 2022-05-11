#ifndef __SATURATION_HPP__
#define __SATURATION_HPP__

#include "NFA.hpp"

/*
 * Saturates the NFA given a predicate label LAB. LAB <= ID
 */
void saturateIDs(NFA &nfa, const TransLabel &lab);

/*
 * Saturates the NFA given an NFA EMPTY that corresponds
 * to a relation R = 0
 */
void saturateEmpty(NFA &nfa, NFA &&empty);

/*
 * Saturates the NFA given a relation label LAB. total(LAB)
 */
void saturateTotal(NFA &nfa, const TransLabel &lab);

#endif /* __SATURATION_HPP__ */
