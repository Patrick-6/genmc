#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

#define mo_rlx memory_order_relaxed

atomic_int x;
atomic_int y;

void *thread_1(void *unused)
{
	atomic_store_explicit(&x, 2, mo_rlx);
	return NULL;
}

void *thread_n(void *unused)
{
	int b = atomic_load_explicit(&x, mo_rlx);
	/* __VERIFIER_assume(b == 0 || b == 1); */
	atomic_store_explicit(&x, 1, mo_rlx);
	atomic_store_explicit(&y, 1, mo_rlx);
	return NULL;
}

void *thread_2(void *unused)
{
	int c = atomic_load_explicit(&y, mo_rlx);
	__VERIFIER_assume(c == 1);
	atomic_store_explicit(&x, 3, mo_rlx);
	return NULL;
}

void *thread_force_co(void *unused)
{
	__VERIFIER_assume(atomic_load_explicit(&x, mo_rlx) == 3 &&
			  atomic_load_explicit(&x, mo_rlx) == 2 &&
			  atomic_load_explicit(&x, mo_rlx) == 1);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3, t4, t5;

	if (pthread_create(&t1, NULL, thread_force_co, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_n, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_2, NULL))
		abort();
	if (pthread_create(&t4, NULL, thread_n, NULL))
		abort();
	if (pthread_create(&t5, NULL, thread_1, NULL))
		abort();

	return 0;
}
