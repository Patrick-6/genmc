#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;

static void thread_0(void *unused)
{
	atomic_store_explicit(&x, 2, memory_order_release);
}

static void thread_n(void *unused)
{
	int r = 2;
	atomic_compare_exchange_strong_explicit(&x, &r, 3, memory_order_relaxed,
						memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t0, t[N];

	atomic_init(&x, 0);
	thrd_create(&t0, (thrd_start_t)&thread_0, NULL);
	for (int i = 0; i < N; i++)
		thrd_create(&t[i], (thrd_start_t)&thread_n, NULL);

	return 0;
}
