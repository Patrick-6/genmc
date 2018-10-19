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

#ifndef __EVENTLABEL_HPP__
#define __EVENTLABEL_HPP__

#include "Event.hpp"
#include "RevisitSet.hpp"
#include "View.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

#include <list>

class EventLabel {

public:
	EventType type;
	EventAttr attr;
	llvm::AtomicOrdering ord;
	Event pos;
	llvm::GenericValue *addr;
	llvm::GenericValue val; /* For Writes and CASs */
	llvm::GenericValue nextVal; /* For CASs and FAIs */
	llvm::AtomicRMWInst::BinOp op;
	llvm::Type *valTyp;
	Event rf; /* For Reads */
	std::list<Event> rfm1; /* For Writes */
	View msgView;
	View hbView;
	int cid; /* For TCreates */
	std::string functionName; /* For GReads/GWrites */
	bool bottom; /* For GWrites */
	RevisitType revType;
	RevisitSet revs;
	View loadPreds;
	std::vector<Event> invalidRfs;

	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, Event tc); /* Start */
	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, int cid); /* Thread Create */
	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e); /* Fences */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::Type *valTyp, Event w); /* Reads */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::Type *valTyp, Event w,
		   std::string &functionName); /* GReads */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue nextVal,
		   llvm::AtomicRMWInst::BinOp op, llvm::Type *valTyp, Event w); /* FAI Reads */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue expected,
		   llvm::GenericValue nextVal, llvm::Type *valTyp, Event w); /* CAS Reads */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp, std::list<Event> rfm1); /* Writes */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp); /* Writes */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp, std::string &functionName); /* GWrites */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp, std::list<Event> rfm1,
		   std::string &functionName); /* GWrites */

	bool isStart() const;
	bool isFinish() const;
	bool isCreate() const;
	bool isJoin() const;
	bool isRead() const;
	bool isWrite() const;
	bool isFence() const;
	bool isNotAtomic() const;
	bool isAtLeastAcquire() const;
	bool isAtLeastRelease() const;
	bool isSC() const;
	bool isRMW() const;
	bool isBottom() const;

	bool isRevisitable()      { return revType == Normal || revType == RMWConflict; };
	void makeNotRevisitable() { revType = NotRevisitable; };
	void makeRevisitable()    { revType = Normal; };

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventLabel &lab);
};

#endif /* #define __EVENTLABEL_HPP__ */
