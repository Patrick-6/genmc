#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
atomic_int z;

int idx[N];

static void thread_one(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_load_explicit(&y, memory_order_acquire);
	atomic_load_explicit(&x, memory_order_acquire);
}

static void thread_two(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_release);
	atomic_load_explicit(&z, memory_order_acquire);
	atomic_load_explicit(&x, memory_order_acquire);
}

static void thread_n(void *arg)
{
	int i = *((int *) arg);

	atomic_store_explicit(&y, i, memory_order_release);
	atomic_store_explicit(&z, i, memory_order_release);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t[N];

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);
	thrd_create(&t1, (thrd_start_t)&thread_one, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_two, NULL);
	for (int i = 0; i < N - 1; i++) {
		idx[i] = i;
		thrd_create(&t[i], (thrd_start_t)&thread_n, &idx[i]);
	}

	return 0;
}
