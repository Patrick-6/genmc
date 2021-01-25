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

#ifndef __ANNOTATION_EXPR_HPP__
#define __ANNOTATION_EXPR_HPP__

#include "Error.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include "llvm/IR/CallSite.h"
#include <llvm/IR/DebugInfo.h>
#if defined(HAVE_LLVM_IR_DATALAYOUT_H)
#include <llvm/IR/DataLayout.h>
#elif defined(HAVE_LLVM_DATALAYOUT_H)
#include <llvm/DataLayout.h>
#endif
#if defined(HAVE_LLVM_IR_FUNCTION_H)
#include <llvm/IR/Function.h>
#elif defined(HAVE_LLVM_FUNCTION_H)
#include <llvm/Function.h>
#endif
#if defined(HAVE_LLVM_INSTVISITOR_H)
#include <llvm/InstVisitor.h>
#elif defined(HAVE_LLVM_IR_INSTVISITOR_H)
#include <llvm/IR/InstVisitor.h>
#elif defined(HAVE_LLVM_SUPPORT_INSTVISITOR_H)
#include <llvm/Support/InstVisitor.h>
#endif
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <sstream>
#include <vector>

/*******************************************************************************
 *** TODO:
 *
 * 1) Remove shared_ptr<>
 * 2) Fix classof operators
 * 3) Fix coding style
 * 4) Check if the #includes from c++lib are really necessary
 *
 ******************************************************************************/

class AnnotationExpr {
private:
	static unsigned count;

public:
	static const unsigned MAGIC_HASH_CONSTANT = 39;

	using Width = unsigned;
	static const Width BoolWidth = 1;

	const llvm::Type *type = nullptr;

	enum Kind {
		InvalidKind = -1,

		// Primitive

		Concrete = 0,

		BasicSymbolic,

		Read,
		Select,
		Concat,
		Extract,

		// Casting,
		ZExt,
		SExt,
		Trunc,

		// Bit
		Not,

		// All subsequent kinds are binary.

		// Arithmetic
		Add,
		Sub,
		Mul,
		UDiv,
		SDiv,
		URem,
		SRem,

		// Bit
		And,
		Or,
		Xor,
		Shl,
		LShr,
		AShr,

		// Compare
		Eq,
		Ne,  ///< Not used in canonical form
		Ult,
		Ule,
		Ugt, ///< Not used in canonical form
		Uge, ///< Not used in canonical form
		Slt,
		Sle,
		Sgt, ///< Not used in canonical form
		Sge, ///< Not used in canonical form

		LastKind = Sge,

		CastKindFirst = ZExt,
		CastKindLast = Trunc,
		BinaryKindFirst = Add,
		BinaryKindLast = Sge,
		CmpKindFirst = Eq,
		CmpKindLast = Sge
	};

protected:

	unsigned hashValue;
	explicit AnnotationExpr(const llvm::Type *t) : type(t) {}

public:

	AnnotationExpr() { AnnotationExpr::count++; }

	virtual ~AnnotationExpr() { AnnotationExpr::count--; }

	virtual Kind getKind() const = 0;

	virtual Width getWidth() const = 0;

	virtual unsigned getNumKids() const = 0;

	virtual std::shared_ptr<AnnotationExpr> getKid(unsigned i) const = 0;

	virtual std::string toString() const = 0;

	virtual llvm::APInt  evaluate(llvm::GenericValue symbolicVal) const = 0;

	virtual std::shared_ptr<AnnotationExpr> clone(unsigned tid) const = 0;

	/// Returns the pre-computed hash of the current expression
	virtual unsigned hash() const { return hashValue; }

	/// (Re)computes the hash of the current expression.
	/// Returns the hash value.
	virtual unsigned computeHash();

	static bool classof(const AnnotationExpr *) { return true; }
};

// TODO: set width in constructor
class ConcreteExpr : public AnnotationExpr {
private:
	llvm::GenericValue value;
	const llvm::Type::TypeID type_id = llvm::Type::VoidTyID;
	Width width = 0;

	ConcreteExpr(llvm::GenericValue v, llvm::Type::TypeID t, Width w = 0);

	ConcreteExpr(llvm::GenericValue v, const llvm::Type *t)
		: AnnotationExpr(t), value(std::move(v)), type_id(t->getTypeID()) {
		BUG_ON(!(t->isIntegerTy()));
		width = t->getIntegerBitWidth();
	}

public:
	~ConcreteExpr() override = default;

