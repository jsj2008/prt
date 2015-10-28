#pragma once

#include <src/shared/avl_tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/* `FastHashBuilder` inserts keys/values into underlying AVL tree.
 * When `FastHash` is built the AVL tree is serialized into
 * linear array of interleaved keys with values. By serializing
 * the tree in-order we get the midpoint indices of binary search
 * nicely located at the start of the array.
 */

typedef struct _FastHashBuilder { BinaryTree *tree; } FastHashBuilder;

typedef struct _FastHash {
  void *items;
  size_t num_items;
} FastHash;

int fasthash_builder_new(FastHashBuilder **out_builder);
int fasthash_builder_add(FastHashBuilder *builder, void *key, void *value);
int fasthash_builder_insert(FastHashBuilder *builder, void *keys, void *values,
                            size_t n);
int fasthash_builder_unref(FastHashBuilder *builder);

int fasthash_build(FastHashBuilder *builder, FastHash **out_hash);
int fasthash_find(FastHash *hash, void *key, void **out_value);
int fasthash_unref(FastHash *hash);

#ifdef __cplusplus
}
#endif
