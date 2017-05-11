#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

int x;
int y;
int c;

void *thread_one(void *arg)
{
	y = 1;
	if (!x) {
		c = 1;
		assert(c == 1);
	}
	return NULL;
}

void *thread_two(void *arg)
{
	x = 1;
	if (!y) {
		c = 0;
		assert(c == 0);
	}
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
