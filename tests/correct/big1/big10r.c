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
	r = w;
	r = x;
	r = y;
	r = z;
	r = w;
	return NULL;
}

void *thread_two(void *arg)
{
	int r;

	r = x;
	r = y;
	r = z;
	r = w;
	r = x;
	r = y;
	return NULL;
}

void *thread_three(void *arg)
{
	x = 1;
	y = 1;
	x = 2;
	y = 2;
	x = 3;
	y = 3;
	return NULL;
}

void *thread_four(void *arg)
{
	z = 1;
	w = 1;
	z = 2;
	w = 2;
	z = 3;
	w = 3;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3, t4;

	if (pthread_create(&t4, NULL, thread_four, NULL))
		abort();
	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();

	return 0;
}
