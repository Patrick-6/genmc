#include "Relation.hpp"

using PB = Predicate::Builtin;
const std::unordered_map<Relation::Builtin, RelationInfo> Relation::builtins = {
        /* po */
        {po_imm,	{"po-imm",      RelType::OneOne,     {},   {},   true,  "po_imm_succs",     "po_imm_preds"}},
        {po_loc_imm,	{"po-loc-imm",  RelType::OneOne,     {},   {},   true,  "poloc_imm_succs",  "poloc_imm_preds"}},
	/* deps */
        {ctrl_imm,	{"ctrl-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true,  "?",               "ctrl_preds"}},
        {addr_imm,	{"addr-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true,  "?",               "addr_preds"}},
        {data_imm,	{"data-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true,  "?",               "data_preds"}},
	/* same thread */
	{same_thread,	{"same-thread", RelType::Conj,	     {},   {},   false, "same_thread",      "same_thread"}},
	/* same location */
        {alloc,		{"alloc",       RelType::ManyOne,    {},   {},   false, "alloc_succs",      "alloc"}},
        {frees,		{"frees",       RelType::OneOne,     {},   {},   false, "frees",            "alloc"}},
        {loc_overlap,	{"loc-overlap", RelType::Final,      {},   {},   false, "?",                "loc_preds"}},
	/* rf, co, fr, detour */
        {rf,		{"rf",          RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false, "rf_succs",      "rf_preds"}},
        {rfe,		{"rfe",         RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false, "rfe_succs",     "rfe_preds"}},
        {rfi,		{"rfi",         RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false, "rfi_succs",     "rfi_preds"}},
        {tc,		{"tc",          RelType::OneOne,     {Predicate::createBuiltin(PB::TC)}, {Predicate::createBuiltin(PB::TB)}, false, "tc_succs",     "tc_preds"}},
        {tj,		{"tj",          RelType::OneOne,     {Predicate::createBuiltin(PB::TE)}, {Predicate::createBuiltin(PB::TJ)}, false, "tj_succs",     "tj_preds"}},
        {mo_imm,	{"mo-imm",      RelType::OneOne,     {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false, "co_imm_succs",  "co_imm_preds"}},
        {moe,		{"moe",         RelType::UnsuppMany, {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false, "co_imm_succs",  "co_imm_preds"}},
        {moi,		{"moi",         RelType::UnsuppMany, {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false, "co_imm_succs",  "co_imm_preds"}},
        {fr_imm,	{"fr-imm",      RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false, "fr_imm_succs",  "fr_imm_preds"}},
        {fre,		{"fre",         RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false, "fr_imm_succs",  "fr_imm_preds"}},
        {fri,		{"fri",         RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false, "fr_imm_succs",  "fr_imm_preds"}},
        {detour,	{"detour",      RelType::OneOne,     {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false, "detour_succs",  "detour_preds"}},
};

const PredicateSet &Relation::getDomain() const
{
	assert(isBuiltin());
	return builtins.find(toBuiltin())->second.dom;
}

const PredicateSet &Relation::getCodomain() const
{
	assert(isBuiltin());
	return builtins.find(toBuiltin())->second.codom;
}

std::string Relation::getName() const
{
	return (isBuiltin() ? Relation::builtins.find(toBuiltin())->second.name :
		("$" + std::to_string(getID())));
}
