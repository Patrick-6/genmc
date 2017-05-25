#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

atomic_int x;
atomic_int y;

void *thread_one(void *arg)
{
	atomic_load_explicit(&x, memory_order_acquire);
	atomic_store_explicit(&x, 1, memory_order_release);
	return NULL;
}

void *thread_two(void *arg)
{
	atomic_store_explicit(&x, 2, memory_order_release);
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
