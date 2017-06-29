#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
int idx[N];

static void thread_n(void *arg)
{
	int v = *((int *) arg);

	atomic_load_explicit(&x, memory_order_relaxed);
	atomic_store_explicit(&x, v, memory_order_release);
}

int user_main(int argc, char **argv)
{
	thrd_t t[N];

	atomic_init(&x, 0);
	for (int i = 0; i < N; i++) {
		idx[i] = i;
		thrd_create(&t[i], (thrd_start_t)&thread_n, &idx[i]);
	}

	return 0;
}
