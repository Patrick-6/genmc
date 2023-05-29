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

#include "Config.hpp"
#include "Error.hpp"
#include "Printer.hpp"
#include "Utils.hpp"

std::vector<unsigned> Printer::calcToIdxMap;

const char *genmcCopyright =
R"(/*
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
)";

const char *katerNotice =
R"(/*******************************************************************************
 * CAUTION: This file is generated automatically by Kater -- DO NOT EDIT.
 *******************************************************************************/
)";

const char *coherenceFuns =
R"(
bool #CLASS#::isWriteRfBefore(Event a, Event b)
{
	auto &g = getGraph();
	auto &before = g.getEventLabel(b)->view(#HB#);
	if (before.contains(a))
		return true;

	const EventLabel *lab = g.getEventLabel(a);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (auto &r : wLab->getReadersList())
		if (before.contains(r))
			return true;
	return false;
}

std::vector<Event>
#CLASS#::getInitRfsAtLoc(SAddr addr)
{
	std::vector<Event> result;

	for (const auto *lab : labels(getGraph())) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
			if (rLab->getRf().isInitializer() && rLab->getAddr() == addr)
				result.push_back(rLab->getPos());
	}
	return result;
}

bool #CLASS#::isHbOptRfBefore(const Event e, const Event write)
{
	auto &g = getGraph();
	const EventLabel *lab = g.getEventLabel(write);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *sLab = static_cast<const WriteLabel *>(lab);
	if (sLab->view(#HB#).contains(e))
		return true;

	for (auto &r : sLab->getReadersList()) {
		if (g.getEventLabel(r)->view(#HB#).contains(e))
			return true;
	}
	return false;
}

int #CLASS#::splitLocMOBefore(SAddr addr, Event e)
{
	const auto &g = getGraph();
	auto rit = std::find_if(store_rbegin(g, addr), store_rend(g, addr), [&](const Event &s){
		return isWriteRfBefore(s, e);
	});
	return (rit == store_rend(g, addr)) ? 0 : std::distance(rit, store_rend(g, addr));
}

int #CLASS#::splitLocMOAfterHb(SAddr addr, const Event read)
{
	const auto &g = getGraph();

	auto initRfs = g.getInitRfsAtLoc(addr);
	if (std::any_of(initRfs.begin(), initRfs.end(), [&read,&g](const Event &rf){
		return g.getEventLabel(rf)->view(#HB#).contains(read);
	}))
		return 0;

	auto it = std::find_if(store_begin(g, addr), store_end(g, addr), [&](const Event &s){
		return isHbOptRfBefore(read, s);
	});
	if (it == store_end(g, addr))
		return std::distance(store_begin(g, addr), store_end(g, addr));
	return (g.getEventLabel(*it)->view(#HB#).contains(read)) ?
		std::distance(store_begin(g, addr), it) : std::distance(store_begin(g, addr), it) + 1;
}

int #CLASS#::splitLocMOAfter(SAddr addr, const Event e)
{
	const auto &g = getGraph();
	auto it = std::find_if(store_begin(g, addr), store_end(g, addr), [&](const Event &s){
		return isHbOptRfBefore(e, s);
	});
	return (it == store_end(g, addr)) ? std::distance(store_begin(g, addr), store_end(g, addr)) :
		std::distance(store_begin(g, addr), it);
}

std::vector<Event>
#CLASS#::getCoherentStores(SAddr addr, Event read)
{
	auto &g = getGraph();
	std::vector<Event> stores;

	/*
	 * If there are no stores (rf?;hb)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (rf?;hb)-before stores.
	 */
	auto begO = splitLocMOBefore(addr, read);
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(store_begin(g, addr) + begO - 1));

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endO = (isDepTracking()) ? splitLocMOAfterHb(addr, read) :
		std::distance(store_begin(g, addr), store_end(g, addr));
	stores.insert(stores.end(), store_begin(g, addr) + begO, store_begin(g, addr) + endO);
	return stores;
}

std::vector<Event>
#CLASS#::getMOOptRfAfter(const WriteLabel *sLab)
{
	std::vector<Event> after;

	auto &g = getGraph();
	std::for_each(co_succ_begin(g, sLab->getAddr(), sLab->getPos()),
		      co_succ_end(g, sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = g.getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});
	return after;
}

std::vector<Event>
#CLASS#::getMOInvOptRfAfter(const WriteLabel *sLab)
{
	auto &g = getGraph();
	std::vector<Event> after;

	/* First, add (mo;rf?)-before */
	std::for_each(co_pred_begin(g, sLab->getAddr(), sLab->getPos()),
		      co_pred_end(g, sLab->getAddr(), sLab->getPos()), [&](const Event &w){
			      auto *wLab = g.getWriteLabel(w);
			      after.push_back(wLab->getPos());
			      after.insert(after.end(), wLab->readers_begin(), wLab->readers_end());
	});

	/* Then, we add the reader list for the initializer */
	auto initRfs = g.getInitRfsAtLoc(sLab->getAddr());
	after.insert(after.end(), initRfs.begin(), initRfs.end());
	return after;
}

std::vector<Event>
#CLASS#::getCoherentRevisits(const WriteLabel *sLab, const VectorClock &pporf)
{
	auto &g = getGraph();
	auto ls = g.getRevisitable(sLab, pporf);

	/* If this store is po- and mo-maximal then we are done */
	if (!isDepTracking() && g.isCoMaximal(sLab->getAddr(), sLab->getPos()))
		return ls;

	/* First, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto optRfs = getMOOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ const View &before = g.getEventLabel(e)->view(#HB#);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&](Event ev)
					 { return before.contains(ev); });
				}), ls.end());

	/* If out-of-order event addition is not supported, then we are done
	 * due to po-maximality */
	if (!isDepTracking())
		return ls;

	/* Otherwise, we also have to exclude hb-before loads */
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
		{ return g.getEventLabel(sLab->getPos())->view(#HB#).contains(e); }),
		ls.end());

	/* ...and also exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in
	 * the resulting graph */
	auto &before = pporf;
	auto moInvOptRfs = getMOInvOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ auto *eLab = g.getEventLabel(e);
				  auto v = g.getViewFromStamp(eLab->getStamp());
				  v->update(before);
				  return std::any_of(moInvOptRfs.begin(),
						     moInvOptRfs.end(),
						     [&](Event ev)
						     { return v->contains(ev) &&
						       g.getEventLabel(ev)->view(#HB#).contains(e); });
				}),
		 ls.end());

	return ls;
}

std::pair<int, int>
#CLASS#::getCoherentPlacings(SAddr addr, Event store, bool isRMW)
{
	const auto &g = getGraph();
	auto *cc = g.getCoherenceCalculator();

	/* If it is an RMW store, there is only one possible position in MO */
	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(store.prev()))) {
			auto offset = cc->getStoreOffset(addr, rLab->getRf()) + 1;
			return std::make_pair(offset, offset);
		}
		BUG();
	}

	/* Otherwise, we calculate the full range and add the store */
	auto rangeBegin = splitLocMOBefore(addr, store);
	auto rangeEnd = (isDepTracking()) ? splitLocMOAfter(addr, store) :
		cc->getStoresToLoc(addr).size();
	return std::make_pair(rangeBegin, rangeEnd);

})";

const std::unordered_map<Relation::Builtin, Printer::RelationOut> Printer::relationNames = {
        /* po */
        {Relation::po_imm,	{"po_imm_succ",      "po_imm_pred"}},
        {Relation::po_loc_imm,	{"poloc_imm_succs",  "poloc_imm_preds"}},
	/* deps */
        {Relation::ctrl_imm,	{"?",                "ctrl_preds"}},
        {Relation::addr_imm,	{"?",                "addr_preds"}},
        {Relation::data_imm,	{"?",                "data_preds"}},
	/* same thread */
	{Relation::same_thread,	{ "same_thread",     "same_thread"}},
	/* same location */
        {Relation::alloc,	{ "?",               "allocs"}},
        {Relation::frees,	{ "?",               "frees"}},
        {Relation::loc_overlap,	{ "?",               "samelocs"}},
	/* rf, co, fr, detour */
        {Relation::rf,		{ "rf_succs",        "rf_preds"}},
        {Relation::rfe,		{ "rfe_succs",       "rfe_preds"}},
        {Relation::rfi,		{ "rfi_succs",       "rfi_preds"}},
        {Relation::tc,		{ "tc_succs",        "tc_preds"}},
        {Relation::tj,		{ "tj_succs",        "tj_preds"}},
        {Relation::mo_imm,	{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::moe,		{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::moi,		{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::fr_imm,	{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::fre,		{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::fri,		{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::detour,	{ "detour_succs",    "detour_preds"}},
};

Printer::Printer(const std::string &dirPrefix, const std::string &outPrefix)
{
	auto name = outPrefix != "" ? outPrefix : "Demo";

	/* Construct all the names to be used */
	std::transform(name.begin(), name.end(), std::back_inserter(prefix), ::toupper);
	className = prefix + "Checker";
	guardName = std::string("__") + prefix + "_CHECKER_HPP__";

	/* Open required streams */
	if (outPrefix != "") {
		foutHpp = openFileForWriting(dirPrefix + "/" + className + ".hpp");
		outHpp = &foutHpp;
		foutCpp = openFileForWriting(dirPrefix + "/" + className + ".cpp");
		outCpp = &foutCpp;
	}
}

std::size_t replaceAll(std::string& inout, const std::string &what, const std::string &with)
{
	std::size_t count{};
	for (std::string::size_type pos{};
	     inout.npos != (pos = inout.find(what.data(), pos, what.length()));
	     pos += with.length(), ++count) {
		inout.replace(pos, what.length(), with.data(), with.length());
	}
	return count;
}

void Printer::printPredSet(std::ostream &ostr, const std::string &arg, const PredicateSet &preds)
{
	auto s = preds.toGenmcString();
	if (s == "")
		return;
	replaceAll(s, "#", arg);
	ostr << "if (" << s << ")";
}

void Printer::printRelation(std::ostream& ostr, const std::string &res,
			    const std::string &arg, const TransLabel *r)
{
	if (r->isPredicate()) {
		ostr << "if (auto " << res << " = " << arg << "->getPos(); true)";
		return;
	}

	if (r->isBuiltin()) {
		const auto &outs = relationNames.find(r->getRelation()->toBuiltin())->second;
		const auto &s = r->getRelation()->isInverse() ? outs.pred : outs.succ;
		// if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
		// 	ostr << "\tif (auto " << res << " = " << s << ") {\n";
		// else
		if (r->getRelation()->toBuiltin() == Relation::Builtin::po_imm)
			ostr << "if (auto " << res << " = " << s << "(g, " << arg << "->getPos()); !" << res << ".isInitializer())";
		else
			ostr << "for (auto &" << res << " : " << s << "(g, " << arg << "->getPos()))";
		return;
	}

	auto index = getCalcIdx(r->getCalcIndex());
	if (viewCalcs.count(r->getCalcIndex())) {
		ostr << "FOREACH_MAXIMAL(" << res << ", " << arg << "->view(" << index << "))";
		return;
	}
	ostr << "for (auto &" << res << " : " << arg << "->calculated(" << index << "))";
	return;
}

void Printer::printTransLabel(const TransLabel *t, const std::string &res, const std::string &arg)
{
	printPredSet(cpp(), arg, t->getPreChecks());
	printRelation(cpp(), res, arg, t);
	printPredSet(cpp(), "g.getEventLabel(" + res + ")", t->getPostChecks());
}

void Printer::printHppHeader()
{
	hpp() << genmcCopyright << "\n"
	      << katerNotice << "\n";

        hpp() << "#ifndef " << guardName << "\n"
	      << "#define " << guardName << "\n"
	      << "\n"
	      << "#include \"config.h\"\n"
	      << "#include \"ExecutionGraph.hpp\"\n"
	      << "#include \"GraphIterators.hpp\"\n"
	      << "#include \"MaximalIterator.hpp\"\n"
	      << "#include \"PersistencyChecker.hpp\"\n"
	      << "#include \"VerificationError.hpp\"\n"
	      << "#include \"VSet.hpp\"\n"
	      << "#include <cstdint>\n"
	      << "#include <vector>\n"
	      << "\n"
	      << "class " << className << " {\n"
	      << "\n"
	      << "private:\n"
	      << "\tenum class NodeStatus : unsigned char { unseen, entered, left };\n"
	      << "\n"
	      << "\tstruct NodeCountStatus {\n"
	      << "\t\tNodeCountStatus() = default;\n"
	      << "\t\tNodeCountStatus(uint16_t c, NodeStatus s) : count(c), status(s) {}\n"
	      << "\t\tuint16_t count = 0;\n"
	      << "\t\tNodeStatus status = NodeStatus::unseen;\n"
	      << "\t};\n"
	      << "\n"
	      << "public:\n"
	      << "\t" << className << "(ExecutionGraph &g) : g(g) {}\n"
	      << "\n"
	      << "\tstd::vector<VSet<Event>> calculateSaved(const Event &e);\n"
	      << "\tstd::vector<View> calculateViews(const Event &e);\n"
	      << "\tbool isDepTracking();\n"
	      << "\tbool isConsistent(const Event &e);\n"
	      << "\tVerificationError checkErrors(const Event &e);\n"
	      << "\tbool isRecoveryValid(const Event &e);\n"
	      << "\tstd::unique_ptr<VectorClock> getPPoRfBefore(const Event &e);\n"
	      << "\tconst View &getHbView(const Event &e);\n"
	      << "\tstd::vector<Event> getCoherentStores(SAddr addr, Event read);\n"
	      << "\tstd::vector<Event> getCoherentRevisits(const WriteLabel *sLab, const VectorClock &pporf);\n"
	      << "\tstd::pair<int, int> getCoherentPlacings(SAddr addr, Event store, bool isRMW);\n"
	      << "\n"
	      << "private:\n"
	      << "\tbool isWriteRfBefore(Event a, Event b);\n"
	      << "\tstd::vector<Event> getInitRfsAtLoc(SAddr addr);\n"
	      << "\tbool isHbOptRfBefore(const Event e, const Event write);\n"
	      << "\tint splitLocMOBefore(SAddr addr, Event e);\n"
	      << "\tint splitLocMOAfterHb(SAddr addr, const Event read);\n"
	      << "\tint splitLocMOAfter(SAddr addr, const Event e);\n"
	      << "\tstd::vector<Event> getMOOptRfAfter(const WriteLabel *sLab);\n"
	      << "\tstd::vector<Event> getMOInvOptRfAfter(const WriteLabel *sLab);\n"
	      << "\n";
}

void Printer::printCppHeader()
{
	cpp() << genmcCopyright << "\n"
	      << katerNotice << "\n";

	cpp() << "#include \"" << className << ".hpp\"\n"
	      << "\n";
}

void Printer::printHppFooter()
{
	hpp() << "\tstd::vector<VSet<Event>> saved;\n"
	      << "\tstd::vector<View> views;\n"
	      << "\n"
	      << "\tExecutionGraph &g;\n"
	      << "\n"
	      << "\tExecutionGraph &getGraph() { return g; }\n"
	      << "\tconst ExecutionGraph &getGraph() const { return g; }\n"
	      << "};\n"
	      << "\n"
	      << "#endif /* " << guardName << " */\n";
}

void Printer::printCppFooter()
{
}

void Printer::outputHpp(const CNFAs &cnfas)
{
	printHppHeader();

	auto i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		printCalculatorHpp(nfaStatus.first, i++, nfaStatus.second);
	});

	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &kv){
		auto &nfaPair = kv.first;
		printInclusionHpp(nfaPair.lhs, nfaPair.rhs, i++, (kv.second == -1 ? std::nullopt : std::make_optional(kv.second)));
	});

	printAcyclicHpp(cnfas.getAcyclic());

	printRecoveryHpp(cnfas.getRecovery());

	printPPoRfHpp(cnfas.getPPoRf().first, cnfas.getPPoRf().second);

	printHppFooter();
}

void Printer::outputCpp(const CNFAs &cnfas)
{
	printCppHeader();

	/* Print calculators + calculateSaved() + calculateViews()*/
	auto i = 0u;
	auto vc = 0u;
	auto sc = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		calcToIdxMap.push_back((nfaStatus.second == VarStatus::View) ? vc++ : sc++);
		if (nfaStatus.second == VarStatus::View)
			viewCalcs.insert(i);
		printCalculatorCpp(nfaStatus.first, i++, nfaStatus.second);
	});
	cpp() << "std::vector<VSet<Event>> " << className << "::calculateSaved(const Event &e)\n"
	      << "{\n";
	i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		if (nfaStatus.second != VarStatus::View)
			cpp() << "\tsaved.push_back(calculate" << i << "(e));\n";
		++i;
	});
	cpp() << "\treturn std::move(saved);\n"
	      << "}\n"
	      << "\n";
	cpp() << "std::vector<View> " << className << "::calculateViews(const Event &e)\n"
	      << "{\n";
	i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		if (nfaStatus.second == VarStatus::View)
			cpp() << "\tviews.push_back(calculate" << i << "(e));\n";
		++i;
	});
	cpp() << "\treturn std::move(views);\n"
	      << "}\n"
	      << "\n";

	cpp() << "bool " << className << "::isDepTracking()\n"
	      << "{\n"
	      << "\treturn " << cnfas.isDepTracking() << ";\n"
	      << "}\n"
	      << "\n";

	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &kv){
		auto &nfaPair = kv.first;
		printInclusionCpp(nfaPair.lhs, nfaPair.rhs, i++, (kv.second == -1 ? std::nullopt : std::make_optional(kv.second)));
	});
	cpp() << "VerificationError " << className << "::checkErrors(const Event &e)\n"
	      << "{";
	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &kv){
		auto &nfaPair = kv.first;
		cpp() << "\n\tif (!checkInclusion" << i << "(e))\n"
		      << "\n\t\t return VerificationError::" << nfaPair.s << ";";
		++i;
	});
	cpp() << "\n\treturn VerificationError::VE_OK;\n"
	      << "}\n"
	      << "\n";

	/* Print acyclicity routines + isConsistent() */
	printAcyclicCpp(cnfas.getAcyclic());
	cpp() << "bool " << className << "::isConsistent(const Event &e)\n"
	      << "{\n"
	      << "\treturn isAcyclic(e);\n"
	      << "}\n"
	      << "\n";

	printRecoveryCpp(cnfas.getRecovery());
	cpp() << "bool " << className << "::isRecoveryValid(const Event &e)\n"
	      << "{\n"
	      << "\treturn isRecAcyclic(e);\n"
	      << "}\n"
	      << "\n";

	/* pporf-before getter */
	printPPoRfCpp(cnfas.getPPoRf().first, cnfas.getPPoRf().second);
	cpp() << "std::unique_ptr<VectorClock> " << className << "::getPPoRfBefore(const Event &e)\n"
	      << "{\n"
	      << "\treturn LLVM_MAKE_UNIQUE<" << (cnfas.getPPoRf().second ? "DepView" : "View")
	      				      << ">(calcPPoRfBefore(e));\n"
	      << "}\n"
	      << "\n";

	/* hb-view getter */
	cpp() << "const View &" << className << "::getHbView(const Event &e)\n"
	      << "{\n"
	      << "\treturn getGraph().getEventLabel(e)->view(" << getCalcIdx(cnfas.getHbIndex()) << ");\n"
	      << "}\n"
	      << "\n";

	/* coherence utils */
	auto s = std::string(coherenceFuns);
	replaceAll(s, "#CLASS#", className);
	replaceAll(s, "#HB#", std::to_string(getCalcIdx(cnfas.getCohIndex())));
	cpp() << s << "\n";

	printCppFooter();
}