	Kind getKind() const override { return Concrete; }

	Width getWidth() const override { return width; }

	llvm::Type::TypeID getTypeId() const { return type_id; }

	unsigned getNumKids() const override { return 0; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned i) const override { return nullptr; }

	std::string toString() const override;

	llvm::APInt  evaluate(llvm::GenericValue symbolicVal) const override { return value.IntVal; }

	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override { return alloc(value, type_id, width); }

	const llvm::GenericValue &getGenericValue() const { return value; }

	virtual std::shared_ptr<AnnotationExpr> rebuild(std::shared_ptr<AnnotationExpr> kids[]) const {
		assert(0 && "rebuild() on ConcreteExpr");
		return nullptr;
	}

	// TODO, using the base class hash for now
        // virtual unsigned computeHash();

	static std::shared_ptr<ConcreteExpr> alloc(const llvm::GenericValue &v, const llvm::Type *t) {
		std::shared_ptr<ConcreteExpr> r(new ConcreteExpr(v, t));
		r->computeHash();
		return r;
	}

	static std::shared_ptr<ConcreteExpr> alloc(const llvm::GenericValue &v, llvm::Type::TypeID t, Width width) {
		std::shared_ptr<ConcreteExpr> r(new ConcreteExpr(v, t, width));
		r->computeHash();
		return r;
	}

	static std::shared_ptr<ConcreteExpr> create(const llvm::GenericValue &v, const llvm::Type *t) {
		return alloc(v, t);
	}

	static std::shared_ptr<ConcreteExpr> create(const llvm::GenericValue &v, llvm::Type::TypeID t, Width width) {
		return alloc(v, t, width);
	}

	static std::string genericValueToString(const llvm::GenericValue &value, llvm::Type::TypeID type_id,
						const llvm::Type *type);

	static bool classof(const AnnotationExpr *E) { return E->getKind() == AnnotationExpr::Concrete; }

	static bool classof(const ConcreteExpr *) { return true; }
};

class NonConstantExpr : public AnnotationExpr {
protected:
	NonConstantExpr() = default;
	NonConstantExpr(llvm::Type *t) : AnnotationExpr(t) {}

public:
	static bool classof(const AnnotationExpr *E) {
		return E->getKind() != AnnotationExpr::Concrete;
	}

	static bool classof(const NonConstantExpr *) { return true; }
};

class SymbolicValue : public NonConstantExpr {
private:
	bool annotated;
	const std::string name;
	static unsigned nextName;
	Width width;
	std::vector<llvm::GenericValue> values;
	// TODO: record address in some way

	explicit SymbolicValue(llvm::Type *t, bool annotated, const std::string &argname = "")
		: NonConstantExpr(t), annotated(annotated),
		  name(argname.empty() ? "#s" + std::to_string(nextName) : argname) {
		if (argname == "") {
			++nextName;
		}
	        BUG_ON(!t->isIntegerTy());
		width = type->getIntegerBitWidth();
		values.clear();
	}

	explicit SymbolicValue(llvm::Type::TypeID t, Width width, bool annotated, const std::string &argname = "")
		: annotated(annotated),
		  name(argname.empty() ? "#s" + std::to_string(nextName) : argname),
		  width(width) {
		if (argname == "") {
			++nextName;
		}
	        BUG_ON(t != llvm::Type::IntegerTyID);
		values.clear();
	}

public:
	unsigned getNumKids() const override { return 0; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned) const override {
		assert(false);
		return nullptr;
	}

	std::string toString() const override { return name; }

	void assign(unsigned tid, llvm::GenericValue symbolicVal) {
		if (tid >= values.size())
			values.resize(tid + 1, llvm::GenericValue());
		values[tid] = symbolicVal;
	}

	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override { return symbolicVal.IntVal; }

	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {
		if (!annotated) {
			BUG_ON(tid >= values.size());
			return ConcreteExpr::alloc(values[tid], llvm::Type::IntegerTyID, width);
		}

		SymbolicValue *clonedValue = new SymbolicValue(llvm::Type::IntegerTyID, width, annotated, name);
		return std::shared_ptr<AnnotationExpr>(clonedValue);
	}

	Kind getKind() const override { return BasicSymbolic; }

	Width getWidth() const override { return width; }

	static std::shared_ptr<SymbolicValue> alloc(llvm::Type *t, bool annotated, const std::string &argname = "") {
		return std::shared_ptr<SymbolicValue>(new SymbolicValue(t, annotated, argname));
	}

