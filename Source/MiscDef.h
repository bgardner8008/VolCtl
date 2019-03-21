/*
 * Miscellaneous and often needed preprocessor definitions.
 */
#ifndef _MISC_DEF_H
#define _MISC_DEF_H

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

/*
 * C++ really complains about pointer types.
 */
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#endif

#ifndef NELEMS
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))
#define NELEM(array) (sizeof(array) / sizeof(array[0]))
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

// This is a simple way to do compile time assertions. If the test is not true,
// then the array will have a negative number of elements and the compiler will
// produce an error.
#define WA_STATIC_ASSERT(test) typedef char assertion_on_mystruct[( !!(test) )*2-1 ]

#endif
