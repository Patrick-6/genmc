#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

int _Atomic x;
int _Atomic y;
int _Atomic z;
int _Atomic w;

void *thread_one(void *arg)
{
	int r;

	r = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

void *thread_two(void *arg)
{
	int r;

	r = atomic_load_explicit(&x, memory_order_acquire);
	atomic_store_explicit(&x, 1, memory_order_release);
	return NULL;
}

void *thread_three(void *arg)
{
	atomic_store_explicit(&x, 2, memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();

	return 0;
}
