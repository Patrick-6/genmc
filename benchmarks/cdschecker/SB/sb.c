#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;

static void thread_one(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_load_explicit(&y, memory_order_acquire);
}

static void thread_two(void *arg)
{
	atomic_store_explicit(&y, 1, memory_order_release);
	atomic_load_explicit(&x, memory_order_acquire);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	thrd_create(&t1, (thrd_start_t)&thread_one, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_two, NULL);

	return 0;
}
