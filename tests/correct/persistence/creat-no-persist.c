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
	int fd_y = open("y", O_RDONLY, S_IRWXU);
	int fd_x = open("x", O_RDONLY, S_IRWXU);


	read(fd_y, buf_y, 1);
	read(fd_x, buf_x, 1);

	__VERIFIER_recovery_assert(!(buf_y[0] == 42 && buf_x[0] == 0));
	return;
}

int main()
{
	int fd_x = creat("foo", S_IRWXU);
	return 0;
}
