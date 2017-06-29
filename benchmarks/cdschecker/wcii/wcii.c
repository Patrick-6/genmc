#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;

static void thread_1(void *unused)
{
	atomic_store_explicit(&x, 5, memory_order_release);
}

static void thread_2(void *unused)
{
	int expected = 1;
	atomic_compare_exchange_strong_explicit(&x, &expected, 2,
						memory_order_acquire,
						memory_order_acquire);
}

static void thread_3(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
}

static void thread_4(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	thrd_create(&t1, (thrd_start_t)&thread_1, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_2, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_3, NULL);
	thrd_create(&t4, (thrd_start_t)&thread_4, NULL);

	return 0;
}
