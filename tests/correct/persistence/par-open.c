#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <genmc.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

atomic_int x;
atomic_int y;

void __VERIFIER_recovery_routine(void)
{
	/* printf("Nothing to do\n"); */
	return;
}

void *thread_1(void *unused)
{
	int fd = open("foo", O_RDONLY, S_IRWXU);
	assert(fd != -1);
	return NULL;
}

void *thread_2(void *unused)
{
	int fd = open("foo", O_RDONLY, S_IRWXU);
	assert(fd != -1);
	return NULL;
}

int main()
{
	pthread_t t1, t2;

	/* Create the file */
	int fd = creat("foo", S_IRWXU);
	assert(fd != -1);

	__VERIFIER_persistence_barrier();

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();

	return 0;
}
