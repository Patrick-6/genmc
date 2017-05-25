#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "../../stdatomic.h"

atomic_int x;
atomic_int flag;

void *thread_writer(void *arg)
{
	atomic_store_explicit(&x, 42, memory_order_release);
	atomic_store_explicit(&flag, 1, memory_order_release);
	return NULL;
}

void *thread_reader(void *arg)
{
	int local = 0;
	int count = 0;
	
	local = atomic_load_explicit(&flag, memory_order_acquire);
	while (local != 1) {
		count++;
		if (count > 40)
			return NULL;
		local = atomic_load_explicit(&flag, memory_order_acquire);
	}
	// printf("got it!\n");
	assert(atomic_load_explicit(&x, memory_order_acquire) == 42);
	return NULL;
}

int main()
{
	pthread_t tr, tw;
	
	if (pthread_create(&tr, NULL, thread_writer, NULL))
		abort();
	if (pthread_create(&tw, NULL, thread_reader, NULL))
		abort();	

	return 0;
}