void Printer::output(const CNFAs &cnfas)
{
	outputHpp(cnfas);
	outputCpp(cnfas);
}

template<typename ITER>
std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end)
{
	std::unordered_map<NFA::State *, unsigned> result;

	auto i = 0u;
	std::for_each(begin, end, [&](auto &s){ result[&*s] = i++; });
	return result;
}

#define GET_ID(nfa_id, state_id) nfa_id << "_" << state_id

void Printer::printAcyclicHpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitAcyclicX() for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tbool visitAcyclic" << ids[&*s] << "(const Event &e)" << ";\n";
	});
	hpp() << "\n";

	/* isAcyclic() for the automaton */
	hpp() << "\tbool isAcyclic(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeCountStatus> visitedAcyclic" << ids[&*s] << ";\n";
	});
	hpp() << "\n";

	/* accepting counter */
	hpp() << "\tuint16_t visitedAccepting = 0;\n";
}

void Printer::printAcyclicCpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* Print a "visitAcyclicXX" for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "bool " << className << "::visitAcyclic" << ids[&*s] << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
		      << "\n";

		if (nfa.isStarting(&*s))
			cpp() << "\t++visitedAccepting;\n";
		cpp() << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp().get()] = "
								"{ visitedAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto &node = visitedAcyclic" << ids[t.dest] << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (node.status == NodeStatus::unseen && !visitAcyclic" << ids[t.dest] << "(p))\n"
			      << "\t\t\treturn false;\n"
			      << "\t\telse if (node.status == NodeStatus::entered && visitedAccepting > node.count)\n"
			      << "\t\t\treturn false;\n"
			      <<"\t}\n";
		});
		if (nfa.isStarting(&*s))
			cpp() << "\t--visitedAccepting;\n";
		cpp() << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp().get()] = "
			"{ visitedAccepting, NodeStatus::left };\n"
		      << "\treturn true;\n"
		      << "}\n"
		      << "\n";
	});

	/* Print a "isAcyclicX" for the automaton */
	cpp() << "bool " << className << "::isAcyclic(const Event &e)\n"
	      << "{\n"
	      << "\tvisitedAccepting = 0;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedAcyclic" << ids[&*s] << ".clear();\n"
		      << "\tvisitedAcyclic" << ids[&*s] << ".resize(g.getMaxStamp().get() + 1);\n";
	});
	cpp() << "\treturn true";
	for (auto sIt = nfa.states_begin(), sE = nfa.states_end(); sIt != sE; ++sIt) {
		if (std::all_of((*sIt)->out_begin(), (*sIt)->out_end(), [&](auto &t){
			auto rOpt = t.label.getRelation();
			return rOpt.has_value() && !rOpt->isInverse() &&
				(Relation::createBuiltin(Relation::Builtin::po_imm).includes(*rOpt) ||
				 Relation::createBuiltin(Relation::Builtin::rf).includes(*rOpt) ||
				 !rOpt->isBuiltin());
				}))
			continue;
		cpp() << "\n\t\t&& visitAcyclic" << ids[&**sIt] << "(e)";
	}
	cpp() << ";\n"
	      << "}\n"
	      << "\n";
}

