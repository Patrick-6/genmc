#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;

static void thread_1(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
	atomic_store_explicit(&y, 1, memory_order_release);
}

static void thread_2(void *unused)
{
	int r = 0;
	atomic_compare_exchange_strong_explicit(&x, &r, 1, memory_order_acquire,
						memory_order_acquire);
}

static void *thread_3(void *unused)
{
	if (atomic_load_explicit(&y, memory_order_relaxed))
		atomic_store_explicit(&x, 4, memory_order_release);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	thrd_create(&t1, (thrd_start_t)&thread_1, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_2, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_3, NULL);

	return 0;
}
