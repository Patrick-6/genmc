/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#ifndef GENMC_REVISIT_HPP
#define GENMC_REVISIT_HPP

#include "ExecutionGraph/EventLabel.hpp"

class ReadRevisit;

/** Abstract class representing a revisit operation */
class Revisit {

public:
	/** LLVM-style RTTI discriminator */
	enum Kind {
		RV_FRev,
		RV_FRevRead,
		RV_FRevMO,
		RV_FRevOpt,
		RV_FRevRerun,
		RV_FRevLast,
		RV_BRev,
		RV_BRevLast,

	};

protected:
	/** Constructors */
	Revisit() = delete;
	Revisit(Kind k, Event p) : kind(k), pos(p) {}

public:
	/** Returns the kind of this item */
	Kind getKind() const { return kind; }

	/** Returns the event for which we are exploring an alternative exploration option */
	Event getPos() const { return pos; }

	static bool classofKind(Kind K) { return true; }
	static ReadRevisit *castToReadRevisit(const Revisit *);
	static Revisit *castFromReadRevisit(const ReadRevisit *);

	/** Destructor and printing facilities */
	virtual ~Revisit() {}
	friend llvm::raw_ostream &operator<<(llvm::raw_ostream &rhs, const Revisit &item);

private:
	Kind kind;
	Event pos;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &rhs, const Revisit::Kind k);

/** Multiple hierarchy for read revisits */
class ReadRevisit {

protected:
	ReadRevisit(Revisit::Kind k, Event rev) : revisitKind(k), rev(rev) {}

public:
	Event getRev() const { return rev; }

	Revisit::Kind getRevisitKind() const { return revisitKind; }
	static bool classof(const Revisit *r)
	{
		return r->getKind() == Revisit::RV_FRevRead ||
		       (r->getKind() >= Revisit::RV_BRev && r->getKind() <= Revisit::RV_BRevLast);
	}

private:
	Revisit::Kind revisitKind;

	/** The store revisiting the read */
	Event rev;
};

/** Represents a forward revisit */
class ForwardRevisit : public Revisit {

protected:
	ForwardRevisit() = delete;
	ForwardRevisit(Kind k, Event p) : Revisit(k, p) {}

public:
	static bool classof(const Revisit *item)
	{
		return item->getKind() >= RV_FRev && item->getKind() <= RV_FRevLast;
	}
};

/** Forward revisit a read */
class ReadForwardRevisit : public ForwardRevisit, public ReadRevisit {

public:
	/** Constructors */
	ReadForwardRevisit() = delete;
	ReadForwardRevisit(Event p, Event r, bool m = false)
		: ForwardRevisit(RV_FRevRead, p), maximal(m), ReadRevisit(RV_FRevRead, r)
	{}

	bool isMaximal() const { return maximal; }

	static bool classof(const Revisit *item) { return item->getKind() == RV_FRevRead; }
	static ReadRevisit *castToReadRevisit(const ReadForwardRevisit *r)
	{
		return static_cast<ReadRevisit *>(const_cast<ReadForwardRevisit *>(r));
	}
	static ReadForwardRevisit *castFromReadRevisit(const ReadRevisit *r)
	{
		return static_cast<ReadForwardRevisit *>(const_cast<ReadRevisit *>(r));
	}

private:
	bool maximal;
};

/** Represents an alternative MO position for a store */
class WriteForwardRevisit : public ForwardRevisit {

protected:
	WriteForwardRevisit(Kind k, Event p, Event moPred) : ForwardRevisit(k, p), moPred(moPred) {}

public:
	WriteForwardRevisit(Event p, Event moPred) : WriteForwardRevisit(RV_FRevMO, p, moPred) {}

	/** Returns the new MO predecessor of the event for which
	 * we are exploring alternative exploration options */
	Event getPred() const { return moPred; }

	static bool classof(const Revisit *item) { return item->getKind() == RV_FRevMO; }

private:
	Event moPred;
};

/** Represents the revisit of an optional block */
class OptionalForwardRevisit : public ForwardRevisit {

public:
	OptionalForwardRevisit(Event p) : ForwardRevisit(RV_FRevOpt, p) {}

	static bool classof(const Revisit *item) { return item->getKind() == RV_FRevOpt; }
};

/** Represents an execution "rerun".  Helpful e.g., when operating under EstimationMode */
class RerunForwardRevisit : public ForwardRevisit {

public:
	RerunForwardRevisit() : ForwardRevisit(RV_FRevRerun, Event::getInit()) {}

	static bool classof(const Revisit *item) { return item->getKind() == RV_FRevRerun; }
};

/** Represents a backward revisit */
class BackwardRevisit : public Revisit, public ReadRevisit {

protected:
	BackwardRevisit(Kind k, Event p, Event r, std::unique_ptr<VectorClock> view)
		: Revisit(k, p), view(std::move(view)), ReadRevisit(k, r)
	{}

public:
	BackwardRevisit(Event p, Event r, std::unique_ptr<VectorClock> view)
		: BackwardRevisit(RV_BRev, p, r, std::move(view))
	{}
	BackwardRevisit(const ReadLabel *rLab, const WriteLabel *wLab,
			std::unique_ptr<VectorClock> view)
		: BackwardRevisit(rLab->getPos(), wLab->getPos(), std::move(view))
	{}

