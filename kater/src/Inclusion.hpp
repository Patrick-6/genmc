#ifndef __INCLUSION_HPP__
#define __INCLUSION_HPP__

#include <string>

#include "Constraint.hpp"

template<typename T>
struct Inclusion {
	T lhs;
	T rhs;
	Constraint::Type type;
	std::string s;
};

#endif /* __INCLUSION_HPP__ */
