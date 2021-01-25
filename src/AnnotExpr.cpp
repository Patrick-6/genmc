/*
 * GenMC -- Generic Model Checking.
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
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "AnnotExpr.hpp"
#include "Error.hpp"

#include <algorithm>

using llvm::GenericValue;
using llvm::Type;

unsigned AnnotationExpr::count = 0;
unsigned SymbolicValue::nextName = 0;

unsigned AnnotationExpr::computeHash()
{
	unsigned res = getKind() * AnnotationExpr::MAGIC_HASH_CONSTANT;

	int n = getNumKids();
	for (int i = 0; i < n; i++) {
		res <<= 1;
		res ^= getKid(i)->hash() * AnnotationExpr::MAGIC_HASH_CONSTANT;
	}

	hashValue = res;
	return hashValue;
}

ConcreteExpr::ConcreteExpr(llvm::GenericValue v, const llvm::Type::TypeID t, Width w)
	: value(std::move(v)), type_id(t), width(w)
{
	// We must know width for integers
	BUG_ON(t != llvm::Type::IntegerTyID);
	BUG_ON(w == 0);
}

std::string ConcreteExpr::genericValueToString(const GenericValue &value, Type::TypeID type_id, const Type *type)
{
	std::stringstream ss;
	switch (type_id) {
        default:
		WARN("Unexpected type of concrete expression");
		BUG();
        case Type::IntegerTyID:
		// gambling
		return value.IntVal.toString(10, true);
        case Type::PointerTyID:
		ss << value.PointerVal;
		break;
        case Type::FloatTyID:
		ss << value.FloatVal;
		break;
        case Type::DoubleTyID:
		ss << value.DoubleVal;
		break;
        case Type::VectorTyID:
		BUG_ON(type == nullptr);
		ss << '[';
		for (const auto &val : value.AggregateVal) {
			auto contained_type = type->getContainedType(0);
			ss << genericValueToString(val, contained_type->getTypeID(), contained_type) << ' ';
		}
		ss << ']';
		break;
	}
	return ss.str();

}

std::string ConcreteExpr::toString() const
{
	std::stringstream ss;
	switch (type_id) {
        default:
		WARN("Unexpected type of concrete expression");
		BUG();
        case Type::IntegerTyID:
		// gambling
		return value.IntVal.toString(10, (width > 1 ? true : false));
        case Type::PointerTyID:
		ss << value.PointerVal;
		break;
        case Type::FloatTyID:
		ss << value.FloatVal;
		break;
        case Type::DoubleTyID:
		ss << value.DoubleVal;
		break;
        case Type::VectorTyID:
		ss << '[';
		for (auto val : value.AggregateVal) {
			ss << val.IntVal.toString(10, true) << ' ';
		}
		ss << ']';
		break;
	}
	return ss.str();
}

unsigned NotExpr::computeHash()
{
	hashValue = expr->hash() * AnnotationExpr::MAGIC_HASH_CONSTANT * AnnotationExpr::Not;
	return hashValue;
}

unsigned CastExpr::computeHash()
{
	unsigned res = getWidth() * AnnotationExpr::MAGIC_HASH_CONSTANT;
	hashValue = res ^ src->hash() * AnnotationExpr::MAGIC_HASH_CONSTANT;
	return hashValue;
}

std::shared_ptr<AnnotationExpr> NotExpr::create(const std::shared_ptr<AnnotationExpr> &e)
{
	BUG_ON(llvm::isa<ConcreteExpr>(e.get()));
	return NotExpr::alloc(e);
}

std::shared_ptr<AnnotationExpr> SelectExpr::create(std::shared_ptr<AnnotationExpr> c, std::shared_ptr<AnnotationExpr> t, std::shared_ptr<AnnotationExpr> f) {
	// TODO: optimizations
	return SelectExpr::alloc(c, t, f);
}

std::string SelectExpr::toString() const
{
	BUG(); /* do not expect to call this */
	return "";
}

std::string CastExpr::toString() const
{
	return "cast" + std::to_string(width) + "(" + src->toString() + ")";
}

std::shared_ptr<AnnotationExpr> ZExtExpr::create(const std::shared_ptr<AnnotationExpr> &e, AnnotationExpr::Width w)
{
	AnnotationExpr::Width cur_width = e->getWidth();
	BUG_ON(cur_width == 0);
	// Can't truncate
	BUG_ON(w < cur_width);
	BUG_ON(llvm::isa<ConcreteExpr>(e.get()));
	if (cur_width == w) {
		return e;
	}
	return ZExtExpr::alloc(e, w);
}

