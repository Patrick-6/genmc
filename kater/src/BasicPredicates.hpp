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

#ifndef KATER_BASIC_PREDICATES_HPP
#define KATER_BASIC_PREDICATES_HPP

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <utility>

// Stored as reversed bitmap
// FIXME: many masks do not compose; representation does not scale
enum PredicateMask : unsigned long long {
	True        = 0ull,
	False       = ~0ull,
	Rna         = ~(1ull << 0),
	NRrlx       = ~(1ull << 1),
	NRacq       = ~(1ull << 2),
	NRsc        = ~(1ull << 3),
	URrlx       = ~(1ull << 4),
	URacq       = ~(1ull << 5),
	URsc        = ~(1ull << 6),
	DR          = ~(1ull << 7),
	Wna         = ~(1ull << 8),
	NWrlx       = ~(1ull <<  9),
	NWrel       = ~(1ull << 10),
	NWsc        = ~(1ull << 11),
	UWrlx       = ~(1ull << 12),
	UWrel       = ~(1ull << 13),
	UWsc        = ~(1ull << 14),
	DW          = ~(1ull << 15),
	Facq        = ~(1ull << 16),
	Frel        = ~(1ull << 17),
	Facqrel     = ~(1ull << 18),
	Fsc         = ~(1ull << 19),
	Fwmb        = ~(1ull << 20),
	Frmb        = ~(1ull << 21),
	Fba         = ~(1ull << 22),
	Faa         = ~(1ull << 23),
	Fas         = ~(1ull << 24),
	Faul        = ~(1ull << 25),
	TC          = ~(1ull << 26),
	TJ          = ~(1ull << 27),
	TK          = ~(1ull << 28),
	TB          = ~(1ull << 29),
	TE          = ~(1ull << 30),
	Alloc       = ~(1ull << 31),
	Free        = ~(1ull << 32),
	HpRetire    = ~(1ull << 33),
	HpProtect   = ~(1ull << 34),
	LR          = ~(1ull << 35),
	LW          = ~(1ull << 36),
	UL          = ~(1ull << 37),
	RCUSync     = ~(1ull << 38),
	RCULock     = ~(1ull << 39),
	RCUUnlock   = ~(1ull << 40),
	DskOpen     = ~(1ull << 41),
	DskFsync    = ~(1ull << 42),
	DskSync     = ~(1ull << 43),
	DskPbarrier = ~(1ull << 44),
	CLFlush     = ~(1ull << 45),
	CLFlushOpt  = ~(1ull << 46),
	IR          = ~(1ull << 47),
	NA          = Rna   & Wna,
	NR          = Rna   & NRrlx & NRacq & NRsc,
	UR          = URrlx & URacq & URsc,
	R           = NR    & UR    & DR    & IR,
	Rrlx        = NRrlx & URrlx,
	Racq        = NRacq & URacq,
	Rsc         = NRsc  & URsc,
	Rmarked     = Rrlx  & Racq  & Rsc,
	NW          = Wna   & NWrlx & NWrel & NWsc,
	UW          = UWrlx & UWrel & UWsc,
	Wrlx        = NWrlx & UWrlx,
	Wrel        = NWrel & UWrel,
	Wsc         = NWsc  & UWsc,
	Wmarked     = Wrlx  & Wrel & Wsc,
	Marked      = Rmarked & Wmarked,
	W           = NW    & UW   & DW,
	MemAccess   = R     & W,
	F           = Frel  & Facq & Facqrel & Fsc,
	Flkmm       = Fwmb  & Frmb & Fba  & Faa & Fas & Faul,
	Acq         = Racq  & Rsc  & Facq & Facqrel & Fsc & TJ & TB,
	Rel         = Wrel  & Wsc  & Frel & Facqrel & Fsc & TC & TE,
	SC          = Rsc   & Wsc  & Fsc,
	D           = DR    & DW,
	U           = UR    & UW,
	Dep         = R     & Alloc,
	Loc         = Alloc & Free & HpRetire & MemAccess,
};

#endif /* KATER_BASIC_PREDICATES_HPP */
