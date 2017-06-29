#include <assert.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
atomic_int y;

#define NUM 5

static void thread_1(void* arg)
{
	for (int i = 0; i < NUM; i++) {
		int prev_x = atomic_load_explicit(&x, memory_order_acquire);
		int prev_y = atomic_load_explicit(&y, memory_order_acquire);
		atomic_store_explicit(&x, prev_x + prev_y, memory_order_release);
	}
}

static void thread_2(void* arg)
{
	for (int i = 0; i < NUM; i++) {
		int prev_x = atomic_load_explicit(&x, memory_order_acquire);
		int prev_y = atomic_load_explicit(&y, memory_order_acquire);
		atomic_store_explicit(&y, prev_x + prev_y, memory_order_release);
	}
}

static void thread_3(void *arg)
{
	if (atomic_load_explicit(&x, memory_order_acquire) > 144 ||
	    atomic_load_explicit(&y, memory_order_acquire) > 144)
		assert(0);
}


int user_main(int argc, char **argv)
{
	thrd_t t1, t2, t3;

	atomic_init(&x, 1);
	atomic_init(&y, 1);
	thrd_create(&t1, (thrd_start_t)&thread_1, NULL);
	thrd_create(&t2, (thrd_start_t)&thread_2, NULL);
	thrd_create(&t3, (thrd_start_t)&thread_3, NULL);

	return 0;
}
