#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "stdatomic.h"

int x;
int idx[N];

void *thread_writer(void *unused)
{
	x = 42;
	return NULL;
}

void *thread_reader(void *arg)
{
	int r = *((int *) arg);
	r = x;
	return NULL;
}

int main()
{
	pthread_t t[N];

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

	return 0;
}
