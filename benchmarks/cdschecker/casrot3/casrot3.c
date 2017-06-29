#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>

atomic_int x;
int idx[N];

static void thread_n(void *arg)
{
	int new = *((int *) arg);
	int exp = new - 1;

	atomic_compare_exchange_strong_explicit(&x, &exp, new, memory_order_relaxed,
						memory_order_relaxed);
}

int user_main(int argc, char **argv)
{
	thrd_t t[N];

	atomic_init(&x, 0);
	for (int i = 1; i < N + 1; i++) {
		idx[i - 1] = i;
		thrd_create(&t[i - 1], (thrd_start_t)&thread_n, &idx[i - 1]);
	}

	return 0;
}
