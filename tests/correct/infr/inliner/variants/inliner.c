int mut_rec1(int size);

/* mut_rec* functions will not be inlined anymore */
int mut_rec3(int size)
{
	return size == 0 ? 0 : mut_rec1(size - 1);
}

int mut_rec2(int size)
{
	return size == 0 ? 0 : mut_rec3(size - 1);
}

int mut_rec1(int size)
{
	return size == 0 ? 0 : mut_rec2(size - 1);
}

/* Hacky way to test inlining: if this function is not inlined, GenMC will (correctly) complain
 * about a read from uninitialized memory (although i is unused/undef) */
int f2(int i)
{
	return mut_rec1(3);
}

int f1(int i)
{
	return f2(i);
}

int main()
{
	int i; /* uninit */
	return f1(i);
}