std::shared_ptr<AnnotationExpr> SExtExpr::create(const std::shared_ptr<AnnotationExpr> &e, AnnotationExpr::Width w)
{
	AnnotationExpr::Width cur_width = e->getWidth();
	BUG_ON(cur_width == 0);
	// Can't truncate
	BUG_ON(w < cur_width);
	BUG_ON(llvm::isa<ConcreteExpr>(e.get()));
	if (cur_width == w) {
		return e;
	}
	return SExtExpr::alloc(e, w);
}

std::shared_ptr<AnnotationExpr> TruncExpr::create(const std::shared_ptr<AnnotationExpr> &e, AnnotationExpr::Width w)
{
	AnnotationExpr::Width cur_width = e->getWidth();
	BUG_ON(cur_width == 0);
	// Can't extend
	BUG_ON(w > cur_width);
	BUG_ON(llvm::isa<ConcreteExpr>(e.get()));
	if (cur_width == w) {
		return e;
	}
	return TruncExpr::alloc(e, w);
}

// TODO: optimizing creates from KLEE

std::shared_ptr<AnnotationExpr> AddExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return AddExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> SubExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return SubExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> MulExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return MulExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> UDivExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return UDivExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> SDivExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return SDivExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> URemExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return URemExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> SRemExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return SRemExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> AndExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete or @l is False
	return AndExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> OrExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return OrExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> XorExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return XorExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> ShlExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return ShlExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> LShrExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return LShrExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> AShrExpr::create(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(l, r);
	}
	// TODO: simplify the expression if one of the children of @r is concrete
	return AShrExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> EqExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	// We shouldn't have called this method if both expressions are actually concrete values
	BUG_ON(llvm::isa<ConcreteExpr>(l.get()) && llvm::isa<ConcreteExpr>(r.get()));
	// Let concrete value always be on the left
	auto left_child = l;
	auto right_child = r;
	if (llvm::isa<ConcreteExpr>(r.get())) {
		std::swap(left_child, right_child);
	}
	return EqExpr::alloc(left_child, right_child);
}

std::shared_ptr<AnnotationExpr> NeExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return NotExpr::create(EqExpr::create(l, r));
}

std::shared_ptr<AnnotationExpr> UltExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return UltExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> UleExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r) {
	return UleExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> SltExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return SltExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> SleExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return SleExpr::alloc(l, r);
}

std::shared_ptr<AnnotationExpr> UgtExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return UltExpr::create(r, l);
}

std::shared_ptr<AnnotationExpr> UgeExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return UleExpr::create(r, l);
}

std::shared_ptr<AnnotationExpr> SgtExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return SltExpr::create(r, l);
}

std::shared_ptr<AnnotationExpr> SgeExpr::create(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
{
	return SleExpr::create(r, l);
}

llvm::APInt SExtExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).sext(getWidth());
}

llvm::APInt ZExtExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).zext(getWidth());
}

llvm::APInt TruncExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).trunc(getWidth());
}

llvm::APInt AddExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) + getKid(1)->evaluate(symbolicVal);
}

llvm::APInt SubExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) - getKid(1)->evaluate(symbolicVal);
}

llvm::APInt MulExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) * getKid(1)->evaluate(symbolicVal);
}

llvm::APInt UDivExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).udiv(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt SDivExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).sdiv(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt URemExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).urem(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt SRemExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).srem(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt AndExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) & getKid(1)->evaluate(symbolicVal);
}

llvm::APInt OrExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) | getKid(1)->evaluate(symbolicVal);
}

llvm::APInt XorExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal) ^ getKid(1)->evaluate(symbolicVal);
}

llvm::APInt ShlExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).shl(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt LShrExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).lshr(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt AShrExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	return getKid(0)->evaluate(symbolicVal).ashr(getKid(1)->evaluate(symbolicVal));
}

llvm::APInt EqExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).eq(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt NeExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).ne(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt UltExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).ult(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt SltExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).slt(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt UleExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).ule(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt SleExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).sle(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt UgtExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).ugt(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt SgtExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).sgt(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt UgeExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).uge(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}

llvm::APInt SgeExpr::evaluate(llvm::GenericValue symbolicVal) const
{
	if (getKid(0)->evaluate(symbolicVal).sge(getKid(1)->evaluate(symbolicVal)))
		return llvm::APInt(BoolWidth, 1);
	else
		return llvm::APInt(BoolWidth, 0);
}
