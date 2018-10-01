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

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventType &t)
{
	switch (t) {
	case ERead    : return s << "R";
	case EWrite   : return s << "W";
	case EFence   : return s << "F";
	case EStart   : return s << "B";
	case EFinish  : return s << "E";
	case ETCreate : return s << "C";
	case ETJoin   : return s << "J";
	default : return s;
	}
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const EventAttr &a)
{
	switch (a) {
	case Plain : return s;
	case CAS : return s << "CAS";
	case RMW : return s << "RMW";
	default : return s;
	}
}

llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const Event &e)
{
	if (e.isInitializer())
		return s << "INIT";
	return s << "\"Event (" << e.thread << ", " << e.index << ")\"";
}
