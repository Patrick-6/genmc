#include "Relation.hpp"

using PB = Predicate::Builtin;
const std::unordered_map<Relation::Builtin, RelationInfo> Relation::builtins = {
        /* po */
        {po_imm,	{"po-imm",      RelType::OneOne,     {},   {},   true}},
        {po_loc_imm,	{"po-loc-imm",  RelType::OneOne,     {},   {},   true}},
	/* deps */
        {ctrl_imm,	{"ctrl-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true}},
        {addr_imm,	{"addr-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true}},
        {data_imm,	{"data-imm",    RelType::UnsuppMany, {Predicate::createBuiltin(PB::R)}, {},   true}},
	/* same thread */
	{same_thread,	{"same-thread", RelType::Conj,	     {},   {},   false}},
	/* same location */
        {alloc,		{"alloc",       RelType::ManyOne,    {},   {},   false}},
        {frees,		{"frees",       RelType::OneOne,     {},   {},   false}},
        {loc_overlap,	{"loc-overlap", RelType::Final,      {},   {},   false}},
	/* rf, co, fr, detour */
        {rf,		{"rf",          RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false}},
        {rfe,		{"rfe",         RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false}},
        {rfi,		{"rfi",         RelType::ManyOne,    {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false}},
        {tc,		{"tc",          RelType::OneOne,     {Predicate::createBuiltin(PB::TC)}, {Predicate::createBuiltin(PB::TB)}, false}},
        {tj,		{"tj",          RelType::OneOne,     {Predicate::createBuiltin(PB::TE)}, {Predicate::createBuiltin(PB::TJ)}, false}},
        {mo_imm,	{"mo-imm",      RelType::OneOne,     {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false}},
        {moe,		{"moe",         RelType::UnsuppMany, {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false}},
        {moi,		{"moi",         RelType::UnsuppMany, {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::W)}, false}},
        {fr_imm,	{"fr-imm",      RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false}},
        {fre,		{"fre",         RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false}},
        {fri,		{"fri",         RelType::ManyOne,    {Predicate::createBuiltin(PB::R)},  {Predicate::createBuiltin(PB::W)}, false}},
        {detour,	{"detour",      RelType::OneOne,     {Predicate::createBuiltin(PB::W)},  {Predicate::createBuiltin(PB::R)}, false}},
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
