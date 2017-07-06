#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

atomic_int x = 0;
atomic_int y = 0;
atomic_int z1 = 0;
atomic_int z2 = 0;

void *thread_one(void *arg)
{
	atomic_store_explicit(&y, 1, memory_order_seq_cst);
	if (!atomic_load_explicit(&x, memory_order_seq_cst)) {
		atomic_store_explicit(&z1, 1, memory_order_relaxed);
	}
	return NULL;
}

void *thread_two(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_seq_cst);
	if (!atomic_load_explicit(&y, memory_order_seq_cst)) {
		atomic_store_explicit(&z2, 1, memory_order_relaxed);
	}
	return NULL;
}

void *thread_three(void *arg)
{
	if (!atomic_load_explicit(&z1, memory_order_relaxed) && !atomic_load_explicit(&z2, memory_order_relaxed)) {
		for (int i = 0; i < N; i++) {
			atomic_store_explicit(&x, i, memory_order_relaxed);
		}
	}
	return NULL;
}

void *thread_four(void *arg)
{
	if (!atomic_load_explicit(&z1, memory_order_relaxed) && !atomic_load_explicit(&z2, memory_order_relaxed)) {
		for (int i = 0; i < N; i++) {
			atomic_load_explicit(&x, memory_order_relaxed);
		}
	}
	return NULL;
}


int main()
{
	pthread_t t1, t2, t3, t4;

	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();
	if (pthread_create(&t4, NULL, thread_four, NULL))
		abort();

	return 0;
}
