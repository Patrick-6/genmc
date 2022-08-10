/*
 * KATER -- Automating Weak Memory Model Metatheory
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
 */

#ifndef KATER_COUNTEREXAMPLE_HPP
#define KATER_COUNTEREXAMPLE_HPP

#include "TransLabel.hpp"
#include <iostream>
#include <vector>

class Counterexample {

private:
	using ContainerT = std::vector<TransLabel>;

public:
	enum Type { NONE, TUT, ANA, };

	using iterator = ContainerT::iterator;
	using const_iterator = ContainerT::const_iterator;

	Counterexample() = default;

	iterator begin() { return cex.begin(); }
	iterator end() { return cex.end(); }
	const_iterator begin() const { return cex.begin(); }
	const_iterator end() const { return cex.end(); }

	bool empty() const { return cex.empty(); }
	bool size() const { return cex.size(); }

	void extend(const TransLabel &lab) { cex.push_back(lab); }

	unsigned int getMismatch() const { return index; }

	Type getType() const { return typ; }

	void setType(Type t) {
		typ = t;
		if (t == TUT) {
			assert(!cex.empty());
			index = cex.size()-1;
		}
	}

	friend std::ostream &operator<<(std::ostream& ostr,
					const Counterexample &c);

private:
	ContainerT cex;
	unsigned int index;
	Type typ;
};

#endif /* KATER_COUNTEREXAMPLE_HPP */
