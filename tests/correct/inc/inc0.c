#include <stdlib.h>
#include <pthread.h>

int x;

int idx[N];

void *thread_n(void *unused)
{
	int r = x;
	x = r + 1;
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread_n, NULL))
			abort();

	return 0;
}
