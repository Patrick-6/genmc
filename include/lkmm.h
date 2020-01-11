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

#ifndef __LKMM_H__
#define __LKMM_H__

#include <stdatomic.h>

/* Atomic data types */
#define atomic_t       atomic_int
#define atomic_long_t  atomic_long
#define ATOMIC_INIT(i) ATOMIC_VAR_INIT(i)

/* ONCE */
#define READ_ONCE(x)     atomic_load_explicit(&x, memory_order_relaxed)
#define WRITE_ONCE(x, v) atomic_store_explicit(&x, v, memory_order_relaxed)

/* Fences */
#define barrier() __asm__ __volatile__ (""   : : : "memory")
#define smp_mb()  __asm__ __volatile__ ("mb" : : : "memory")
#define smp_rmb() __asm__ __volatile__ ("rmb" : : : "memory")
#define smp_wmb() __asm__ __volatile__ ("wmb" : : : "memory")
#define smp_mb__before_atomic()     __asm__ __volatile__ ("ba" : : : "memory")
#define smp_mb__after_atomic()      __asm__ __volatile__ ("aa" : : : "memory")
#define smp_mb__after_spinlock()    __asm__ __volatile__ ("as" : : : "memory")
#define smp_mb__after_unlock_lock() __asm__ __volatile__ ("au" : : : "memory")

/* Acquire/Release and friends */
#define smp_load_acquire(p)      atomic_load_explicit(p, memory_order_acquire)
#define smp_store_release(p, v)  atomic_store_explicit(p, v, memory_order_release)
#define rcu_dereference(p)       READ_ONCE(*p)
#define rcu_assign_pointer(p, v) smp_store_release(p, v)
#define smp_store_mb(x, v)					\
do {								\
	atomic_store_explicit(&x, v, memory_order_relaxed);	\
	smp_mb();						\
} while (0)

/* Exchange */
#define xchg(p, v)         atomic_exchange(p, v)
#define xchg_relaxed(p, v) atomic_exchange_explicit(p, v, memory_order_relaxed)
#define xchg_release(p, v) atomic_exchange_explicit(p, v, memory_order_release)
#define xchg_acquire(p, v) atomic_exchange_explicit(p, v, memory_order_acquire)
#define __cmpxchg(p, o, n, s, f)				\
({								\
	__typeof__(&(o)) _o_ = (o);				\
	(atomic_compare_exchange_strong(p, _o_, n, s, f));	\
})
#define cmpxchg(p, o, n)					\
	__cmpxchg(p, o, n, memory_order_seq_cst, memory_order_seq_cst)
#define cmpxchg_relaxed(p, o, n)				\
	__cmpxchg(p, o, n, memory_order_relaxed, memory_order_relaxed)
#define cmpxchg_acquire(x, o, n)				\
	__cmpxchg(p, o, n, memory_order_acquire, memory_order_acquire)
#define cmpxchg_release(x, o, n)				\
	__cmpxchg(p, o, n, memory_order_release, memory_order_release)

/* Spinlocks */
#define spin_lock(l)      pthread_mutex_lock(&l)
#define spin_unlock(l)    pthread_mutex_unlock(&l)
#define spin_trylock(l)   pthread_mutex_trylock(&l)
#define spin_is_locked(l) (atomic_load_explicit(&l, memory_order_relaxed) == 1)

/* RCU */
void rcu_read_lock();
void rcu_read_unlock();
void synchronize_rcu();
void synchronize_rcu_expedited();

/* Atomic */
#define atomic_read(v)   READ_ONCE(*v)
#define atomic_set(v, i) WRITE_ONCE(*v, i)
#define atomic_read_acquire(v)   smp_load_acquire(v)
#define atomic_set_release(v, i) smp_store_release(v, i)

#define __atomic_add(i, v, m) atomic_fetch_add_explicit(v, i, m)
#define __atomic_sub(i, v, m) atomic_fetch_sub_explicit(v, i, m)
#define atomic_add(i, v) __atomic_add(i, v, memory_order_seq_cst)
#define atomic_sub(i, v) __atomic_add(i, v, memory_order_seq_cst)
#define atomic_inc(v) atomic_add(1, v)
#define atomic_dec(v) atomic_sub(1, v)

