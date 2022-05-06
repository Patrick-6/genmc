#ifndef __STDLIB_H__
#define __STDLIB_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <genmc_internal.h>

void exit(int);
int atexit(void (*func)(void)) { return __VERIFIER_atexit(func); }

void abort(void);
int abs(int);
int atoi(const char *);
char *getenv(const char *);

#define free(ptr) __VERIFIER_free(ptr)

#define malloc(size) __VERIFIER_malloc(size)

#define aligned_alloc(align, size) __VERIFIER_malloc_aligned(align, size)

#ifdef __cplusplus
}
#endif

#endif /* __STDLIB_H__ */
