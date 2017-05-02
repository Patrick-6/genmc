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
	r = y;
	r = z;
	r = x;
	r = y;
	return NULL;
}

void *thread_two(void *arg)
{
	int r;

	r = x;
	r = y;
	r = z;
	r = x;
	return NULL;
}

void *thread_three(void *arg)
{
	x = 1;
	y = 1;
	z = 1;
	x = 2;
	y = 2;
	z = 2;
	return NULL;
}

void *thread_four(void *arg)
{
	x = 3;
	y = 3;
	z = 3;
	x = 4;
	y = 4;
	z = 4;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3, t4;

	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();
	if (pthread_create(&t4, NULL, thread_four, NULL))
		abort();

	return 0;
}
