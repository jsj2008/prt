#pragma once

#include <prt/shared/basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t refcount_t;
#define refcount_add(c) __sync_fetch_and_add((refcount_t *)(&c), 1)
#define refcount_sub(c) __sync_fetch_and_sub((refcount_t *)(&c), 1)

#ifdef __cplusplus
}
#endif
