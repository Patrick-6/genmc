/*
 * RCMC -- Model Checking for C11 programs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#include "Event.hpp"
#include "EventLabel.hpp"
#include "Error.hpp"
#include <llvm/IR/Instructions.h>

#include <cassert>

/* Start */
EventLabel::EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, Event tc)
	: type(typ), ord(ord), pos(e), rf(tc) {}

/* Thread Create/Join */
EventLabel::EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, int cid)
	: type(typ), ord(ord), pos(e), cid(cid) {}

/* Fence */
EventLabel::EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e)
	: type(typ), ord(ord), pos(e) {}

/* Read */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), valTyp(valTyp), rf(w) {}

/* GRead */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::Type *valTyp, Event w,
		       std::string &functionName)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), valTyp(valTyp), rf(w),
	  functionName(functionName), initial(false) {}

/* CAS+FAI Reads */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue expected,
		       llvm::GenericValue nextVal, llvm::AtomicRMWInst::BinOp op,
		       llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(expected),
	  nextVal(nextVal), op(op), valTyp(valTyp), rf(w) {}

/* CAS Read */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val,
		       llvm::GenericValue nextVal, llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val),
	  nextVal(nextVal), valTyp(valTyp), rf(w) {}

/* FAI Read */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue nextVal,
		       llvm::AtomicRMWInst::BinOp op, llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), nextVal(nextVal),
	  op(op), valTyp(valTyp), rf(w) {}

/* Store / FAI Store */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val, llvm::Type *valTyp)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val), valTyp(valTyp) {}

/* GStore */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val, llvm::Type *valTyp,
		       std::string &functionName, bool isInit)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val), valTyp(valTyp),
	  functionName(functionName), initial(isInit) {}

/* GStore */
EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val,
		       llvm::Type *valTyp, std::list<Event> rfm1, std::string &functionName)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val),
	  valTyp(valTyp), rfm1(rfm1), functionName(functionName) {}


unsigned int EventLabel::getStamp() const
{
	return stamp;
}

View& EventLabel::getHbView()
{
	return hbView;
}

View& EventLabel::getPorfView()
{
	return porfView;
}

View& EventLabel::getMsgView()
{
	return msgView;
}

bool EventLabel::isStart() const
{
	return type == EStart;
}

bool EventLabel::isFinish() const
{
	return type == EFinish;
}

bool EventLabel::isCreate() const
{
	return type == ETCreate;
}

bool EventLabel::isJoin() const
{
	return type == ETJoin;
}

bool EventLabel::isRead() const
{
	return type == ERead;
}

bool EventLabel::isWrite() const
{
	return type == EWrite;
}

bool EventLabel::isFence() const
{
	return type == EFence;
}

bool EventLabel::isNotAtomic() const
{
	return ord == llvm::NotAtomic;
}

bool EventLabel::isAtLeastAcquire() const
{
	return (ord == llvm::Acquire ||
		ord == llvm::AcquireRelease ||
		ord == llvm::SequentiallyConsistent);
	// return llvm::isAtLeastAcquire(ord);
}

bool EventLabel::isAtLeastRelease() const
{
	return (ord == llvm::Release ||
		ord == llvm::AcquireRelease ||
		ord == llvm::SequentiallyConsistent);
	// return llvm::isAtLeastRelease(ord);
}

bool EventLabel::hasReadSem() const
{
	return type == ERead || type == EStart || type == ETJoin;
}

bool EventLabel::isSC() const
{
	return (ord == llvm::SequentiallyConsistent);
}

bool EventLabel::isRMW() const
{
	return attr != Plain;
}

bool EventLabel::isLibInit() const
{
	return initial;
}

bool EventLabel::isRevisitable() const
{
	return revisitable;
}

bool EventLabel::hasBeenRevisited() const
{
	return revisited;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const llvm::AtomicOrdering &o)
{
	switch (o) {
	case llvm::NotAtomic : return s << "NA";
	case llvm::Unordered : return s << "Un";
	case llvm::Monotonic : return s << "Rlx";
	case llvm::Acquire   : return s << "Acq";
	case llvm::Release   : return s << "Rel";
	case llvm::AcquireRelease : return s << "AcqRel";
	case llvm::SequentiallyConsistent : return s << "SeqCst";
	default : return s;
	}
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventLabel &lab)
{
	return s << "EventLabel (Type: " << lab.type << "/" << lab.ord
		 << ", " << lab.pos << (lab.isRMW() ? ", " : "")
		 << lab.attr << ", HB: " << lab.hbView
		 << ", SBRF: " << lab.porfView << ")";
}
