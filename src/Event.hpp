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

struct Event {
	int threadIndex;
	int eventIndex;

	Event() : threadIndex(0), eventIndex(0) {}; /* Initializer event */
	Event(int t, int e) : threadIndex(t), eventIndex(e) {};
	Event(int t, int e, bool rmw) : threadIndex(t), eventIndex(e) {};

	bool isInitializer() { return threadIndex == 0 && eventIndex == 0; };

	friend std::ostream& operator<<(std::ostream &s, const Event &e);

	inline bool operator==(const Event &e) const {
		return e.threadIndex == threadIndex &&
			e.eventIndex == eventIndex;
	}
	inline bool operator!=(const Event &e) const {
		return !(*this == e);
	}
};

class EventLabel {
	
public:
	EventType type;
	Event pos;
	llvm::GenericValue *addr;
	llvm::GenericValue val; /* For Writes */
	llvm::Type *valTyp;
	Event rf; /* For Reads */
	std::list<Event> rfm1; /* For Writes */
	bool isRMW;

	EventLabel(EventType typ, Event e); /* Start */
	EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		   llvm::Type *valTyp, Event w, bool rmw); /* Reads */
	EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		   llvm::GenericValue expected, llvm::Type *valTyp,
		   Event w, bool rmw);
	EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		   llvm::GenericValue val, llvm::Type *valTyp,
		   std::list<Event> rfm1, bool rmw); /* Writes */
	EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		   llvm::GenericValue val, llvm::Type *valTyp, bool rmw); /* Writes */

	friend std::ostream& operator<<(std::ostream &s, const EventLabel &lab);
};

#endif /* __EVENT_HPP__ */
