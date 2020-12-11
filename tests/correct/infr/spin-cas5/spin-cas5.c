/*
 * spin-cas5: PHI nodes for other variables apart from the CAS one.
 * Transformation should fail.
 */

atomic_int x;

void *thread_1()
{
	int a = 0;
	int r = x;
	while (!atomic_compare_exchange_strong(&x, &r, 42))
		a++;
	return NULL;
}

void *thread_2()
{
	x = 1;
	return NULL;
}
