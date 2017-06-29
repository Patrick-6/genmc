#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;

static void thread_1(void *unused)
{
	atomic_load_explicit(&x, memory_order_relaxed);
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
}

static void thread_2(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	atomic_init(&x, 0);
	thrd_create(&t1, (thrd_start_t)&thread_1, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_2, NULL);

	return 0;
}
