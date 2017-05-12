#include <stdlib.h>
#include <pthread.h>

int x;
int y;
int z;

int idx[N];

void *thread_one(void *unused)
{
	int r;

	x = 1;
	r = y;
	r = x;
	return NULL;
}

void *thread_two(void *unused)
{
	int r;

	x = 2;
	r = z;
	r = x;
	return NULL;
}

void *thread_n(void *arg)
{
	int i = *((int *) arg);

	y = i;
	z = i;
	return NULL;
}

int main()
{
	pthread_t t1, t2, t[N];

	for (int i = 0; i < N; i++) {
		idx[i] = i;
		if (pthread_create(&t[i], NULL, thread_n, &idx[i]))
			abort();
	}
	if (pthread_create(&t1, NULL, thread_one, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_two, NULL))
		abort();

	return 0;
}
