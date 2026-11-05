#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#if defined(_WIN32)

#include <windows.h>

typedef volatile LONG atomic_int;

static inline LONG
atomic_load(const atomic_int* x)
{
    return InterlockedCompareExchange(
        (LONG*)x, 0, 0
    );
}

static inline void
atomic_store(atomic_int* x, LONG value)
{
    InterlockedExchange(x, value);
}

static inline LONG
atomic_fetch_add(atomic_int* x, LONG value)
{
    return InterlockedExchangeAdd(x, value);
}

static inline LONG
atomic_fetch_sub(atomic_int* x, LONG value)
{
    return InterlockedExchangeAdd(x, -value);
}

#elif defined(__GNUC__) || defined(__clang__)

typedef int atomic_int;

static inline int
atomic_load(const atomic_int* x)
{
    return __atomic_load_n(x, __ATOMIC_SEQ_CST);
}

static inline void
atomic_store(atomic_int* x, int value)
{
    __atomic_store_n(x, value, __ATOMIC_SEQ_CST);
}

static inline int
atomic_fetch_add(atomic_int* x, int value)
{
    return __atomic_fetch_add(x, value, __ATOMIC_SEQ_CST);
}

static inline int
atomic_fetch_sub(atomic_int* x, int value)
{
    return __atomic_fetch_sub(x, value, __ATOMIC_SEQ_CST);
}

#endif

#endif