#define atomic_add_return(i, v) (atomic_add(i, v) + i)
#define atomic_add_return_relaxed(i, v) (__atomic_add(i, v, memory_order_relaxed + i)
#define atomic_add_return_acquire(i, v) (__atomic_add(i, v, memory_order_acquire + i)
#define atomic_add_return_release(i, v) (__atomic_add(i, v, memory_order_release + i)

#define atomic_fetch_add(i, v)         atomic_add(i, v)
#define atomic_fetch_add_relaxed(i, v) __atomic_add(i, v, memory_order_relaxed)
#define atomic_fetch_add_acquire(i, v) __atomic_add(i, v, memory_order_acquire)
#define atomic_fetch_add_release(i, v) __atomic_add(i, v, memory_order_release)

#define atomic_inc_return(v)         atomic_add_return(1, v)
#define atomic_inc_return_relaxed(v) atomic_add_return_relaxed(1, v)
#define atomic_inc_return_acquire(v) atomic_add_return_acquire(1, v)
#define atomic_inc_return_release(v) atomic_add_return_release(1, v)
#define atomic_fetch_inc(v)         atomic_fetch_add(1, v)
#define atomic_fetch_inc_relaxed(v) atomic_fetch_add_relaxed(1, v)
#define atomic_fetch_inc_acquire(v) atomic_fetch_add_acquire(1, v)
#define atomic_fetch_inc_release(v) atomic_fetch_add_release(1, v)

#define atomic_sub_return(i, v)         (atomic_sub(i, v) - i)
#define atomic_sub_return_relaxed(i, v) (__atomic_sub(i, v, memory_order_relaxed) - i)
#define atomic_sub_return_acquire(i, v) (__atomic_sub(i, v, memory_order_acquire) - i)
#define atomic_sub_return_release(i, v) (__atomic_sub(i, v, memory_order_release) - i)

#define atomic_fetch_sub(i, v)         atomic_sub(i, v)
#define atomic_fetch_sub_relaxed(i, v) __atomic_sub(i, v, memory_order_relaxed)
#define atomic_fetch_sub_acquire(i, v) __atomic_sub(i, v, memory_order_acquire)
#define atomic_fetch_sub_release(i, v) __atomic_sub(i, v, memory_order_release)

#define atomic_dec_return(v)         atomic_sub_return(1, v)
#define atomic_dec_return_relaxed(v) atomic_sub_return_relaxed(1, v)
#define atomic_dec_return_acquire(v) atomic_sub_return_acquire(1, v)
#define atomic_dec_return_release(v) atomic_sub_return_release(1, v)
#define atomic_fetch_dec(v)         atomic_fetch_sub(1, v)
#define atomic_fetch_dec_relaxed(v) atomic_fetch_sub_relaxed(1, v)
#define atomic_fetch_dec_acquire(v) atomic_fetch_sub_acquire(1, v)
#define atomic_fetch_dec_release(v) atomic_fetch_sub_release(1, v)

#define atomic_xchg(x, v)         xchg(x, v)
#define atomic_xchg_relaxed(x, v) xchg_relaxed(x, v)
#define atomic_xchg_release(x, v) xchg_release(x, v)
#define atomic_xchg_acquire(x, v) xchg_acquire(x, v)
#define atomic_cmpxchg(x, o, n)       cmpxchg(x, o, n)
#define atomic_cmpxchg_relaxed(x,v,W) cmpxchg_relaxed(x, o, n)
#define atomic_cmpxchg_acquire(x,v,W) cmpxchg_acquire(x, o, n)
#define atomic_cmpxchg_release(x,v,W) cmpxchg_release(x, o, n)

#define atomic_sub_and_test(i, v) atomic_sub(i, v) == 0
#define atomic_dec_and_test(v)    atomic_dec(v) == 0
#define atomic_inc_and_test(v)    atomic_inc(v) == 0
#define atomic_add_negative(i, v) atomic_add(i, v) < 0

#endif /* __LKMM_H__ */
