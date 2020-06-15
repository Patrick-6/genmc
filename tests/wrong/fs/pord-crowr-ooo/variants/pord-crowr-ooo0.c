#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <genmc.h>

#include <fcntl.h>
#include <sys/stat.h>

void __VERIFIER_recovery_routine(void)
{
	char buf[2];
	int fd = open("foo", O_RDONLY, S_IRWXU);
	if (fd == -1)
		return;

	int fb = open("bar", O_RDONLY, 0640);
	int nr = read(fd, buf, 2);

	/* Is it possible to see the append but not "bar"? */
	__VERIFIER_recovery_assert(!(nr == 2 && fb == -1));
	return;
}

int main()
{
	char buf[2] = "00";

	int fd = creat("foo", S_IRWXU);

	__VERIFIER_persistence_barrier();

	int fd2 = creat("bar", 0640);
	/* We won't do any operations on bar */

	pwrite(fd, buf, 2, 0);
	close(fd);
	close(fd2);

	return 0;
}
