#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "stdatomic.h"

int x = 3;
int y = 4;

int *p;

void *thread_one(void *unused)
{
	int c1 = 0;
	
	p = &y;
	for (int i = 0; i < N; i++)
		c1 += x;
	*p += 3;
	/* assert(3 <= x && x <= 9); */
	/* assert(3 <= y && y <= 9); */
	return NULL;
}

void *thread_two(void *unused)
{
	int c2 = 0;
	
	p = &x;
	for (int i = 0; i < N; i++)
		c2 += y;
	*p += 2;
	/* assert(3 <= x && x <= 9); */
	/* assert(3 <= y && y <= 9); */
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	if (pthread_create(&t2, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_two, NULL))
		abort();

	return 0;
}
