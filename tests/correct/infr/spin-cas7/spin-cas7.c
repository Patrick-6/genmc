/*
 * spin-cas7: Invalid PHIs in the CAS loop.
 * Transformation should fail in this case as well.
 */

atomic_int x;

void *thread_1()
{
	int r = x + 1;
	while (!atomic_compare_exchange_strong(&x, &r, 42))
		;
	return NULL;
}

void *thread_2()
{
	x = 1;
	return NULL;
}
