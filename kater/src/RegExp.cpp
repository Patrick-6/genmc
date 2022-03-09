#include "RegExp.hpp"

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
