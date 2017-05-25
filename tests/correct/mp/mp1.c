#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

int _Atomic x;
int _Atomic y;

void *thread_one(void *arg)
{
	int r;

	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_store_explicit(&y, 1, memory_order_release);
	return NULL;
}

void *thread_two(void *arg)
{
	int r_y, r_x;

	r_y = atomic_load_explicit(&y, memory_order_acquire);
	r_x = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();

	return 0;
}
