/*
 * KATER -- Automating Weak Memory Model Metatheory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 */

#include "RegExp.hpp"
#include "Saturation.hpp"

std::unique_ptr<RegExp> RegExp::createFalse()
{
	return AltRE::create();
}

std::unique_ptr<RegExp> RegExp::createId()
{
	TransLabel t (std::nullopt);
	return CharRE::create(t);
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

std::unique_ptr<RegExp> RegExp::getDomain() const
{
	if (auto charRE = dynamic_cast<const CharRE *>(&*this)) {
		TransLabel p (std::nullopt, charRE->getLabel().getPreChecks(), {});
		return CharRE::create(p);
	}
	return createId();
}

std::unique_ptr<RegExp> RegExp::getCodomain() const
{
	if (auto charRE = dynamic_cast<const CharRE *>(&*this)) {
		TransLabel p (std::nullopt, charRE->getLabel().getPostChecks(), {});
		return CharRE::create(p);
	}
	return createId();
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

static bool isPredicate (const RegExp &r)
{
	if (auto charRE = dynamic_cast<const CharRE *>(&r))
		return charRE->getLabel().isPredicate();
	return false;
}

static bool isAnyRel (const RegExp &r)
{
	if (auto charRE = dynamic_cast<const CharRE *>(&r))
		if (auto rel = charRE->getLabel().getRelation())
			return (rel->isBuiltin() && rel->toBuiltin() == Relation::any);
	return false;
}

std::unique_ptr<RegExp>
AndRE::createOpt(std::unique_ptr<RegExp> r1, std::unique_ptr<RegExp> r2)
{
	// `r & r = r`
	if (*r1 == *r2)
		return std::move(r1);
	// `pred & pred2 =  pred1 ; pred2`
	if (isPredicate(*r1) && isPredicate(*r2))
		return SeqRE::createOpt(std::move(r1), std::move(r2));
	// `([pred] ; any ; [pred2]) & r = [pred1] ; r ; [pred2]`
	if (isAnyRel(*r1))
		return SeqRE::createOpt(r1->getDomain(), std::move(r2), r1->getCodomain());
	// `r & ([pred] ; any ; [pred2]) = [pred1] ; r ; [pred2]`
	if (isAnyRel(*r2))
		return SeqRE::createOpt(r2->getDomain(), std::move(r1), r2->getCodomain());
	return create(std::move(r1), std::move(r2));
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
