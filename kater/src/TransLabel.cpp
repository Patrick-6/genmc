#include "TransLabel.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

bool TransLabel::merge(const TransLabel &other,
		       std::function<bool(const TransLabel &)> isValid)
{
	if (!isValid(*this) || !isValid(other))
		return false;
	if (!isPredicate() && !other.isPredicate())
		return false;

	if (isPredicate() && getPreChecks().composes(other.getPreChecks())) {
		/* Do not merge into THIS before ensuring combo is valid */
		TransLabel t(*this);
		t.getRelation() = other.getRelation();
		t.getPreChecks().merge(other.getPreChecks());
		t.flipped = other.isFlipped();
		assert(t.getPostChecks().empty());
		t.getPostChecks() = other.getPostChecks();
		if (isValid(t))
			*this = t;
		return isValid(t);
	} else if (!isPredicate() && other.isPredicate() &&
		   getPostChecks().composes(other.getPreChecks())) {
		TransLabel t(*this);
		t.getPostChecks().merge(other.getPreChecks());
		if (isValid(t))
			*this = t;
		return isValid(t);
	}
	return false;
}

bool TransLabel::composesWith(const TransLabel &other) const
{
	if (isPredicate() && other.isPredicate())
		return getPreChecks().composes(other.getPreChecks());

	if (!isPredicate() && other.isPredicate())
		return getPostChecks().composes(other.getPreChecks()) &&
			(!isBuiltin() || getRelation()->getCodomain().composes(other.getPreChecks()));

	if (isPredicate() && !other.isPredicate())
		return getPreChecks().composes(other.getPreChecks()) &&
			(!other.isBuiltin() ||
			 getPreChecks().composes(other.getRelation()->getDomain()));

	return getPostChecks().composes(other.getPreChecks()) &&
		(!isBuiltin() || (getRelation()->getCodomain().composes(other.getPreChecks()) &&
				  (!other.isBuiltin() || getRelation()->getCodomain().composes(
					  other.getRelation()->getDomain())))) &&
		(!other.isBuiltin() ||
		 getPostChecks().composes(other.getRelation()->getDomain()));
}

std::string TransLabel::toString() const
{
	std::stringstream ss;

	if (!getPreChecks().empty())
		ss << getPreChecks() << (!isPredicate() ? ";" : "");
	if (!isPredicate()) {
		ss << getRelation()->getName();
		if (isFlipped())
			ss << "^-1";
	}
	if (!getPostChecks().empty())
		ss << ";" << getPostChecks();
	return ss.str();
}
