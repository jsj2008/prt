#pragma once
#define SPARSE_HASH_SYNCHRONIZED

#include <prt/shared/basic.h>
#include <prt/shared/bit_vector.h>

#ifdef SPARSE_HASH_SYNCHRONIZED
#include <prt/runtime/lock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Sparse hashtable
 *  The idea for now is to lazily allocate buckets on-demand and use the
 *  bucket's position within the bit vector as index into the bucket array.
 *
 *  The downside of this is that we need to re-order all buckets greater
 *  than the just inserted, since their bit index within the vector has
 *  just increased by 1. Resizing/rehashing is currently not implemented,
 *  so prefer larger bucket sizes up-front.
 */

typedef struct _Element {
  Id key;
  void *value;
} Element;

typedef struct _SparseHash {
  Element **buckets;
  BitVector *vector;
  size_t num_buckets;
  size_t num_items;
  size_t capacity;
  float rehash_factor;
#ifdef SPARSE_HASH_SYNCHRONIZED
  Lock lock;
#endif
} SparseHash;

int sparse_hash_new(size_t, SparseHash **);
int sparse_hash_add(SparseHash *, Id, void *);
int sparse_hash_find(SparseHash *, Id, void **);
int sparse_hash_remove(SparseHash *, Id, void **);
int sparse_hash_unref(SparseHash *);

#ifdef __cplusplus
}
#endif
