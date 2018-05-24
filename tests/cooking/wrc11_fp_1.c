#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "../stdatomic.h"

atomic_int x;

void *thread_1(void *unused)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	return NULL;
}

void *thread_2(void *unused)
{
	atomic_int a, b;
	atomic_store_explicit(&x, 2, memory_order_release);
	a = atomic_load_explicit(&x, memory_order_acquire);
	b = atomic_load_explicit(&x, memory_order_acquire);
	printf("a = %d, r2 = %d\n", a, b); // take me out
	assert(! (a == 1 && b == 2));      // ideally, this should be in main
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();

	return 0;
}
