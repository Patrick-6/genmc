#include "Support/ResultHandling.hpp"

auto operator<<(llvm::raw_ostream &s, const GenmcScalar &v) -> llvm::raw_ostream &
{
	if (v.is_init) {
		s << "{" << v.value << ", " << v.extra << "}";
	} else {
		s << "{UNINITIALIZED}";
	}
	// TODO GENMC: how to print this? How to present `extra`?
	return s;
}
