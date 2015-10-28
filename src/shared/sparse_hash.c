#include <src/shared/sparse_hash.h>

static size_t _sparse_hash_calc_size(size_t old_size) { return old_size << 1; }

static float _sparse_hash_load_factor(size_t buckets, size_t items) {
  return (float)items / ((float)buckets * 4.0f);
}

static size_t _sparse_bucket_size(Element *bucket) {
  return PTR_TO_ULONG((bucket - 1)->value);
}

static size_t _sparse_bucket_size_set(Element *bucket, size_t size) {
  (bucket - 1)->value = ULONG_TO_PTR(size);
  return 0;
}

/* the bucket parameter is modified to point to the newly allocated bucket */
static int _sparse_hash_insert_bucket(Element **bucket, Id key, void *value) {
  void *mh;
  Element *b;
  size_t s, i;

  s = 0;

  b = *bucket;

  if (b) {
    s = _sparse_bucket_size(b);
    *bucket = b = (Element *)reallocarray(b - 1, sizeof(Element), s + 2) + 1;
  } else
    *bucket = b = (Element *)calloc(sizeof(Element), 2) + 1;

  if (!b)
    return -ENOMEM;

  (void)_sparse_bucket_size_set(b, s + 1);

  b[s].key = key;
  b[s].value = value;

  return 0;
}

/* appends new bucket and returns it, `hash->vector` is not updated */
static int _sparse_hash_append_bucket(SparseHash *hash, Element ***out_bucket) {
  assert(hash);

  /* assert ? */
  if (hash->num_buckets >= hash->capacity)
    return -EINVAL;

  if (!hash->buckets)
    hash->buckets = calloc(sizeof(Element *), 1);
  else
    hash->buckets = (Element **)reallocarray(hash->buckets, sizeof(Element *),
                                             hash->num_buckets + 1);

  if (!hash->buckets)
    return -ENOMEM;

  hash->num_buckets++;
  *out_bucket = hash->buckets + (hash->num_buckets - 1);

  return 0;
}

/* NOT IMPLEMENTED! */
static int _sparse_hash_resize_and_rehash(SparseHash *hash) {
  /*BitVector *vector;
  Element *element;
  size_t new_size, new_loc, old_size, i, j;
  int r;
  assert(hash);*/
  assert(!"Resizing / rehashing a sparse hash is not supported at the moment");
  /*
  old_size = hash->num_buckets;
  new_size = _sparse_hash_calc_size(hash->num_buckets);

  r = bitvector_new(new_size, &vector);
  if (r < 0)
    return r;

  free((void *)hash->vector);
  hash->vector = vector;
   new_buckets = (Bucket *)calloc(sizeof(Bucket), new_size);

   for (i = 0; i < old_size; ++i) {
     for (j = 0; j < hash->buckets[i].num_items; ++j) {
       element = &hash->buckets[i].items[j];

       new_loc = element->key % new_size;

       (void)bitvector_set_bit(vector, new_loc, true);
       (void)_sparse_hash_insert_bucket(&new_buckets[new_loc], element->key,
                                        element->value);
     }
   }

   free((void *)hash->buckets);
   hash->buckets = new_buckets;
 */
  return 0;
}

static int _sparse_hash_elt_swap(Element *bucket, size_t a, size_t b) {
  Id old_key;
  void *old_value;
  assert(bucket);

  old_key = bucket[a].key;
  old_value = bucket[a].value;
  bucket[a].key = bucket[b].key;
  bucket[a].value = bucket[b].value;
  bucket[b].key = old_key;
  bucket[b].value = old_value;

  return 0;
}

static int _sparse_hash_swap(SparseHash *hash, size_t a, size_t b) {
  Element *p;
  assert(hash);

  p = hash->buckets[a];
  hash->buckets[a] = hash->buckets[b];
  hash->buckets[b] = p;

  return 0;
}

int sparse_hash_new(size_t buckets, SparseHash **out_hash) {
  SparseHash *sh;
  size_t size;
  int r = -ENOMEM;

  size = _sparse_hash_calc_size(buckets);

  sh = NEW0(SparseHash);
  if (!sh)
    return r;

  r = bitvector_new(size, &sh->vector);
  if (r < 0)
    goto err;

  sh->capacity = size;
  sh->rehash_factor = 0.75f;
#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_init(&sh->lock);
#endif
  *out_hash = sh;

  return 0;

err:
  if (sh)
    free((void *)sh);

  return r;
}

