#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>

#include "../main.c"

int main()
{
	num_threads = N;
	threads[0] = pthread_self();

	add(&myheap, 0, 0);
	for (int i = 0; i < num_threads; i++)
		if (pthread_create(&threads[i + 1], NULL, thread_n, NULL))
			abort();

	for (int i = 0; i < num_threads; i++)
		if (pthread_join(threads[i + 1], NULL))
			abort();

	/* for (int i = 0; i < num_threads; i++) { */
	/* 	if (pthread_create(&threads[i], NULL, thread_n, NULL)) */
	/* 		abort(); */
	/* } */

	/* for (int i = 0; i < num_threads; i++) { */
	/* 	if (pthread_join(threads[i], NULL)) */
	/* 		abort(); */
	/* } */

	/* for (int i = 0; i < myheap.next; i++) */
	/* 	printf("%d ", myheap.nodes[i].score); */
	/* printf("success\n"); */

	return 0;
}
