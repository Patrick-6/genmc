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
#include "Error.hpp"
#include <llvm/IR/Instructions.h>

#include <cassert>

EventLabel::EventLabel(EventType typ, Event e)
	: type(typ), pos(e) {}

EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), valTyp(valTyp), rf(w) {}

EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val, llvm::Type *valTyp)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val), valTyp(valTyp) {}

EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val,
		       llvm::Type *valTyp, Event w)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val),
	  valTyp(valTyp), rf(w) {}

EventLabel::EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		       llvm::GenericValue *addr, llvm::GenericValue val,
		       llvm::Type *valTyp, std::list<Event> rfm1)
	: type(typ), attr(attr), ord(ord), pos(e), addr(addr), val(val),
	  valTyp(valTyp), rfm1(rfm1) {}


bool EventLabel::isRead() const
{
	return type == R;
}

bool EventLabel::isWrite() const
{
	return type == W;
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

bool EventLabel::isRMW() const
{
	return attr != Plain;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventType &t)
{
	switch (t) {
	case R : return s << "R";
	case W : return s << "W";
	case NA : return s << "NA";
	}
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventAttr &a)
{
	switch (a) {
	case Plain : return s << "";
	case CAS : return s << "CAS";
	case RMW : return s << "RMW";
	}
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const Event &e)
{
	return s << "Event (" << e.thread << ", " << e.index << ")";
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventLabel &lab)
{
	return s << "EventLabel (Type: " << lab.type << ", "
		 << lab.pos << (lab.isRMW() ? ", " : "")
		 << lab.attr << ")";
}
