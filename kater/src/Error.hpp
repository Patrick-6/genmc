#include <config.h>

#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#define EPARSE 5
#define ECHECK 6
#define EPRINT 7

/* Provide LLVM-style DEBUG_WITH_TYPE utilities */
#ifdef ENABLE_KATER_DEBUG

extern bool katerDebug;

bool isCurrentDebugType(const char *);
void addDebugType(const char *);

#define DEBUG_WITH_TYPE(TYPE, X)					\
do {									\
	if (katerDebug && isCurrentDebugType(TYPE)) { X; }		\
} while (false)

#else /* !ENABLE_KATER_DEBUG */

#define isCurrentDebugType(X) (false)
#define addDebugType(X)
#define DEBUG_WITH_TYPE(TYPE, X) do {} while (0)
#endif /* ENABLE_KATER_DEBUG */

#define KATER_DEBUG(X) DEBUG_WITH_TYPE(DEBUG_TYPE, X)

#endif /* __ERROR_HPP__ */
