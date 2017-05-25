#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../stdatomic.h"

_Atomic int x;

void *thread_n(void *unused)
{
	int r = 42;

	atomic_fetch_add(&x, r);
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
