/*
 * spin-cas3: CAS spinloop captured by the transformation.
 */

atomic_int x;

void *thread_n()
{
	int r = 0;
	while (!atomic_compare_exchange_strong(&x, &r, 1))
		r = 0;
	x = 0;
	return NULL;
}
