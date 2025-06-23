_Atomic(atomic_int *) p;

int *x[2];

void *thread1(void *unused)
{
	p = malloc(sizeof(atomic_int));
	atomic_store(p, 42);
	return NULL;
}

void *thread2(void *unused)
{
	atomic_int *a = atomic_load_explicit(&p, memory_order_acquire);
	int r = 0;
	atomic_compare_exchange_strong(a, &r, 1);
	return NULL;
}
