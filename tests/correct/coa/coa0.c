#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

int _Atomic x;
int _Atomic y;
int _Atomic z;

int idx[N];

void *thread_one(void *unused)
{
	int r;

	atomic_store_explicit(&x, 1, memory_order_release);
	r = atomic_load_explicit(&y, memory_order_acquire);
	r = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

void *thread_two(void *unused)
{
	int r;

	atomic_store_explicit(&x, 2, memory_order_release);
	r = atomic_load_explicit(&z, memory_order_acquire);
	r = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

void *thread_n(void *arg)
{
	int i = *((int *) arg);

	atomic_store_explicit(&y, i, memory_order_release);
	atomic_store_explicit(&z, i, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t[N];

	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	for (int i = 0; i < N; i++) {
		idx[i] = i;
		if (pthread_create(&t[i], NULL, thread_n, &idx[i]))
			abort();
	}

	return 0;
}
