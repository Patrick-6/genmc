#include <stdlib.h>
#include <pthread.h>

typedef int elem_t;

void myinit(elem_t *loc, elem_t val);
elem_t mylock(elem_t *loc);
void myunlock(elem_t *loc, elem_t val);

elem_t l;

void *thread_n(void *unused)
{
	mylock(&l);
	myunlock(&l, 0);
	return NULL;
}

int main()
{
	pthread_t t[N];

	myinit(&l, 0);

	for (int i = 0; i < N; i++)
		if (pthread_create(&t[i], NULL, thread_n, NULL))
			abort();

	for (int i = 0; i < N; i++)
		if (pthread_join(t[i], NULL))
			abort();

	return 0;
}
