#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
atomic_int z;
atomic_int w;

static void thread_one(void *arg)
{
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_load_explicit(&y, memory_order_acquire);
	atomic_load_explicit(&z, memory_order_acquire);
	atomic_load_explicit(&w, memory_order_acquire);
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_load_explicit(&y, memory_order_acquire);
	atomic_load_explicit(&z, memory_order_acquire);
}

static void thread_two(void *arg)
{
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_load_explicit(&y, memory_order_acquire);
	atomic_load_explicit(&z, memory_order_acquire);
}

static void thread_three(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_store_explicit(&y, 1, memory_order_release);
	atomic_store_explicit(&x, 2, memory_order_release);
	atomic_store_explicit(&y, 2, memory_order_release);
	atomic_store_explicit(&x, 3, memory_order_release);
	atomic_store_explicit(&y, 3, memory_order_release);
}

static void thread_four(void *arg)
{
	atomic_store_explicit(&z, 1, memory_order_release);
	atomic_store_explicit(&w, 1, memory_order_release);
	atomic_store_explicit(&z, 2, memory_order_release);
	atomic_store_explicit(&w, 2, memory_order_release);
	atomic_store_explicit(&z, 3, memory_order_release);
	atomic_store_explicit(&w, 3, memory_order_release);
}


int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);
	atomic_init(&w, 0);

	thrd_create(&t1, (thrd_start_t)&thread_one, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_two, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_three, NULL);
	thrd_create(&t4, (thrd_start_t)&thread_four, NULL);

	thrd_join(t1);
	thrd_join(t2);
	thrd_join(t3);
	thrd_join(t4);

	return 0;
}