void Printer::printRecoveryHpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitRecoveryX() for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tbool visitRecovery" << ids[&*s] << "(const Event &e)" << ";\n";
	});
	hpp() << "\n";

	/* isRecAcyclic() for the automaton */
	hpp() << "\tbool isRecAcyclic(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeCountStatus> visitedRecovery" << ids[&*s] << ";\n";
	});
	hpp() << "\n";

	/* accepting counter */
	hpp() << "\tuint16_t visitedRecAccepting = 0;\n";
}

void Printer::printRecoveryCpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* Print a "visitRecoveryXX" for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "bool " << className << "::visitRecovery" << ids[&*s] << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
		      << "\n";

		if (nfa.isStarting(&*s))
			cpp() << "\t++visitedRecAccepting;\n";
		cpp() << "\tvisitedRecovery" << ids[&*s] << "[lab->getStamp().get()] = "
								"{ visitedRecAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto &node = visitedRecovery" << ids[t.dest] << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (node.status == NodeStatus::unseen && !visitRecovery" << ids[t.dest] << "(p))\n"
			      << "\t\t\treturn false;\n"
			      << "\t\telse if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)\n"
			      << "\t\t\treturn false;\n"
			      <<"\t}\n";
		});
		if (nfa.isStarting(&*s))
			cpp() << "\t--visitedRecAccepting;\n";
		cpp() << "\tvisitedRecovery" << ids[&*s] << "[lab->getStamp().get()] = "
			"{ visitedRecAccepting, NodeStatus::left };\n"
		      << "\treturn true;\n"
		      << "}\n"
		      << "\n";
	});

	/* Print a "isRecAcyclicX" for the automaton */
	cpp() << "bool " << className << "::isRecAcyclic(const Event &e)\n"
	      << "{\n"
	      << "\tvisitedRecAccepting = 0;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedRecovery" << ids[&*s] << ".clear();\n"
		      << "\tvisitedRecovery" << ids[&*s] << ".resize(g.getMaxStamp().get() + 1);\n";
	});
	cpp() << "\treturn true";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\n\t\t&& visitRecovery" << ids[&*s] << "(e)";
	});
	cpp() << ";\n"
	      << "}\n"
	      << "\n";
}

