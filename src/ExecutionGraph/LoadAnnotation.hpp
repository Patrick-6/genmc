/*
 * GenMC -- Generic Model Checking.
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
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#ifndef GENMC_LOAD_ANNOTATION_HPP
#define GENMC_LOAD_ANNOTATION_HPP

#include "ADT/value_ptr.hpp"
#include "Static/ModuleID.hpp"
#include "Support/SExpr.hpp"

#include <cstdint>

enum class AssumeType : std::uint8_t {
	User,
	Spinloop,
};

struct Annotation {
	using Expr = SExpr<ModuleID::ID>;
	using ExprVP = value_ptr<Expr, SExprCloner<ModuleID::ID>>;

	AssumeType type;
	ExprVP expr;
};

#endif /* GENMC_LOAD_ANNOTATION_HPP */