	static std::shared_ptr<SymbolicValue> create(llvm::Type *t, bool annotated) {
		return alloc(t, annotated);
	}

	static bool classof(const AnnotationExpr *E) {
		return E->getKind() == AnnotationExpr::BasicSymbolic;
	}

	static bool classof(const SymbolicValue *) { return true; }
};

class BinaryExpr : public NonConstantExpr {
private:
	std::shared_ptr<AnnotationExpr> left, right;

protected:
	BinaryExpr(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r)
		: left(l), right(r) {}

	virtual std::string getOperator() const = 0;

public:
	unsigned getNumKids() const override { return 2; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned i) const override {
		if (i == 0)
			return left;
		if (i == 1)
			return right;
		return 0;
	}

	std::string toString() const override {
		return "(" + left->toString() + " " + getOperator() + " " + right->toString() + ")";
	}

	static bool classof(const AnnotationExpr *E) {
		Kind k = E->getKind();
		return AnnotationExpr::BinaryKindFirst <= k && k <= AnnotationExpr::BinaryKindLast;
	}

	static bool classof(const BinaryExpr *) { return true; }
};

class CmpExpr : public BinaryExpr {
protected:
	CmpExpr(const std::shared_ptr<AnnotationExpr> l, const std::shared_ptr<AnnotationExpr> r)
		: BinaryExpr(l, r) {}

public:
	Width getWidth() const override { return BoolWidth; }

	static bool classof(const AnnotationExpr *E) {
		Kind k = E->getKind();
		return AnnotationExpr::CmpKindFirst <= k && k <= AnnotationExpr::CmpKindLast;
	}

	static bool classof(const CmpExpr *) { return true; }
};

// Class representing an if-then-else expression.
class SelectExpr : public NonConstantExpr {
private:
	std::shared_ptr<AnnotationExpr> cond, trueExpr, falseExpr;

	SelectExpr(const std::shared_ptr<AnnotationExpr> &c,
		   const std::shared_ptr<AnnotationExpr> &t,
		   const std::shared_ptr<AnnotationExpr> &f)
		: cond(c), trueExpr(t), falseExpr(f) {}

public:
	static std::shared_ptr<AnnotationExpr> alloc(const std::shared_ptr<AnnotationExpr> &c,
			                             const std::shared_ptr<AnnotationExpr> &t,
						     const std::shared_ptr<AnnotationExpr> &f) {
		std::shared_ptr<AnnotationExpr> r(new SelectExpr(c, t, f));
		r->computeHash();
		return r;
	}

	static std::shared_ptr<AnnotationExpr> create(std::shared_ptr<AnnotationExpr> c,
			                              std::shared_ptr<AnnotationExpr> t,
						      std::shared_ptr<AnnotationExpr> f);

	std::string toString() const override;

	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override {
		if (cond->evaluate(symbolicVal).getBoolValue())
			return trueExpr->evaluate(symbolicVal);
		else
			return falseExpr->evaluate(symbolicVal);
	}

	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {
		std::shared_ptr<AnnotationExpr> clonedCond = cond.get()->clone(tid);
		std::shared_ptr<AnnotationExpr> clonedTrueExpr = trueExpr.get()->clone(tid);
		std::shared_ptr<AnnotationExpr> clonedFalseExpr = falseExpr.get()->clone(tid);
		return alloc(clonedCond, clonedTrueExpr, clonedFalseExpr);
	}

	Kind getKind() const override { return Select; }

	Width getWidth() const override { return trueExpr->getWidth(); }

	unsigned getNumKids() const override { return 3; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned i) const override {
		switch (i) {
			case 0:
				return cond;
		        case 1:
		        	return trueExpr;
		        case 2:
		        	return falseExpr;
		        default:
				return nullptr;
		}
	}

	static bool classof(const AnnotationExpr *E) {
		return E->getKind() == AnnotationExpr::Select;
	}

	static bool classof(const SelectExpr *) { return true; }
};


// Bitwise Not
class NotExpr : public NonConstantExpr {
private:
	std::shared_ptr<AnnotationExpr> expr;

	NotExpr(const std::shared_ptr<AnnotationExpr> &e) : expr(e) {}

public:
	static std::shared_ptr<AnnotationExpr> alloc(const std::shared_ptr<AnnotationExpr> &e) {
		std::shared_ptr<AnnotationExpr> r(new NotExpr(e));
		r->computeHash();
		return r;
	}