void Printer::printCalculatorHpp(const NFA &nfa, unsigned id, VarStatus status)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitCalcXX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tvoid visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, "
		      << ((status == VarStatus::View) ? "View &" : "VSet<Event> &") << "calcRes);\n";
	});
	hpp() << "\n";

	/* calculateX for the automaton */
	hpp() << "\t" << ((status == VarStatus::View) ? "View" : "VSet<Event>") << " calculate" << id << "(const Event &e);\n";
	hpp() << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedCalc" << GET_ID(id, ids[&*s]) << ";\n";
	});
	hpp() << "\n";
}

void Printer::printCalculatorCpp(const NFA &nfa, unsigned id, VarStatus status)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, "
		      << ((status == VarStatus::View) ? "View &" : "VSet<Event> &") << "calcRes)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\n"
		      << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp().get()] = NodeStatus::entered;\n";
		if (nfa.isStarting(&*s)) {
			if (status != VarStatus::View)
				cpp() << "\tcalcRes.insert(e);\n";
			if (status == VarStatus::Reduce) {
				cpp() << "\tfor (const auto &p : lab->calculated(" << getCalcIdx(id) << ")) {\n"
				      << "\t\tcalcRes.erase(p);\n";
				std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
					cpp() << "\t\tvisitedCalc" << GET_ID(id, ids[a]) << "[g.getEventLabel(p)->getStamp().get()] = NodeStatus::left;\n";
				});
				cpp() << "\t}\n";
			} else if (status == VarStatus::View) {
				cpp() << "\tcalcRes.update(lab->view(" << getCalcIdx(id) << "));\n"
				      << "\tcalcRes.updateIdx(lab->getPos());\n";
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedCalc" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitCalc" << GET_ID(id, ids[t.dest]) << "(p, calcRes);\n"
			      <<"\t}\n";
		});
		cpp() << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp().get()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	cpp() << ((status == VarStatus::View) ? "View " : "VSet<Event> ") << className << "::calculate" << id << "(const Event &e)\n"
	      << "{\n"
	      << "\t" << ((status == VarStatus::View) ? "View" : "VSet<Event>") << " calcRes;\n";
	if (status == VarStatus::View)
		cpp() << "\tcalcRes.updateIdx(e.prev());\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".clear();\n"
		      << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);\n";
	});
	cpp() << "\n";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
		cpp() << "\tvisitCalc" << GET_ID(id, ids[a]) << "(e, calcRes);\n";
	});
	cpp() << "\treturn calcRes;\n"
	      << "}\n";
}

