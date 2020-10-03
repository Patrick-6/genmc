/*
 * spin-cas6: Multiple exiting blocks for the spinloop.
 * Transformation should fail because of that.
 */

atomic_int x;

void *thread_1()
{
	int r = x;
	while (!atomic_compare_exchange_strong(&x, &r, 42))
		if (x == 1)
			break;
	return NULL;
}

void *thread_2()
{
	x = 1;
	return NULL;
}
