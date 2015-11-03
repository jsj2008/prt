#include <prt/shared/pool.h>

int string_pool_new(size_t capacity, StringPool **out_pool) {
  StringPool *pool;
  assert(out_pool);

  pool = NEW0(StringPool);
  if (!pool)
    return -ENOMEM;

  pool->pool = NEW0N(char *, capacity);
  if (!pool->pool) {
    free((void *)pool);
    return -ENOMEM;
  }

  pool->capacity = capacity;

  *out_pool = pool;

  return 0;
}

int string_pool_add(StringPool *pool, const char *str, size_t *out_index) {
  char *c;
  assert(pool);
  assert(str);
  assert(out_index);

  c = strdup(str);
  if (!c)
    return -ENOMEM;

  if (pool->free_list != NULL) {
    pool->pool[pool->free_list->free_idx] = c;
    *out_index = pool->free_list->free_idx;
    pool->free_list = pool->free_list->next_free;
    return 0;
  }

  if (pool->capacity <= (pool->head - 1))
    return -EFBIG;

  pool->pool[pool->head] = c;
  *out_index = pool->head;

  pool->head++;

  return 0;
}

int string_pool_get(StringPool *pool, size_t index, char **out_str) {
  assert(pool);
  assert(out_str);

  if (pool->capacity <= index)
    return -EINVAL;

  if (pool->pool[index] == NULL)
    return -ENOENT;

  *out_str = pool->pool[index];

  return 0;
}

int string_pool_remove(StringPool *pool, size_t index) {
  Freelist *f, *last;
  assert(pool);

  if (index >= pool->capacity || !pool->pool[index])
    return -EINVAL;

  f = NEW0(Freelist);
  if (!f)
    return -ENOMEM;

  f->free_idx = index;
  last = pool->free_list;

  if (!last) {
    pool->free_list = f;
    free((void *)pool->pool[index]);
    return 0;
  }

  while (last->next_free != NULL)
    last = last->next_free;
  last->next_free = f;

  free((void *)pool->pool[index]);

  return 0;
}

int string_pool_unref(StringPool *pool) {
  size_t i;
  assert(pool);

  for (i = 0; i < pool->capacity; ++i)
    if (pool->pool[i])
      free((void *)pool->pool[i]);

  free((void *)pool->pool);
  free((void *)pool);

  return 0;
}
