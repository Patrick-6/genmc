#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include "fake.h"
#include "qspinlock.c"

arch_spinlock_t lock;

int shared;
int idx[N];

void *thread_n(void *param)
{
	int i = *((int *) param);
	set_cpu(i);

	arch_spin_lock(&lock);
	shared = 42;
	arch_spin_unlock(&lock);
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
