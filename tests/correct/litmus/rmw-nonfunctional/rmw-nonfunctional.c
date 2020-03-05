#define N 3

static atomic_int x = 0;
static atomic_int cnt;

static atomic_int rf[N + 1];

static pthread_mutex_t lock;
static int t;

static void *run(void *arg)
{
	int old = atomic_fetch_add(&x,1);
	assert(atomic_fetch_add_explicit(&rf[old],1, memory_order_acquire) == 0);
	atomic_fetch_add_explicit(&cnt,1,memory_order_acquire);
	return NULL;
}