void Printer::printPPoRfHpp(const NFA &nfa, bool deps)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitPPoRfXX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tvoid visitPPoRf" << ids[&*s] << "(const Event &e, "
		      			     << (deps ? "DepView" : "View") << " &pporf);\n";
	});
	hpp() << "\n";

	/* calcPPoRfBefore for the automaton */
	hpp() << "\t" << (deps ? "DepView" : "View") << " calcPPoRfBefore(const Event &e);\n";
	hpp() << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedPPoRf" << ids[&*s] << ";\n";
	});
	hpp() << "\n";
}

void Printer::printPPoRfCpp(const NFA &nfa, bool deps)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitPPoRf" << ids[&*s] << "(const Event &e, "
		      					        << (deps ? "DepView" : "View") << " &pporf)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\n"
		      << "\tvisitedPPoRf" << ids[&*s] << "[lab->getStamp().get()] = NodeStatus::entered;\n";
		if (nfa.isStarting(&*s))
			cpp() << "\tpporf.updateIdx(e);\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedPPoRf" << ids[t.dest] << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitPPoRf" << ids[t.dest] << "(p, pporf);\n"
			      <<"\t}\n";
		});
		cpp() << "\tvisitedPPoRf" << ids[&*s] << "[lab->getStamp().get()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	cpp() << (deps ? "DepView " : "View ") << className << "::calcPPoRfBefore(const Event &e)\n"
	      << "{\n"
	      << "\t" << (deps ? "DepView" : "View") << " pporf;\n"
	      << "\tpporf.updateIdx(e);\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedPPoRf" << ids[&*s] << ".clear();\n"
		      << "\tvisitedPPoRf" << ids[&*s] << ".resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);\n";
	});
	cpp() << "\n";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
		cpp() << "\tvisitPPoRf" << ids[a] << "(e, pporf);\n";
	});
	cpp() << "\treturn pporf;\n"
	      << "}\n";
}

