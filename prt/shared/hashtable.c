#include <prt/shared/hashtable.h>

/* used during add_str and rehash  */
static Id _hashtable_hash_string(void *p) {
  const char *str = p;
  return HASH(str);
}

/* used during key comparisons */
static int _hashtable_cmp_string(const void *a, const void *b) {
  if (a == b)
    return 0;

  return strcmp((const char *)a, (const char *)b);
}

/* 2*n expansion strategy */
static size_t _hashtable_calc_size(size_t old_size) { return old_size << 1; }

/* With 4 elements per bucket, the buffer has
 * 16 bytes per element, so we get 64 bytes per
 * bucket and that should fit nicely into single
 * data lane in most L1 CPU caches; which makes
 * linear probing almost for free.
 */
static float _hashtable_load_factor(size_t buckets, size_t items) {
  return (float)items / ((float)buckets * 4.0f);
}

static int _hashtable_insert_bucket(Bucket *bucket, void *key, void *value) {
  assert(bucket);

  bucket->items =
      reallocarray(bucket->items, sizeof(Element), bucket->num_items + 1);

  if (!bucket->items)
    return -ENOMEM;

  bucket->num_items++;
  bucket->items[bucket->num_items - 1].key = key;
  bucket->items[bucket->num_items - 1].value = value;

  return 0;
}

static int _hashtable_resize_and_rehash(Hashtable *hash) {
  BitVector *vector;
  Element *element;
  Bucket *new_buckets;
  size_t new_size, new_loc, old_size, i, j;
  int r;
  assert(hash);

  old_size = hash->num_buckets;
  new_size = _hashtable_calc_size(hash->num_buckets);

  r = bitvector_new(new_size, &vector);
  if (r < 0)
    return r;

  new_buckets = (Bucket *)calloc(sizeof(Bucket), new_size);

  for (i = 0; i < old_size; ++i) {
    for (j = 0; j < hash->buckets[i].num_items; ++j) {
      element = &hash->buckets[i].items[j];

      new_loc = hash->hash_func(element->key) % new_size;

      (void)bitvector_set_bit(vector, new_loc, true);
      r = _hashtable_insert_bucket(&new_buckets[new_loc], element->key,
                                   element->value);
      if (r < 0)
        goto err_insert;
    }
  }

  free((void *)hash->buckets);
  hash->buckets = new_buckets;
  free((void *)hash->vector);
  hash->vector = vector;

  return 0;
err_insert:
  /* the current insert failed, so move one back */
  for (--i; i != BV_INT_MAX; --i)
    free((void *)new_buckets[i].items);
  free((void *)new_buckets);
  bitvector_unref(vector);
  return r;
}

int hashtable_new(size_t buckets, Hashtable **out_hash) {
  Hashtable *sh;
  size_t size;
  int r = -ENOMEM;
  assert(out_hash);

  size = _hashtable_calc_size(buckets);

  sh = NEW0(Hashtable);
  if (!sh)
    return r;

  sh->buckets = (Bucket *)calloc(sizeof(Bucket), size);
  if (!sh->buckets)
    goto err;

  r = bitvector_new(size, &sh->vector);
  if (r < 0)
    goto err;

  sh->key_cmp = _hashtable_cmp_string;
  sh->hash_func = _hashtable_hash_string;

  sh->num_buckets = size;
  sh->rehash_factor = 0.75f;
#ifdef HASH_SYNCHRONIZED
  lock_init(&sh->lock);
#endif
  *out_hash = sh;

  return 0;

err:
  if (sh)
    free((void *)sh);
  if (sh->buckets)
    free((void *)sh->buckets);

  return r;
}

int hashtable_add(Hashtable *hash, Id id, void *key, void *value) {
  size_t index;
  int r;
  assert(hash);

  r = hashtable_find(hash, id, key, NULL);
  if (!r) {
    r = -EEXIST;
    goto out;
  } else
    r = 0;

#ifdef HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  if (_hashtable_load_factor(hash->num_buckets, hash->num_items + 1) >
      hash->rehash_factor) {
    r = _hashtable_resize_and_rehash(hash);
    if (r < 0)
      goto out;
  }

  index = id % hash->num_buckets;

  r = bitvector_set_bit(hash->vector, index, true);
  if (r < 0)
    goto out;

  r = _hashtable_insert_bucket(&hash->buckets[index], key, value);
  if (r < 0) {
    /* reset the bit if insertion failed and the bucket is empty */
    if (hash->buckets[index].num_items > 0)
      (void)bitvector_set_bit(hash->vector, index, false);
    goto out;
  }

  hash->num_items++;

