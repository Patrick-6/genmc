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

#ifndef __MEMORY_MODEL_HPP__
#define __MEMORY_MODEL_HPP__

#include "Error.hpp"

#include <cstdint>
#include <string>

enum class ModelType : std::uint8_t { SC, RA, RC11, IMM };

inline llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const ModelType &m)
{
	switch (m) {
	case ModelType::SC:
		return s << "SC";
	case ModelType::RA:
		return s << "RA";
	case ModelType::RC11:
		return s << "RC11";
	case ModelType::IMM:
		return s << "IMM";
	default:
		PRINT_BUGREPORT_INFO_ONCE("missing-model-name", "Unknown memory model name");
		return s;
	}
}

#endif /* __MEMORY_MODEL_HPP__ */
