#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include "fake.h"
#include "mcs_spinlock.h"

_Atomic(struct mcs_spinlock *) lock;

int shared;
int idx[N];
struct mcs_spinlock nodes[N];

void *thread_n(void *param)
{
	int i = *((int *) param);
	mcs_spin_lock(&lock, &nodes[i]);
	shared = 42;
	mcs_spin_unlock(&lock, &nodes[i]);
	return NULL;
}

int main()
{
	pthread_t t[N];

	for (int i = 0; i < N; i++) {
		idx[i] = i;
		if (pthread_create(&t[i], NULL, thread_n, &idx[i]))
			abort();
	}

	return 0;
}
