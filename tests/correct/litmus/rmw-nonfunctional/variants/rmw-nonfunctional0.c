#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <assert.h>

#include "../rmw-nonfunctional.c"

int main() {
	pthread_t t[N];

	for (size_t i = 0; i < N; i++)
                pthread_create(&t[i], 0, &run, (void *) i);

	for (intptr_t i = 0; i < N; i++)
		pthread_join(t[i], NULL);

        return 0;
}
