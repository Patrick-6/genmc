#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

atomic_int x;
atomic_int idx[N+1];

void *thread_writer(void *unused)
{
	atomic_store_explicit(&x, 42, memory_order_release);
	return NULL;
}

void *thread_reader(void *arg)
{
	int r = *((int *) arg);
	atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

int main()
{
	pthread_t t[N+1];

	for (int i = 0; i <= N; i++) {
		idx[i] = i;
		if (i == 0) {
			if (pthread_create(&t[i], NULL, thread_writer, NULL))
				abort();
		} else {
			if (pthread_create(&t[i], NULL, thread_reader, &idx[i-1]))
				abort();
		}
	}

	for (int i = 0; i <= N; i++) {
		pthread_join(t[i], NULL);
	}	

	return 0;
}
