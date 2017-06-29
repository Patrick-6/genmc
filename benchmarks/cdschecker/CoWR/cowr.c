#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;

static void thread_one(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_load_explicit(&x, memory_order_acquire);
}

static void thread_two(void *arg)
{
	atomic_store_explicit(&x, 2, memory_order_release);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	thrd_create(&t1, (thrd_start_t)&thread_one, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_two, NULL);

	return 0;
}
