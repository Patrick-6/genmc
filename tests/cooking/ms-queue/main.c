#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "../../stdatomic.h"

#include "my_queue.c"

static queue_t *queue;
static int num_threads;

queue_t myqueue;
int param[N];
unsigned int input[N];
unsigned int output[N];

int get_thread_num()
{
	return pthread_self();
}

bool succ1, succ2;

void *main_task(void *param)
{
	unsigned int val;
	int pid = *((int *)param);
	if (!pid) {
		input[0] = 17;
		enqueue(queue, input[0]);
		succ1 = dequeue(queue, &output[0]);
		//printf("Dequeue: %d\n", output[0]);
	} else {
		input[1] = 37;
		enqueue(queue, input[1]);
		succ2 = dequeue(queue, &output[1]);
	}
	return NULL;
}

int main()
{
	pthread_t threads[N];
	int i;
	unsigned int in_sum = 0, out_sum = 0;

	queue = &myqueue;;
	num_threads = N;

	init_queue(queue, num_threads);
	for (i = 0; i < num_threads; i++) {
		param[i] = i;
		pthread_create(&threads[i], NULL, main_task, &param[i]);
	}

	for (i = 0; i < num_threads; i++) {
		in_sum += input[i];
		out_sum += output[i];
	}
	/* for (i = 0; i < num_threads; i++) */
	/* 	printf("input[%d] = %u\n", i, input[i]); */
	/* for (i = 0; i < num_threads; i++) */
	/* 	printf("output[%d] = %u\n", i, output[i]); */
	/* if (succ1 && succ2) */
	/* 	assert(in_sum == out_sum); */
	/* else */
	/* 	assert(0); */

	return 0;
}
