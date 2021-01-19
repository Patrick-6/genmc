#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#include "../spin-cas3.c"

int main()
{
	pthread_t t[N];

	for (unsigned i = 0; i < N; i++) {
		if (pthread_create(&t[i], NULL, thread_n, NULL))
			abort();
	}

	return 0;
}
