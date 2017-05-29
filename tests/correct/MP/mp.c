int _Atomic x;
int _Atomic y;

void *thread_one(void *arg)
{
	int r;

	atomic_store_explicit(&x, 1, memory_order_release);
	atomic_store_explicit(&y, 1, memory_order_release);
	return NULL;
}

void *thread_two(void *arg)
{
	int r_y, r_x;

	r_y = atomic_load_explicit(&y, memory_order_acquire);
	r_x = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}
