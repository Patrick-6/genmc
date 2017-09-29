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
