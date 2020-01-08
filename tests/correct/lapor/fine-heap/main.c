#include <assert.h>
#include "fine-heap.c"

/* Driver code */
#ifndef N
# define N 2
#endif

#ifndef MAX_THREADS
# define MAX_THREADS 32
#endif

static int num_threads;
pthread_t threads[MAX_THREADS + 1];

int get_thread_num()
{
	pthread_t curr = pthread_self();
	for (int i = 0; i <= num_threads; i++)
		if (curr == threads[i])
			return i;
	assert(0);
	return -1;
}

DEFINE_HEAP(myheap);

void *thread_n(void *unused)
{
	int t = get_thread_num();

	if (t % 2 == 0)
		add(&myheap, t * 2, t);
	else
		add(&myheap, t / 2, t);
	return NULL;
}