	/** Returns (releases) the prefix of the revisiting event */
	std::unique_ptr<VectorClock> getViewRel() { return std::move(view); }

	/** Returns (but does not release) the prefix of the revisiting event */
	const std::unique_ptr<VectorClock> &getViewNoRel() const { return view; }

	static bool classof(const Revisit *item)
	{
		return item->getKind() >= RV_BRev && item->getKind() <= RV_BRevLast;
	}
	static ReadRevisit *castToReadRevisit(const BackwardRevisit *r)
	{
		return static_cast<ReadRevisit *>(const_cast<BackwardRevisit *>(r));
	}
	static BackwardRevisit *castFromReadRevisit(const ReadRevisit *r)
	{
		return static_cast<BackwardRevisit *>(const_cast<ReadRevisit *>(r));
	}

private:
	std::unique_ptr<VectorClock> view;
};

/*******************************************************************************
 **                             Static methods
 *******************************************************************************/

inline Revisit *Revisit::castFromReadRevisit(const ReadRevisit *r)
{
	auto rk = r->getRevisitKind();
	switch (rk) {
	case Revisit::Kind::RV_FRevRead:
		return static_cast<ReadForwardRevisit *>(const_cast<ReadRevisit *>(r));
	case Revisit::Kind::RV_BRev:
		return static_cast<BackwardRevisit *>(const_cast<ReadRevisit *>(r));
	default:
		BUG();
	}
}

inline ReadRevisit *Revisit::castToReadRevisit(const Revisit *r)
{
	auto rk = r->getKind();
	switch (rk) {
	case Revisit::Kind::RV_FRevRead:
		return static_cast<ReadForwardRevisit *>(const_cast<Revisit *>(r));
	case Revisit::Kind::RV_BRev:
		return static_cast<BackwardRevisit *>(const_cast<Revisit *>(r));
	default:
		BUG();
	}
}

/*******************************************************************************
 **                             RTTI helpers
 *******************************************************************************/

/** Specialization selected when ToTy is not a known subclass of ReadRevisit */
template <class ToTy, bool IsKnownSubtype = ::std::is_base_of<ReadRevisit, ToTy>::value>
struct cast_convert_read_rev {
	static const ToTy *doit(const ReadRevisit *Val)
	{
		return static_cast<const ToTy *>(Revisit::castFromReadRevisit(Val));
	}

	static ToTy *doit(ReadRevisit *Val)
	{
		return static_cast<ToTy *>(Revisit::castFromReadRevisit(Val));
	}
};

/** Specialization selected when ToTy is a known subclass of ReadRevisit */
template <class ToTy> struct cast_convert_read_rev<ToTy, true> {
	static const ToTy *doit(const ReadRevisit *Val) { return static_cast<const ToTy *>(Val); }

	static ToTy *doit(ReadRevisit *Val) { return static_cast<ToTy *>(Val); }
};

namespace llvm {

/** isa<T>(ReadRevisit *) */
template <typename To> struct isa_impl<To, ::ReadRevisit> {
	static bool doit(const ::ReadRevisit &Val) { return To::classofKind(Val.getRevisitKind()); }
};

/** cast<T>(ReadRevisit *) */
template <class ToTy> struct cast_convert_val<ToTy, const ::ReadRevisit, const ::ReadRevisit> {
	static const ToTy &doit(const ::ReadRevisit &Val)
	{
		return *::cast_convert_read_rev<ToTy>::doit(&Val);
	}
};

template <class ToTy> struct cast_convert_val<ToTy, ::ReadRevisit, ::ReadRevisit> {
	static ToTy &doit(::ReadRevisit &Val) { return *::cast_convert_read_rev<ToTy>::doit(&Val); }
};

template <class ToTy> struct cast_convert_val<ToTy, const ::ReadRevisit *, const ::ReadRevisit *> {
	static const ToTy *doit(const ::ReadRevisit *Val)
	{
		return ::cast_convert_read_rev<ToTy>::doit(Val);
	}
};

template <class ToTy> struct cast_convert_val<ToTy, ::ReadRevisit *, ::ReadRevisit *> {
	static ToTy *doit(::ReadRevisit *Val) { return ::cast_convert_read_rev<ToTy>::doit(Val); }
};

/// Implement cast_convert_val for EventLabel -> ReadRevisit conversions.
template <class FromTy> struct cast_convert_val<::ReadRevisit, FromTy, FromTy> {
	static ::ReadRevisit &doit(const FromTy &Val) { return *FromTy::castToReadRevisit(&Val); }
};

template <class FromTy> struct cast_convert_val<::ReadRevisit, FromTy *, FromTy *> {
	static ::ReadRevisit *doit(const FromTy *Val) { return FromTy::castToReadRevisit(Val); }
};

template <class FromTy> struct cast_convert_val<const ::ReadRevisit, FromTy, FromTy> {
	static const ::ReadRevisit &doit(const FromTy &Val)
	{
		return *FromTy::castToReadRevisit(&Val);
	}
};

template <class FromTy> struct cast_convert_val<const ::ReadRevisit, FromTy *, FromTy *> {
	static const ::ReadRevisit *doit(const FromTy *Val)
	{
		return FromTy::castToReadRevisit(Val);
	}
};

} /* namespace llvm */

#endif /* GENMC_REVISIT_HPP */
