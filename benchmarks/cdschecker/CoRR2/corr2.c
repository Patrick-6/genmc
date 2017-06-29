#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;

static void thread_one(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
}

static void thread_two(void *arg)
{
	atomic_store_explicit(&x, 2, memory_order_release);
}

static void thread_three(void *arg)
{
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_load_explicit(&x, memory_order_acquire);
}

static void thread_four(void *arg)
{

	atomic_load_explicit(&x, memory_order_acquire);
	atomic_load_explicit(&x, memory_order_acquire);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	thrd_create(&t1, (thrd_start_t)&thread_one, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_two, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_three, NULL);
	thrd_create(&t4, (thrd_start_t)&thread_four, NULL);

	return 0;
}
