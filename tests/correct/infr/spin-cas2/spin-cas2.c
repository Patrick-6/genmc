/*
 * spin-cas2: Another CAS spinloop captured by the transformation.
 */

atomic_int x;

void *thread_1()
{
	int r = x;
	while (!atomic_compare_exchange_strong(&x, &r, 42))
		;
	return NULL;
}

void *thread_2()
{
	x = 1;
	return NULL;
}
