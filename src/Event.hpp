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

#include <llvm/IR/Instructions.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#include <iostream>
#include <list>

enum EventType { R, W, NA };
enum EventAttr { Plain, CAS, RMW};

struct Event {
	int thread;
	int index;

//	Event() : thread(0), index(0) {}; /* Initializer event */
	Event() : thread(-42), index(-42) {};
	Event(int t, int e) : thread(t), index(e) {};
	Event(int t, int e, bool rmw) : thread(t), index(e) {};

	bool isInitializer() { return thread == 0 && index == 0; };
	Event prev() { return Event(thread, index-1); };
	Event next() { return Event(thread, index+1); };

	friend std::ostream& operator<<(std::ostream &s, const Event &e);

	inline bool operator==(const Event &e) const {
		return e.index == index && e.thread == thread;
	}
	inline bool operator!=(const Event &e) const {
		return !(*this == e);
	}
};

class EventLabel {

public:
	EventType type;
	EventAttr attr;
	llvm::AtomicOrdering ord;
	Event pos;
	llvm::GenericValue *addr;
	llvm::GenericValue val; /* For Writes */
	llvm::Type *valTyp;
	Event rf; /* For Reads */
	std::list<Event> rfm1; /* For Writes */

	EventLabel(EventType typ, Event e); /* Start */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::Type *valTyp, Event w); /* Reads */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue expected,
		   llvm::Type *valTyp, Event w);
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp, std::list<Event> rfm1); /* Writes */
	EventLabel(EventType typ, EventAttr attr, llvm::AtomicOrdering ord, Event e,
		   llvm::GenericValue *addr, llvm::GenericValue val,
		   llvm::Type *valTyp); /* Writes */

	bool isRead() const;
	bool isWrite() const;
	bool isAtLeastAcquire() const;
	bool isAtLeastRelease() const;
	bool isRMW() const;

	friend std::ostream& operator<<(std::ostream &s, const EventLabel &lab);
};

#endif /* __EVENT_HPP__ */
