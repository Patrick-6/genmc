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

#include <iostream>
#include <cassert>

EventLabel::EventLabel(EventType typ, Event e)
	: type(typ), pos(e), isRMW(false) {}

EventLabel::EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		       Event w, bool rmw)
	: type(typ), pos(e), addr(addr), rf(w), isRMW(rmw) {}

EventLabel::EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		       llvm::GenericValue val, bool rmw)
	: type(typ), pos(e), addr(addr), val(val), isRMW(rmw) {}

EventLabel::EventLabel(EventType typ, Event e, llvm::GenericValue *addr,
		       llvm::GenericValue val, std::list<Event> rfm1, bool rmw)
	: type(typ), pos(e), addr(addr), val(val), rfm1(rfm1), isRMW(rmw) {}
	

std::ostream& operator<<(std::ostream &s, const EventType &t)
{
	switch (t) {
	case 0 : return s << "R";
	case 1 : return s << "W";
	case 2 : return s << "NA";
	}
}

std::ostream& operator<<(std::ostream &s, const Event &e)
{
	return s << "Event (Thread: " << e.threadIndex << ", Index: " << e.eventIndex << ")";
}

std::ostream& operator<<(std::ostream &s, const EventLabel &lab)
{
	return s << "EventLabel (Type: " << lab.type << ", "
		 << lab.pos << (lab.isRMW ? ", RMW" : "") << ")";
}
