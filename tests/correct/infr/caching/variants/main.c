#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>
#include <assert.h>

/*
 * This testcase demonstrates two different issues that affect caching
 * and the form of execution graphs.
 *
 * 1) Until v0.11.0, we were dispensing increasing unique TIDs, and were
 *    also ensuring that the same TID will be used by the same thread after
 *    restriction, so as to be able to be reuse EE's state. This had several
 *    disadvantages: (a) annoying empty threads in the execution graph (e.g.,
 *    when conditionally spawning threads), (b) conditionally initializing
 *    EE::ECstacks from the driver, and (c) having to query ECStack emptiness
 *    from the driver when deciding when a thread is schedulable. In addition,
 *    even though the code ensured (?) that a given thread would always
 *    be spawned from the same create position (so we could cache based on
 *    TIDs), the same thread would often be created at a different index
 *    (e.g., after some intervening empty threads), in which case we would not use
 *    the cache (although we could).
 * 2) A solution for 1) would be to not have empty threads in the graph.
 *    Disposing those, however, would mean that we have to cache based on
 *    <function_id, TID>, and not just TID.
 *
 * This test has the following form:
 *
 *   x := 1; ...; x := N || { either a := y or assert(0) }
 *
 * T2 is expensive to run (due to a local loop), and its spawn is conditional
 * on a read of x, which would force different TIDs to be used with the old infra).
 * In addition, for a given value of x an assert(0) thread is introduced instead, which
 * shows that we cannot cache solely based on TIDs with the new infra.
 *
 */

#ifndef N
# define N 5
#endif

atomic_int x;
atomic_int y;

void *thread_1(void *unused)
{
	for (int i = 0U; i < N; i++)
		x = i;
	return NULL;
}

void *thread_n(void *unused)
{
	/* make this thread expensive to run */
	for (int i = 0U; i < 1000000; i++)
		;

	int r = y;
	return NULL;
}

void *thread_error(void *unused)
{
#ifdef BUG
	assert(0);
#endif
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	pthread_create(&t1, NULL, thread_1, NULL);

	int a = x;
	for (int i = 0U; i < N; i++) {
		if (a == i) {
			/* Forces different TIDs to be spawned (if begins are kept),
			 * but instructions could be learned due to y writing the same val */
			for (int j = 0U; j < i; j++)
				y = 1;
			pthread_create(&t2, NULL, (i == 1) ? thread_error : thread_n, NULL);
			break;
		}
	}

	return 0;
}
