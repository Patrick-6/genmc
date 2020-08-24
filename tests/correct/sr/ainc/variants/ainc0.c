#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#ifndef N
# define N 2
#endif

atomic_int x;

void *thread_n(void *unused)
{
	atomic_fetch_add_explicit(&x, 1, memory_order_relaxed);
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
