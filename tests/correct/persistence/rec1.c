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

void *thread_1(void *unused)
{
	char buf[8] = {42, 42, 42, 42};

	int fd_x = open("x", 4242);
	write(fd_x, buf, 1);

	sync();

	int fd_y = open("y", 4242);
	write(fd_y, buf, 1);

	return NULL;
}

int main(int argc, char** argv)
{
	pthread_t t1;
	char buf[8];

	buf[0] = 1;
	buf[1] = 2;

	int fd_x = open("x", 4242);
	rename("x", "y"); assert(0);
	write(fd_x, buf, 1);

	int fd_y = open("y", 4242);
	write(fd_y, buf, 1);

	__VERIFIER_persistence_barrier();

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();

	if (pthread_join(t1, NULL))
		abort();

	return 0;
}

void __VERIFIER_recovery_routine(void)
{
	char buf_x[8], buf_y[8];

	int fd_y = open("y", 4242);
	read(fd_y, buf_y, 1);

	int fd_x = open("x", 4242);
	read(fd_x, buf_x, 1);

	__VERIFIER_recovery_assert(!(buf_y[0] == 42 && buf_x[0] == 1));
	return;
}
