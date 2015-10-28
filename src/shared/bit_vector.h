#pragma once

#include <src/shared/basic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { BV_MAX_SHIFT = sizeof(uint64_t) * 8, BV_INT_MAX = (uint64_t)-1 };

/*
 * BitVector
 *  Implements a one-dimensional bit vector which automatically
 *  expands as new items are inserted. Bits are stored in the
 *  multiples of 64, allowing us to swap division and modulo
 *  operations with much cheaper bit shift right and binary AND,
 *  respectively.
 */

typedef struct _BitVector {
  uint64_t *items;
  size_t num_bits;
  size_t num_items;
} BitVector;

int bitvector_new(size_t, BitVector **);
int bitvector_set_bit(BitVector *, size_t, bool);
int bitvector_set_bits(BitVector *, size_t, size_t, bool);
int bitvector_mask_bits(BitVector *, size_t, uint64_t *, size_t);
int bitvector_get_bit(BitVector *, size_t, bool *);
int bitvector_next_set_bit(BitVector *, size_t, size_t *);
int bitvector_count_bits(BitVector *, size_t, size_t *);
int bitvector_unref(BitVector *);

#ifdef __cplusplus
}
#endif
