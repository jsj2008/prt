#pragma once

#include <src/shared/basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Freelist {
  size_t free_idx;
  struct _Freelist *next_free;
} Freelist;

typedef struct _StringPool {
  char **pool;
  size_t capacity;
  size_t head;
  Freelist *free_list;
} StringPool;

int string_pool_new(size_t capacity, StringPool **out_pool);
int string_pool_add(StringPool *pool, const char *str, size_t *out_index);
int string_pool_get(StringPool *pool, size_t index, char **out_str);
int string_pool_remove(StringPool *pool, size_t index);
int string_pool_unref(StringPool *pool);

#ifdef __cplusplus
}
#endif
