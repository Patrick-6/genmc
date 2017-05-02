#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int x;
int y;
int z;
int w;

void *thread_one(void *arg)
{
	int r;

	r = x;
	r = x;
	return NULL;
}

void *thread_two(void *arg)
{
	int r;

	x = 1;
	return NULL;
}

void *thread_three(void *arg)
{
	x = 2;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();

	return 0;
}
