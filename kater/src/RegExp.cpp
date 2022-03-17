#include "RegExp.hpp"

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

template<typename OptT>
void addChildToVector(std::unique_ptr<RegExp> arg, std::vector<std::unique_ptr<RegExp>> &res)
{
	if (auto *re = dynamic_cast<const OptT *>(&*arg)) {
		for (auto i = 0u; i < arg->getNumKids(); i++)
			res.emplace_back(arg->releaseKid(i));
	} else {
		res.emplace_back(std::move(arg));
	}
}

template<typename OptT, typename... Ts>
std::vector<std::unique_ptr<RegExp>>
createOptChildVector(Ts... args)
{
	std::vector<std::unique_ptr<RegExp>> res;
	(addChildToVector<OptT>(std::move(args), res), ...);
	return res;
}

std::unique_ptr<RegExp>
AltRE::createOpt(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	auto r = createOptChildVector<AltRE>(std::move(r1), std::move(r2));
	std::sort(r.begin(), r.end());
	r.erase(std::unique(r.begin(), r.end()), r.end());
	return r.size() == 1 ? std::move(*r.begin()) : AltRE::create(std::move(r));
}

std::unique_ptr<RegExp>
SeqRE::createOpt(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	auto r = createOptChildVector<SeqRE>(std::move(r1), std::move(r2));

	auto it = std::find_if(r.begin(), r.end(), [&](auto &re){ return re->isFalse(); });
	if (it != r.end())
		return std::move(*it);

	for (auto it = r.begin(); it != r.end() && it + 1 != r.end(); /* */) {
		if (auto p = dynamic_cast<PredRE *>(it->get()))
			if (auto q = dynamic_cast<PredRE *>((it + 1)->get())) {
				if (!p->getLabel().merge(q->getLabel()))
					return RegExp::createFalse();
				it = r.erase(it + 1);
				continue;
			}
		++it;
	}
	return r.size() == 1 ? std::move(*r.begin()) : SeqRE::create(std::move(r));
}

RegExp &SeqRE::flip()
{
	for (auto &r : getKids())
		r->flip();
	std::reverse(kids.begin(), kids.end());
	return *this;
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
