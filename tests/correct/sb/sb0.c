#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int x;
int y;

void *thread_one(void *arg)
{
	int r;

	x = 1;
	r = y;
	return NULL;
}

void *thread_two(void *arg)
{
	int r;

	y = 1;
	r = x;
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
