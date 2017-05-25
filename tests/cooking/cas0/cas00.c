#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

_Atomic int x;

void *thread_n(void *unused)
{
	int r = 0;

	atomic_compare_exchange_strong(&x, &r, 42);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; ++i)
		if (pthread_create(&t[i], NULL, thread_n, NULL))
			abort();

	return 0;
}
