#ifndef __GENMC_H__
#define __GENMC_H__

#include <genmc_internal.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * If the argument is not true, blocks the execution
 */
void __VERIFIER_assume(int);

/*
 * Models a limited amount of non-determinism by returning
 * a pseudo-random sequence of integers. This sequence
 * is always the same per execution for each thread
 */
int __VERIFIER_nondet_int(void);

/*
 * Marker functions that can be used to mark the
 * beginning and end of spinloops that are not automatically
 * transformed to assume() statements by GenMC.
 *
 * 1) __VERIFIER_loop_begin() marks the beginning of a loop
 * 2) __VERIFIER_spin_start() marks the beginning of an iteration
 * 3) __VERIFIER_spin_end(cond) ends a given loop iteration if
 *   COND does not hold
 *
 * Example usage:
 *
 *     for (__VERIFIER_loop_begin();
 *          __VERIFIER_spin_start(), ..., __VERIFIER_spin_end(...);
 *          ...) {
 *     // no side-effects, preferrably empty
 * }
 *
 * NOTE: These should _not_ be used on top of the
 * automatic spin-assume transformation.
 */
void __VERIFIER_loop_begin(void);
void __VERIFIER_spin_start(void);
void __VERIFIER_spin_end(int);

/*
 * GenMC-specific CAS instructions. They can be used for CASes that
 * have a "helping" role (e.g., tail advancement in Michael-Scott queue),
 * and can aid in state-space reduction. They do not return any value
 * See the manual for a lengthier explanation.
 */
#define __VERIFIER_CAS_helped_explicit(p, c, v, ms, mf)		\
	__VERIFIER_cmpxchg_noret(p, c, v, ms, mf, 1)
#define __VERIFIER_CAS_helped(p, c, v)				\
	__VERIFIER_CAS_helped(p, c, v, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define __VERIFIER_CAS_helping_explicit(p, c, v, ms, mf)	\
	__VERIFIER_cmpxchg_noret(p, c, v, ms, mf, 2)
#define __VERIFIER_CAS_helping(p, c, v)				\
	__VERIFIER_CAS_helping(p, c, v, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

/*
 * The signature of a recovery routine to be specified
 * by the user. This routine will run after each execution,
 * if the checker is run with the respective flags enabled
 */
void __VERIFIER_recovery_routine(void);

/*
 * All the file opeartions before this barrier will
 * have persisted to memory when the recovery routine runs.
 * Should be used only once.
 */
void __VERIFIER_pbarrier(void);

#ifdef __cplusplus
}
#endif

#endif /* __GENMC_H__ */
