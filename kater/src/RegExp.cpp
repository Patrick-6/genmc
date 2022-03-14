#include "RegExp.hpp"

bool RegExp::isFalse() const
{
	return getNumKids() == 0 && dynamic_cast<const AltRE *>(this);
}

RegExp &RegExp::flip()
{
	for (auto &r : getKids())
		r->flip();
	return *this;
}

#define ADD_CHILD(_res, _arg, _type)					\
	do {								\
		if (auto *re = dynamic_cast<const _type *>(&* _arg))	\
			for (int i = 0; i < _arg->getNumKids(); i++)	\
				_res.emplace_back(_arg->releaseKid(i));	\
		else _res.emplace_back(std::move(_arg));		\
	} while (false)

#define RETURN_OBJ(_res, _type)				\
	do {						\
		if (r.size() == 1)			\
			return std::move(r[0]);		\
		return _type::create(std::move(r));	\
	} while (false)

std::unique_ptr<RegExp>
AltRE::createOpt(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	std::vector<std::unique_ptr<RegExp> > r;

        ADD_CHILD(r, r1, AltRE);
        ADD_CHILD(r, r2, AltRE);
	std::sort(r.begin(), r.end());
	RETURN_OBJ(r, AltRE);
}

std::unique_ptr<RegExp>
SeqRE::createOpt(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	std::vector<std::unique_ptr<RegExp>> r;

        ADD_CHILD(r, r1, SeqRE);
        ADD_CHILD(r, r2, SeqRE);
	for (auto it = r.begin(); it != r.end(); it++) {
		if ((*it)->isFalse()) return std::move(*it);
	}
	RETURN_OBJ(r, SeqRE);
}

RegExp &SeqRE::flip()
{
	for (auto &r : getKids()) r->flip();
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

std::unique_ptr<RegExp>
SymRE_create(std::unique_ptr<RegExp> re)
{
	auto re1 = re->clone();
	re1->flip();
	return AltRE::create(std::move(re), std::move(re1));
}
