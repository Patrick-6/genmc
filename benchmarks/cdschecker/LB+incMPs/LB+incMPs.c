#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;
atomic_int w;
atomic_int z;

static void thread_1(void *arg)
{
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_fetch_add_explicit(&z, 1, memory_order_acquire);
}

static void thread_2(void *arg)
{
	atomic_fetch_add_explicit(&z, 1, memory_order_acquire);
	atomic_store_explicit(&y, 1, memory_order_release);
}

static void thread_3(void *arg)
{
	atomic_load_explicit(&y, memory_order_acquire);
	atomic_fetch_add_explicit(&w, 1, memory_order_acquire);
}

static void thread_4(void *arg)
{
	atomic_fetch_add_explicit(&w, 1, memory_order_acquire);
	atomic_store_explicit(&x, 1, memory_order_release);
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3, t4;

	atomic_init(&x, 0);
	atomic_init(&y, 0);
	atomic_init(&z, 0);
	atomic_init(&w, 0);
	thrd_create(&t1, (thrd_start_t)&thread_1, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_2, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_3, NULL);
	thrd_create(&t4, (thrd_start_t)&thread_4, NULL);

	return 0;
}