	static std::shared_ptr<AnnotationExpr> create(const std::shared_ptr<AnnotationExpr> &e);

	std::string toString() const override { return "!" + getKid(0)->toString(); }

	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override {
		if (expr->evaluate(symbolicVal).getBoolValue())
			return llvm::APInt(BoolWidth, 0);
		else
			return llvm::APInt(BoolWidth, 1);
	}

	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {
		std::shared_ptr<AnnotationExpr> clonedExpr = expr.get()->clone(tid);
		return alloc(clonedExpr);
	}

	Kind getKind() const override { return Not; }

	unsigned getWidth() const override { return expr->getWidth(); }

	unsigned getNumKids() const override { return 1; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned i) const override { return (i == 0) ?  expr : nullptr; }

	unsigned computeHash() override;

	static bool classof(const AnnotationExpr *E) {
		return E->getKind() == AnnotationExpr::Not;
	}

	static bool classof(const NotExpr *) { return true; }
};

// Casting
class CastExpr : public NonConstantExpr {
private:
	std::shared_ptr<AnnotationExpr> src;
	Width width;

public:
	CastExpr(const std::shared_ptr<AnnotationExpr> &e, Width w)
		: src(e), width(w) {}

	Width getWidth() const override { return width; }

	unsigned getNumKids() const override { return 1; }

	std::shared_ptr<AnnotationExpr> getKid(unsigned i) const override { return (i == 0) ? src : nullptr; }

	std::string toString() const override;

	static bool needsResultType() { return true; }

	virtual unsigned computeHash() override;

	static bool classof(const AnnotationExpr *E) {
		AnnotationExpr::Kind k = E->getKind();
		return AnnotationExpr::CastKindFirst <= k && k <= AnnotationExpr::CastKindLast;
	}

	static bool classof(const CastExpr *) { return true; }
};

#define CAST_EXPR_CLASS(_class_kind)                                                                      \
class _class_kind ## Expr : public CastExpr {                                                             \
public:                                                                                                   \
        _class_kind ## Expr(const std::shared_ptr<AnnotationExpr>& e, Width w)                            \
	: CastExpr(e, w) {}                                                                               \
									                                  \
	static std::shared_ptr<AnnotationExpr> alloc(const std::shared_ptr<AnnotationExpr> &e, Width w) { \
		std::shared_ptr<AnnotationExpr> r(new _class_kind ## Expr(e, w));                         \
		r->computeHash();					                                  \
		return r;						                                  \
	}								                                  \
									                                  \
	static std::shared_ptr<AnnotationExpr> create(const std::shared_ptr<AnnotationExpr> &e, Width w); \
								                                          \
	Kind getKind() const override { return _class_kind; }	                                          \
                                                                                                          \
	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override;                              \
                                                                                                          \
	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {                              \
		return alloc(getKid(0).get()->clone(tid), getWidth());                                    \
	}                                                                                                 \
								                                          \
	static bool classof(const AnnotationExpr *E) {		                                          \
		return E->getKind() == AnnotationExpr::_class_kind;                                       \
	}							                                          \
								                                          \
	static bool classof(const  _class_kind ## Expr *) {	                                          \
		return true;					                                          \
	}						                                                  \
};								                                          \


CAST_EXPR_CLASS(SExt)

CAST_EXPR_CLASS(ZExt)

CAST_EXPR_CLASS(Trunc)

// Arithmetic/Bit Exprs

