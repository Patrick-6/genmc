/*
 * spin-cas4: A more complicated CAS spinloop captured by the transformation.
 */

atomic_int x, y;

void *thread_1()
{
	int r = x;
	while (!atomic_compare_exchange_strong(&x, &r, 42)) {
		if (y == 0)
			r = x;
	}
	return NULL;
}

void *thread_2()
{
	x = 1;
	y = 1;
	return NULL;
}
