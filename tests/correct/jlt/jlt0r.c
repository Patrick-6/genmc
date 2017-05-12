#include <stdlib.h>
#include <pthread.h>

int x;
int y;

void *thread_one(void *arg)
{
	int r;
	
	x = 1;
	r = y;
	r = x;
	return NULL;
}

void *thread_two(void *arg)
{
	int r;
	
	x = 2;
	r = x;
	return NULL;
}

void *thread_three(void *arg)
{
	y = 3;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t3;

	if (pthread_create(&t3, NULL, thread_three, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();
	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();

	return 0;
}
