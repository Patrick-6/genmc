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

#ifndef __EVENT_HPP__
#define __EVENT_HPP__

#include "View.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

#include <list>

enum EventType { EStart, EFinish, ETCreate, ETJoin, ERead, EWrite, EFence };
enum EventAttr { Plain, CAS, RMW};

struct Event {
	int thread;
	int index;

//	Event() : thread(-42), index(-42) {}; /* Initializer event */
	Event() : thread(-17), index(-17) {};
	Event(int t, int e) : thread(t), index(e) {};
	Event(int t, int e, bool rmw) : thread(t), index(e) {};

	static Event getInitializer() { return Event(0, 0); };

	bool isInitializer() const { return *this == getInitializer(); };
	bool isInitializer()       { return *this == getInitializer(); };
	Event prev() { return Event(thread, index-1); };
	Event next() { return Event(thread, index+1); };

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const Event &e);

	inline bool operator==(const Event &e) const {
		return e.index == index && e.thread == thread;
	}
	inline bool operator!=(const Event &e) const {
		return !(*this == e);
	}
	inline bool operator<(const Event &e) const {
		return (index < e.index) || (index == e.index && thread < e.thread);
	}
	inline bool operator>(const Event &e) const {
		return (index > e.index) || (index == e.index && thread > e.thread);
	}
};

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

	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, Event tc); /* Start */
	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e, int cid); /* Thread Create */
	EventLabel(EventType typ, llvm::AtomicOrdering ord, Event e); /* Fences */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::Type *valTyp, Event w); /* Reads */
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

	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventLabel &lab);
};

#endif /* __EVENT_HPP__ */