void Printer::printInclusionHpp(const NFA &lhs, const NFA &rhs, unsigned id, std::optional<unsigned> rhsViewIdx)
{
	auto idsLHS = assignStateIDs(lhs.states_begin(), lhs.states_end());
	auto idsRHS = assignStateIDs(rhs.states_begin(), rhs.states_end());

	if (rhsViewIdx.has_value()) {
		/* bool visitInclusionXX for LHS only */
		std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
			hpp() << "\tbool visitInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "(const Event &e, const View &v)" << ";\n";
		});
	} else {
		/* visitInclusionXX for each state */
		std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
			hpp() << "\tvoid visitInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "(const Event &e)" << ";\n";
		});
		std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
			hpp() << "\tvoid visitInclusionRHS" << GET_ID(id, idsRHS[&*s]) << "(const Event &e)" << ";\n";
		});
	}
	hpp() << "\n";

	/* checkInclusionX for the automaton */
	hpp() << "\tbool checkInclusion" << id << "(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << ";\n";
	});
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedInclusionRHS" << GET_ID(id, idsRHS[&*s]) << ";\n";
	});
	hpp() << "\n";

	/* caches for inclusion checks */
	hpp() << "\tstatic inline thread_local std::vector<bool> lhsAccept" << id << ";\n"
	      << "\tstatic inline thread_local std::vector<bool> rhsAccept" << id << ";\n"
	      << "\n";
}

