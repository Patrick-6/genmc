#include "RegExp.hpp"
#include "Saturation.hpp"

std::unique_ptr<RegExp> RegExp::createFalse()
{
	return AltRE::create();
}

std::unique_ptr<RegExp> RegExp::createSym(std::unique_ptr<RegExp> re)
{
	auto re1 = re->clone();
	re1->flip();
	return AltRE::createOpt(std::move(re), std::move(re1));
}

bool RegExp::isFalse() const
{
	return getNumKids() == 0 && dynamic_cast<const AltRE *>(this);
}

std::unique_ptr<RegExp>
PlusRE::createOpt(std::unique_ptr<RegExp> r)
{
	if (dynamic_cast<PlusRE *>(&*r) || dynamic_cast<StarRE *>(&*r))
		return std::move(r);
	if (dynamic_cast<QMarkRE *>(&*r))
		return StarRE::createOpt(r->releaseKid(0));
	return create(std::move(r));
}

std::unique_ptr<RegExp>
StarRE::createOpt(std::unique_ptr<RegExp> r)
{
	if (dynamic_cast<StarRE *>(&*r))
		return std::move(r);
	if (dynamic_cast<PlusRE *>(&*r) || dynamic_cast<QMarkRE *>(&*r))
		return create(r->releaseKid(0));
	return create(std::move(r));
}

std::unique_ptr<RegExp>
QMarkRE::createOpt(std::unique_ptr<RegExp> r)
{
	if (dynamic_cast<QMarkRE *>(&*r) || dynamic_cast<StarRE *>(&*r))
		return std::move(r);
	if (dynamic_cast<PlusRE *>(&*r))
		return StarRE::createOpt(r->releaseKid(0));
	return create(std::move(r));
}

std::unique_ptr<RegExp>
RotRE::createOpt(std::unique_ptr<RegExp> r)
{
	if (dynamic_cast<RotRE *>(&*r))
		return std::move(r);
	return create(std::move(r));
}

NFA RotRE::toNFA() const
{
	auto nfa = getKid(0)->toNFA();
	saturateRotate(nfa);
	return nfa;
}
