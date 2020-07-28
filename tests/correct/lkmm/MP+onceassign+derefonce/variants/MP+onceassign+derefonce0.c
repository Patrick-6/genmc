#include <stdlib.h>
#include <lkmm.h>
#include <pthread.h>
#include <assert.h>

atomic_t x;
atomic_t z;
_Atomic(atomic_t *) y;

void *P0(void *unused)
{
	WRITE_ONCE(x, 1);
	rcu_assign_pointer(y, &x);
}

void *P1(void *unused)
{
	atomic_t *r0;
	int r1;

	/* rcu_read_lock(); */
	r0 = rcu_dereference(y);
	r1 = READ_ONCE(*r0);
	/* rcu_read_unlock(); */
}


int main()
{
	pthread_t t0, t1;

	WRITE_ONCE(y, &z);

	pthread_create(&t0, NULL, P0, NULL);
	pthread_create(&t1, NULL, P1, NULL);

	return 0;
}
