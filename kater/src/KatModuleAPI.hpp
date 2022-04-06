#ifndef __KAT_MODULE_API_HPP__
#define __KAT_MODULE_API_HPP__

#include "RegExp.hpp"

using URE = std::unique_ptr<RegExp>;

enum class VarStatus { Normal, Reduce, View };

// FIXME: Polymorphism
struct SavedVar {
	SavedVar(URE exp) : exp(std::move(exp)) {}
	SavedVar(URE exp, NFA::ReductionType t, URE red)
		: exp(std::move(exp)), status(VarStatus::Reduce),
		  redT(std::move(t)), red(std::move(red)) {}

	SavedVar(URE exp, VarStatus s)
		: exp(std::move(exp)), status(s) {}

	URE exp = nullptr;
	VarStatus status = VarStatus::Normal;
	NFA::ReductionType redT = NFA::ReductionType::Self;
	URE red = nullptr;
};

#endif /* __KAT_MODULE_API_HPP__ */