int sparse_hash_add(SparseHash *hash, Id key, void *value) {
  size_t index, next, size, count, after;
  Element **bucket;
  int r;
  bool v;
  assert(hash);

  r = sparse_hash_find(hash, key, NULL);
  if (!r)
    return -EEXIST;

#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  if (_sparse_hash_load_factor(hash->capacity, hash->num_items + 1) >
      hash->rehash_factor)
    (void)_sparse_hash_resize_and_rehash(hash);

  index = key % hash->capacity;

  bitvector_count_bits(hash->vector, index, &count);
  /* do we already have this bucket? */
  (void)bitvector_get_bit(hash->vector, index, &v);
  if (!v) {
    r = bitvector_set_bit(hash->vector, index, true);
    if (r < 0)
      goto out;

    r = _sparse_hash_append_bucket(hash, &bucket);
    if (r < 0)
      goto out;

    r = _sparse_hash_insert_bucket(bucket, key, value);
    if (r < 0)
      goto out;

    if (hash->num_buckets > 1)
      _sparse_hash_swap(hash, count, hash->num_buckets - 1);

    while ((ssize_t)count++ < ((ssize_t)hash->num_buckets - 1))
      _sparse_hash_swap(hash, count, hash->num_buckets - 1);

  } else {
    r = _sparse_hash_insert_bucket(&hash->buckets[count], key, value);
    if (r < 0)
      goto out;
  }

  hash->num_items++;

out:
  if (r < 0)
    bitvector_set_bit(hash->vector, index, false);
#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int sparse_hash_find(SparseHash *hash, Id key, void **out_value) {
  size_t index, i, count;
  bool v;
  int r;
  assert(hash);

#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  r = -ENOENT;
  index = key % hash->capacity;
  (void)bitvector_get_bit(hash->vector, index, &v);
  if (!v)
    goto out;

  (void)bitvector_count_bits(hash->vector, index, &count);

  for (i = 0; i < _sparse_bucket_size(hash->buckets[count]); ++i) {
    if (hash->buckets[count][i].key == key) {
      if (out_value)
        *out_value = hash->buckets[count][i].value;
      r = 0;
      goto out;
    }
  }

out:
#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int sparse_hash_remove(SparseHash *hash, Id key, void **out_value) {
  size_t index, i, size, count, next;
  Id tkey;
  void *tvalue;
  int r;
  bool v;
  assert(hash);

  r = -ENOENT;

#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_acquire(&hash->lock);
#endif

  index = key % hash->capacity;
  (void)bitvector_get_bit(hash->vector, index, &v);

  if (!v)
    goto out;

  (void)bitvector_count_bits(hash->vector, index, &count);
  size = _sparse_bucket_size(hash->buckets[count]);

  if (size == 1) {
    /* free the bucket */
    (void)bitvector_set_bit(hash->vector, index, false);
    free((void *)(hash->buckets[count] - 1));

    next = index;

    while (count < (hash->num_buckets - 1) &&
           !bitvector_next_set_bit(hash->vector, next, &next)) {
      _sparse_hash_swap(hash, count, count + 1);
      count++;
    }

    hash->num_buckets -= 1;
  } else {
    next = _sparse_bucket_size(hash->buckets[count]);
    for (i = 0; i < next; ++i) {
      if (hash->buckets[count][i].key == key) {
        /* swap out the just removed key with the last element */
        _sparse_hash_elt_swap(hash->buckets[count], i, next);
        break;
      }
    }

    /* we simply decrease the size of the bucket here; if
     * you have large buckets and delete a lot, it might be
     * worthwhile to realloc */
    _sparse_bucket_size_set(hash->buckets[count], size - 1);
  }

  hash->num_items -= 1;
  r = 0;
out:
#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_release(&hash->lock);
#endif

  return r;
}

int sparse_hash_unref(SparseHash *hash) {
  size_t i;
  assert(hash);

#ifdef SPARSE_HASH_SYNCHRONIZED
  lock_unref(&hash->lock);
#endif

  for (i = 0; i < hash->num_buckets; ++i)
    free((void *)(hash->buckets[i] - 1));

  free((void *)hash->buckets);

  bitvector_unref(hash->vector);

  free((void *)hash);

  return 0;
}
