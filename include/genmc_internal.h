#ifndef __GENMC_INTERNAL_H__
#define __GENMC_INTERNAL_H__

#include <stdint.h>

/* Internal declarations for GenMC -- should not be used by user programs */

/*
 * A non-value-returning CAS instruction. In contrast to the C11 one,
 * it does *not* return the old value of the memory location in the 2nd arg.
 * Use __VERIFIER_cmpxchg_noret in user-exposed libraries.
 * Attribute: 0 -> normal, 1-> helped, 2 -> helping */
void __VERIFIER_atomiccas_noret(void *, uint64_t c, uint64_t v, int size,
				int memsucc, int memfail, int attr);

#define __VERIFIER_cmpxchg_noret(p, c, v, ms, mf, a)			\
do {									\
	__typeof__((*c)) _c_ = (*c);					\
	__VERIFIER_atomiccas_noret(p, _c_, v, sizeof(*p), ms, mf, a);	\
	(*c) = _c_;							\
} while (0)


/* A non-value-returning FAI instruction.
 * The last argument of __VERIFIER_atomic_noret stems from llvm::AtomicRMWInst::BinOp,
 * and encodes the type of operation performed. */
/* TODO: Unify signature w/ __VERIFIER_atomiccas_noret */
void __VERIFIER_atomicrmw_noret(int *, int, int memory_order, int);
#define __VERIFIER_fetch_add_noret(v, i, m) __VERIFIER_atomicrmw_noret(v, i, m, 1)
#define __VERIFIER_fetch_sub_noret(v, i, m) __VERIFIER_atomicrmw_noret(v, i, m, 2)

void __VERIFIER_rcu_read_lock();
void __VERIFIER_rcu_read_unlock();
void __VERIFIER_synchronize_rcu();

#endif /* __GENMC_INTERNAL_H__ */
