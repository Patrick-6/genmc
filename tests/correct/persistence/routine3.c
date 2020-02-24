#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <genmc.h>

#include <fcntl.h>
#include <sys/stat.h>

atomic_int x;
atomic_int y;

void *thread_1(void *unused)
{
	char buf[8];

	int fd = open("x", 4242);

	read(fd, buf, 4);

	/* printf("%d, %d, %d\n", buf[0], buf[1], buf[2]); */

	/* open("another", 42); */
	/* x = 1; */
	/* int r = y; */
	/* read(1, NULL, 42); */
	return NULL;
}

void *thread_2(void *unused)
{
	char buf[8];
	buf[0] = 42;

	int fd = open("x", 4242);

	write(fd, buf, 4);

/* 	char *str = "this is a random string\n"; */

/* 	open("file", 42); */

/* 	y = 1; */
/* 	int r = x; */
/* 	write(1, NULL, 42); */
 	return NULL;
}

int main()
{
	pthread_t t1, t2;
	char buf[8];

	buf[0] = 1;
	buf[1] = 2;

	int fd = open("x", 4242);
	write(fd, buf, 1);

	if (pthread_create(&t1, NULL, thread_1, NULL))
		abort();
	if (pthread_create(&t2, NULL, thread_2, NULL))
		abort();

	if (pthread_join(t1, NULL))
		abort();
	if (pthread_join(t2, NULL))
		abort();

	return 0;
}

void __VERIFIER_recovery_routine(void)
{
	char buf[8];
	buf[0] = 17;

	int fd = open("x", 4242);

	read(fd, buf, 1);
	return;
}
