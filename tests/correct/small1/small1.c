int _Atomic x;
int _Atomic y;
int _Atomic z;
int _Atomic w;

void *thread_one(void *arg)
{
	int r;

	r = atomic_load_explicit(&z, memory_order_acquire);
	r = atomic_load_explicit(&w, memory_order_acquire);
	r = atomic_load_explicit(&x, memory_order_acquire);
	return NULL;
}

void *thread_two(void *arg)
{
	atomic_store_explicit(&x, 1, memory_order_release);
	return NULL;
}

void *thread_three(void *arg)
{
	atomic_store_explicit(&z, 1, memory_order_release);
	atomic_store_explicit(&w, 1, memory_order_release);
	return NULL;
}
