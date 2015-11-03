#include <prt/shared/fast_hash.h>

int unsigned_pointer_compare(void *a, void *b) {
  uintptr_t _a = (uintptr_t)a;
  uintptr_t _b = (uintptr_t)b;

  if (_a > _b)
    return 1;
  else if (_a < _b)
    return -1;
  return 0;
}

int fasthash_builder_new(FastHashBuilder **out_builder) {
  FastHashBuilder *fhb;
  int r;

  fhb = NEW0(FastHashBuilder);
  if (!fhb)
    return -ENOMEM;

  r = binary_tree_new(unsigned_pointer_compare, &fhb->tree);
  if (r < 0) {
    free((void *)fhb);
    return r;
  }

  *out_builder = fhb;

  return 0;
}

int fasthash_builder_add(FastHashBuilder *builder, void *key, void *value) {
  assert(builder);

  return binary_tree_insert(builder->tree, key, value);
}

int fasthash_builder_insert(FastHashBuilder *builder, void *keys, void *values,
                            size_t n) {
  size_t i;
  int r;

  assert(builder);

  for (i = 0; i < n; ++i) {
    r = binary_tree_insert(builder->tree, keys + i, values + i);
    if (r < 0)
      return r;
  }

  return 0;
}

int fasthash_builder_unref(FastHashBuilder *builder) {
  assert(builder);

  (void)binary_tree_unref(builder->tree);
  free((void *)builder);
  return 0;
}

int fasthash_build(FastHashBuilder *builder, FastHash **out_hash) {
  FastHash *fh;
  int r;

  assert(builder);

  fh = NEW0(FastHash);
  if (!fh)
    return -ENOMEM;

  r = binary_tree_to_array(builder->tree, &fh->items, &fh->num_items);
  if (r < 0) {
    free((void *)fh);
    return r;
  }

  *out_hash = fh;

  return 0;
}

int fasthash_find(FastHash *hash, void *key, void **out_value) {
  size_t n, k;
  int cmp;

  assert(hash);

  k = hash->num_items >> 1;
  n = 0;

#define KEY_PTR_AT(h, n) *((void **)h->items + n * 2)
#define VALUE_PTR_AT(h, n) *((void **)h->items + (n * 2) + 1)

  while (k) {
    cmp = unsigned_pointer_compare(KEY_PTR_AT(hash, n), key);

    k >>= 1;

    if (cmp == 0) {
      *out_value = VALUE_PTR_AT(hash, n);
      return 0;
    } else if (cmp == -1)
      n -= k;
    else
      n += k;
  }

#undef KEY_PTR_AT
#undef VALUE_PTR_AT

  return -ENOENT;
}

int fasthash_unref(FastHash *hash) {
  free((void *)hash->items);
  free((void *)hash);
  return 0;
}
