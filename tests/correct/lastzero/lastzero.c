#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

atomic_int array[N+1];
int idx[N+1];

void *thread_reader(void *unused)
{
	for (int i = N; atomic_load_explicit(&array[i], memory_order_acquire) != 0; i--)
		;
	return NULL;
}

void *thread_writer(void *arg)
{
	int j = *((int *) arg);
	
	atomic_store_explicit(&array[j],
			      atomic_load_explicit(&array[j - 1], memory_order_acquire) + 1,
			      memory_order_release);
	return NULL;
}

int main()
{
	pthread_t t[N+1];

	for (int i = 0; i <= N; i++) {
		idx[i] = i;
		if (i == 0) {
			if (pthread_create(&t[i], NULL, thread_reader, &idx[i]))
				abort();
		} else {
			if (pthread_create(&t[i], NULL, thread_writer, &idx[i]))
				abort();
		}
	}	

	return 0;
}
