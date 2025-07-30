/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#include "Revisit.hpp"

llvm::raw_ostream &operator<<(llvm::raw_ostream &s, const Revisit::Kind k)
{
	switch (k) {
	case Revisit::RV_FRevRead:
		s << "FR";
		break;
	case Revisit::RV_FRevOpt:
		s << "OPT";
		break;
	case Revisit::RV_FRevMO:
		s << "MO";
		break;
	case Revisit::RV_BRev:
		s << "BR";
		break;
	default:
		PRINT_BUGREPORT_INFO_ONCE("print-revisit-type", "Cannot print revisit type");
		s << "UNKNOWN";
		break;
	}
	return s;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &s, const Revisit &item)
{
	switch (item.getKind()) {
	case Revisit::RV_FRevRead: {
		auto &fi = static_cast<const ReadForwardRevisit &>(item);
		s << fi.getKind() << "(" << fi.getPos() << ": " << fi.getRev() << ")";
		break;
	}
	case Revisit::RV_FRevMO: {
		auto &mi = static_cast<const WriteForwardRevisit &>(item);
		s << mi.getKind() << "(" << mi.getPos() << ": " << mi.getPred() << ")";
		break;
	}
	case Revisit::RV_FRevOpt: {
		auto &mi = static_cast<const OptionalForwardRevisit &>(item);
		s << mi.getKind() << "(" << mi.getPos() << ")";
		break;
	}
	case Revisit::RV_BRev: {
		auto &bi = static_cast<const BackwardRevisit &>(item);
		s << bi.getKind() << "(" << bi.getPos() << ": [" << bi.getRev() << ", ";
		s << *bi.getViewNoRel();
		s << "]";
		break;
	}

	default:
		s << item.getKind();
		break;
	}
	return s;
}