#define ARITHMETIC_EXPR_CLASS(_class_kind, _class_operator)                                             \
class _class_kind##Expr : public BinaryExpr {	                                                        \
public:							                                                \
        _class_kind##Expr(std::shared_ptr<AnnotationExpr> l, std::shared_ptr<AnnotationExpr> r)         \
	: BinaryExpr(l, r) {}		                  				                \
									                                \
	static std::shared_ptr<AnnotationExpr> alloc(std::shared_ptr<AnnotationExpr> l,                 \
			                             std::shared_ptr<AnnotationExpr> r) {               \
		std::shared_ptr<AnnotationExpr> res(new _class_kind##Expr(l, r));                       \
		res->computeHash();					                                \
		return res;						                                \
	}								                                \
									                                \
	static std::shared_ptr<AnnotationExpr> create(std::shared_ptr<AnnotationExpr> l,                \
			                              std::shared_ptr<AnnotationExpr> r);               \
								                                        \
	virtual std::shared_ptr<AnnotationExpr> rebuild(std::shared_ptr<AnnotationExpr> kids[]) const {	\
		return create(kids[0], kids[1]);                                                        \
	}					                                                        \
					                                                                \
	Kind getKind() const override { return _class_kind; }	                                        \
								                                        \
	Width getWidth() const override { return getKid(0)->getWidth(); }                               \
								                                        \
	std::string getOperator() const override { return _class_operator; }                            \
									                                \
	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override;                            \
								                                        \
	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {                            \
		return alloc(getKid(0).get()->clone(tid), getKid(1).get()->clone(tid));                 \
	}                                                                                               \
								                                        \
	static bool classof(const AnnotationExpr *E) {			                                \
		return E->getKind() == AnnotationExpr::_class_kind;	                                \
	}								                                \
									                                \
	static bool classof(const _class_kind##Expr *) { return true; }	                                \
};

ARITHMETIC_EXPR_CLASS(Add, "+")

ARITHMETIC_EXPR_CLASS(Sub, "-")

ARITHMETIC_EXPR_CLASS(Mul, "*")

ARITHMETIC_EXPR_CLASS(UDiv, "/")

ARITHMETIC_EXPR_CLASS(SDiv, "//")

ARITHMETIC_EXPR_CLASS(URem, "%")

ARITHMETIC_EXPR_CLASS(SRem, "s%")

ARITHMETIC_EXPR_CLASS(And, "&")

ARITHMETIC_EXPR_CLASS(Or, "|")

ARITHMETIC_EXPR_CLASS(Xor, "^")

ARITHMETIC_EXPR_CLASS(Shl, "<<")

ARITHMETIC_EXPR_CLASS(LShr, "l>>")

ARITHMETIC_EXPR_CLASS(AShr, "a>>")

// Comparison Exprs

#define COMPARISON_EXPR_CLASS(_class_kind, _class_operator)                                                   \
class _class_kind##Expr : public CmpExpr {                                                                    \
public:									                                      \
        _class_kind##Expr(const std::shared_ptr<AnnotationExpr> &l, const std::shared_ptr<AnnotationExpr> &r) \
        : CmpExpr(l, r) {}                                                                                    \
									                                      \
	static std::shared_ptr<AnnotationExpr> alloc(const std::shared_ptr<AnnotationExpr> &l,                \
			                             const std::shared_ptr<AnnotationExpr> &r) {              \
		std::shared_ptr<AnnotationExpr> res(new _class_kind##Expr(l, r));                             \
		res->computeHash();					                                      \
		return res;						                                      \
	}								                                      \
									                                      \
	static std::shared_ptr<AnnotationExpr> create(const std::shared_ptr<AnnotationExpr> &l,               \
			                              const std::shared_ptr<AnnotationExpr> &r);              \
									                                      \
	virtual std::shared_ptr<AnnotationExpr> rebuild(std::shared_ptr<AnnotationExpr> kids[]) const {	      \
		return create(kids[0], kids[1]);			                                      \
	}								                                      \
									                                      \
	Kind getKind() const override { return _class_kind; }		                                      \
									                                      \
	std::string getOperator() const override { return _class_operator; }                                  \
									                                      \
	llvm::APInt evaluate(llvm::GenericValue symbolicVal) const override;                                  \
								                                              \
	std::shared_ptr<AnnotationExpr> clone(unsigned tid) const override {                                  \
		return alloc(getKid(0).get()->clone(tid), getKid(1).get()->clone(tid));                       \
	}                                                                                                     \
								                                              \
	static bool classof(const AnnotationExpr *E) {			                                      \
		return E->getKind() == AnnotationExpr::_class_kind;	                                      \
	}								                                      \
									                                      \
	static bool classof(const _class_kind##Expr *) { return true; }	                                      \
};

COMPARISON_EXPR_CLASS(Eq, "==")

COMPARISON_EXPR_CLASS(Ne, "!=")

COMPARISON_EXPR_CLASS(Ult, "u<")

COMPARISON_EXPR_CLASS(Ule, "u<=")

COMPARISON_EXPR_CLASS(Ugt, "u>")

COMPARISON_EXPR_CLASS(Uge, "u>=")

COMPARISON_EXPR_CLASS(Slt, "s<")

COMPARISON_EXPR_CLASS(Sle, "s<=")

COMPARISON_EXPR_CLASS(Sgt, "s>")

COMPARISON_EXPR_CLASS(Sge, "s>=")

#endif /* __ANNOTATION_EXPR_HPP__ */
