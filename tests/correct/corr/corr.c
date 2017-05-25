int _Atomic x;
int _Atomic y;

void *thread_one(void *arg)
{
	int r;

	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_store_explicit(&x, 2, memory_order_release);
	return NULL;
}

void *thread_two(void *arg)
{
	int r_x;

	r_x = atomic_load_explicit(&x, memory_order_acquire);
	r_x = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}
