#include "RegExp.hpp"

bool RegExp::isFalse() const
{
	return getNumKids() == 0 && dynamic_cast<const AltRE *>(this);
}

std::unique_ptr<RegExp>
RegExp::optimize(std::unique_ptr<RegExp> re)
{
	if (!re->hasKids())
		return re;

	for (auto i = 0u; i < re->getNumKids(); i++)
		re->setKid(i, optimize(std::move(re->kids[i])));

	/* The only expression we actually optimize is SeqRE */
	if (auto *seqRE = dynamic_cast<const SeqRE *>(&*re))
		return SeqRE::createOpt(std::move(re->kids[0]), std::move(re->kids[1]));
	return re;
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
AltRE::createOpt (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	std::vector<std::unique_ptr<RegExp> > r;

        ADD_CHILD(r, r1, AltRE);
        ADD_CHILD(r, r2, AltRE);
	RETURN_OBJ(r, AltRE);
}

std::unique_ptr<RegExp>
SeqRE::createOpt (std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	std::vector<std::unique_ptr<RegExp>> r;

        ADD_CHILD(r, r1, SeqRE);
        ADD_CHILD(r, r2, SeqRE);
	for (auto it = r.begin(); it != r.end(); it++) {
		if ((*it)->isFalse()) return std::move(*it);
	}
	RETURN_OBJ(r, SeqRE);
}

std::unique_ptr<RegExp>
SymRE_create(std::unique_ptr<RegExp> re)
{
	auto re1 = re->clone();
	return AltRE::create(std::move(re), InvRE::create(std::move(re1)));
}