void Printer::printInclusionCpp(const NFA &lhs, const NFA &rhs, unsigned id, std::optional<unsigned> rhsViewIdx)
{
	auto idsLHS = assignStateIDs(lhs.states_begin(), lhs.states_end());

	if (rhsViewIdx.has_value()) {
		std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
			cpp() << "bool " << className << "::visitInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "(const Event &e, const View &v)\n"
			      << "{\n"
			      << "\tauto &g = getGraph();\n"
			      << "\tauto *lab = g.getEventLabel(e);\n"
			      << "\n"
			      << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::entered;\n";
			if (lhs.isStarting(&*s))
				cpp() << "\tif (!v.contains(lab->getPos()))\n"
				      << "\t\treturn false;\n";
			std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
				cpp () << "\t";
				printTransLabel(&t.label, "p", "lab");
				cpp() << " {\n"
				      << "\t\tauto status = visitedInclusionLHS" << GET_ID(id, idsLHS[t.dest]) << "[g.getEventLabel(p)->getStamp().get()];\n"
				      << "\t\tif (status == NodeStatus::unseen && !visitInclusionLHS" << GET_ID(id, idsLHS[t.dest]) << "(p, v))\n"
				      << "\t\t\treturn false;\n"
				      << "\t}\n";
			});
			cpp() << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::left;\n"
			      << "\treturn true;\n"
			      << "}\n"
			      << "\n";
		});

		cpp() << "bool " << className << "::checkInclusion" << id << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &v = getGraph().getEventLabel(e)->view(" << getCalcIdx(*rhsViewIdx) << ");\n"
		      << "\n";
		      std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
			      cpp() << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << ".clear();\n"
				    << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << ".resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);\n";
		      });
		cpp() << "\treturn true";
		std::for_each(lhs.accept_begin(), lhs.accept_end(), [&](auto &s){
			cpp() << "\n\t\t&& visitInclusionLHS" << GET_ID(id, idsLHS[s]) << "(e, v)";
		});
		cpp() << ";\n"
		      << "}\n"
		      << "\n";
		return;
	}

	std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\n"
		      << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::entered;\n";
		if (lhs.isStarting(&*s))
			cpp() << "\tlhsAccept" << id << "[lab->getStamp().get()] = true;\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedInclusionLHS" << GET_ID(id, idsLHS[t.dest]) << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitInclusionLHS" << GET_ID(id, idsLHS[t.dest]) << "(p);\n"
			      << "\t}\n";
		});
		cpp() << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	auto idsRHS = assignStateIDs(rhs.states_begin(), rhs.states_end());
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitInclusionRHS" << GET_ID(id, idsRHS[&*s]) << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\n"
		      << "\tvisitedInclusionRHS" << GET_ID(id, idsRHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::entered;\n";
		if (rhs.isStarting(&*s))
			cpp() << "\trhsAccept" << id << "[lab->getStamp().get()] = true;\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedInclusionRHS" << GET_ID(id, idsRHS[t.dest]) << "[g.getEventLabel(p)->getStamp().get()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitInclusionRHS" << GET_ID(id, idsRHS[t.dest]) << "(p);\n"
			      << "\t}\n";
		});
		cpp() << "\tvisitedInclusionRHS" << GET_ID(id, idsRHS[&*s]) << "[lab->getStamp().get()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	cpp() << "bool " << className << "::checkInclusion" << id << "(const Event &e)\n"
	      << "{\n";
	std::for_each(lhs.states_begin(), lhs.states_end(), [&](auto &s){
		cpp() << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << ".clear();\n"
		      << "\tvisitedInclusionLHS" << GET_ID(id, idsLHS[&*s]) << ".resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);\n";
	});
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		cpp() << "\tvisitedInclusionRHS" << GET_ID(id, idsRHS[&*s]) << ".clear();\n"
		      << "\tvisitedInclusionRHS" << GET_ID(id, idsRHS[&*s]) << ".resize(g.getMaxStamp().get() + 1, NodeStatus::unseen);\n";
	});
	cpp() << "\tlhsAccept" << id << ".clear();\n"
	      << "\tlhsAccept" << id << ".resize(g.getMaxStamp().get() + 1, false);\n"
	      << "\trhsAccept" << id << ".clear();\n"
	      << "\trhsAccept" << id << ".resize(g.getMaxStamp().get() + 1, false);\n"
	      << "\n";

	std::for_each(lhs.accept_begin(), lhs.accept_end(), [&](auto &s){
		cpp() << "\tvisitInclusionLHS" << GET_ID(id, idsLHS[s]) << "(e);\n";
	});
	std::for_each(rhs.accept_begin(), rhs.accept_end(), [&](auto &s){
		cpp() << "\tvisitInclusionRHS" << GET_ID(id, idsRHS[s]) << "(e);\n";
	});

	cpp() << "\tfor (auto i = 0u; i < lhsAccept" << id <<".size(); i++) {\n"
	      << "\t\tif (lhsAccept" << id << "[i] && !rhsAccept" << id << "[i])\n"
	      << "\t\t\treturn false;\n"
	      << "\t}\n"
	      << "\treturn true;\n"
	      << "}\n"
	      << "\n";
}
