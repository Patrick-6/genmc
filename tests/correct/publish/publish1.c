#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

int x;
int flag;

void *thread_writer(void *arg)
{
	x = 42;
	flag = 1;
	return NULL;
}

void *thread_reader(void *arg)
{
	int local = 0;
	int count = 0;
	
	local = flag;
	while (local != 1) {
		count++;
		if (count > 40)
			return NULL;
		local = flag;
	}
	// printf("got it!\n");
	assert(x == 42);
	return NULL;
}

int main()
{
	pthread_t tr, tw;
	
	if (pthread_create(&tr, NULL, thread_writer, NULL))
		abort();
	if (pthread_create(&tw, NULL, thread_reader, NULL))
		abort();	

	return 0;
}
