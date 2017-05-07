#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

_Atomic int x;

void *thread_one(void *arg)
{
	int r = 0;

	atomic_compare_exchange_strong(&x, &r, 42);
	return NULL;
}

void *thread_two(void *arg)
{
	int r = 0;

	atomic_compare_exchange_strong(&x, &r, 42);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();

	return 0;
}
