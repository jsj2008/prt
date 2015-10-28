#pragma once
#define HASH_SYNCHRONIZED

#include <src/shared/basic.h>
#include <src/shared/bit_vector.h>

#ifdef HASH_SYNCHRONIZED
#include <src/runtime/lock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef Id (*HtHashKey)(void *);
typedef int (*HtCmpKey)(const void *, const void *);

typedef struct _Element {
  void *key;
  void *value;
} Element;

typedef struct _Bucket {
  size_t num_items;
  Element *items;
} Bucket;

typedef struct _Hashtable {
  Bucket *buckets;
  BitVector *vector;
  size_t num_buckets;
  size_t num_items;
  float rehash_factor;
  HtHashKey hash_func;
  HtCmpKey key_cmp;
#ifdef HASH_SYNCHRONIZED
  Lock lock;
#endif
} Hashtable;

typedef struct _HtIterator {
  Hashtable *hashtable;
  size_t offset;
  size_t probe;
  size_t seen;
} HtIterator;

int hashtable_new(size_t, Hashtable **);
int hashtable_add(Hashtable *, Id, void *, void *);
int hashtable_add_str(Hashtable *, const char *, void *);
int hashtable_find(Hashtable *, Id, void *, void **);
int hashtable_remove(Hashtable *, Id, void *, void **);
int hashtable_unref(Hashtable *);

int hashtable_iterate(Hashtable *, HtIterator **);
bool hashtable_iterator_next(HtIterator *, void **, void **);
bool hashtable_iterator_end(HtIterator *);
int hashtable_iterator_unref(HtIterator *);

#ifdef __cplusplus
}
#endif