out:
#ifdef HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int hashtable_add_str(Hashtable *hash, const char *key, void *value) {
  assert(hash);
  assert(key);
  return hashtable_add(hash, hash->hash_func((void *)key), (void *)key, value);
}

int hashtable_find(Hashtable *hash, Id id, void *key, void **out_value) {
  size_t index, i;
  bool v;
  int r;
  assert(hash);

  r = -ENOENT;

#ifdef HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  index = id % hash->num_buckets;
  (void)bitvector_get_bit(hash->vector, index, &v);
  if (!v)
    goto out;

  for (i = 0; i < hash->buckets[index].num_items; ++i) {
    if (hash->key_cmp(hash->buckets[index].items[i].key, key) == 0) {
      if (out_value)
        *out_value = hash->buckets[index].items[i].value;
      r = 0;
      goto out;
    }
  }

out:
#ifdef HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int hashtable_remove(Hashtable *hash, Id id, void *key, void **out_value) {
  size_t index, i, last;
  Id tkey;
  void *tvalue;
  int r;
  bool v;
  assert(hash);

  r = -ENOENT;

#ifdef HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  index = id % hash->num_buckets;
  (void)bitvector_get_bit(hash->vector, index, &v);
  if (!v)
    goto out;

  for (i = 0; i < hash->buckets[index].num_items; ++i) {
    if (hash->key_cmp(hash->buckets[index].items[i].key, key) == 0) {
      if (hash->buckets[index].num_items == 1) {
        /* guaranteed to not reallocate */
        (void)bitvector_set_bit(hash->vector, index, false);
        free((void *)hash->buckets[index].items);
      } else {
        last = hash->buckets[index].num_items - 1;

        if (i != last) {
          hash->buckets[index].items[i].key =
              hash->buckets[index].items[last].key;
          hash->buckets[index].items[i].value =
              hash->buckets[index].items[last].value;
        }

        hash->buckets[index].items =
            reallocarray(hash->buckets[index].items, sizeof(Element), last);
        if (!hash->buckets[index].items) {
          r = -ENOMEM;
          goto out;
        }
      }

      if (out_value)
        *out_value = hash->buckets[index].items[i].value;

      hash->num_items--;
      hash->buckets[index].num_items--;

      r = 0;
      goto out;
    }
  }

out:
#ifdef HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int hashtable_unref(Hashtable *hash) {
  assert(hash);

#ifdef HASH_SYNCHRONIZED
  lock_unref(&hash->lock);
#endif
  bitvector_unref(hash->vector);
  free((void *)hash->buckets);
  free((void *)hash);

  return 0;
}

int hashtable_iterate(Hashtable *hash, HtIterator **out_iterator) {
  HtIterator *it;
  size_t next;
  assert(hash);
  assert(out_iterator);

  it = NEW0(HtIterator);
  if (!it)
    return -ENOMEM;

  it->hashtable = hash;

#ifdef HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  *out_iterator = it;

  return 0;
}

bool hashtable_iterator_next(HtIterator *iterator, void **out_key,
                             void **out_value) {
  Element *e;
  Hashtable *h;
  size_t next;
  int r;
  assert(iterator);

  h = iterator->hashtable;

  if (h->num_items == iterator->seen)
    return false;

  if (iterator->probe >= h->buckets[iterator->offset].num_items) {
    iterator->probe = 0;

    r = bitvector_next_set_bit(iterator->hashtable->vector, iterator->offset,
                               &next);
    if (r < 0)
      return false;

    iterator->offset = next;
    if (h->buckets[iterator->offset].num_items)
      goto success;

    while (!h->buckets[iterator->offset].num_items) {
      r = bitvector_next_set_bit(iterator->hashtable->vector, iterator->offset,
                                 &next);
      if (r < 0)
        return false;

      iterator->offset = next;
    }
  }

success:
  e = &h->buckets[iterator->offset].items[iterator->probe];
  if (out_key)
    *out_key = e->key;
  if (out_value)
    *out_value = e->value;
  iterator->probe++;
  iterator->seen++;
  return true;
}

bool hashtable_iterator_end(HtIterator *iterator) {
  assert(iterator);

  return iterator->hashtable->num_items == iterator->seen;
}

int hashtable_iterator_unref(HtIterator *iterator) {
  assert(iterator);

#ifdef HASH_SYNCHRONIZED
  lock_release(&iterator->hashtable->lock);
#endif
  free((void *)iterator);
  return 0;
}
