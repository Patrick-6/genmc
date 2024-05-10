#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <genmc.h>

bool done = false;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void *thread_1(void *unused)
{
	/* do some work */
	/* ... */

	/* custom return */
	pthread_mutex_lock(&m);
	done = true;
	pthread_cond_signal(&c);
	pthread_mutex_unlock(&m);
	return NULL;
}

int main()
{
	pthread_t t;

	/* create a child */
	if (pthread_create(&t, NULL, thread_1, NULL))
		abort();

	/* custom join */
	pthread_mutex_lock(&m);
	while (!done)
		pthread_cond_wait(&c, &m);
	pthread_mutex_unlock(&m);

	return 0;
}
