#include <stdlib.h>
#include <pthread.h>
#include "../../../stdatomic.h"

#include "../cinc.c"

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[N], NULL, thread_n, NULL))
			abort();

	return 0;
}
