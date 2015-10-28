#include <prt/shared/bit_vector.h>
#include <prt/shared/popcnt.h>

int bitvector_new(size_t num_bits, BitVector **out_vector) {
  BitVector *bv;
  uint64_t *data;
  size_t items;

  items = 1;
  if (num_bits > 64)
    items = num_bits >> 3;

  bv = NEW0(BitVector);
  if (!bv)
    return -ENOMEM;

  data = (uint64_t *)calloc(items, sizeof(*data));
  if (!data) {
    free((void *)bv);
    return -ENOMEM;
  }

  bv->items = data;
  bv->num_items = items;

  *out_vector = bv;

  return 0;
}

static int _bitvector_realloc(BitVector *vector, size_t offset) {
  assert(vector);

  /* return if there's enough space already */
  if (vector->num_items >= offset)
    return 0;

  /* increase by 64 bits at a time,
   * might be worthwhile to check different
   * strategies (n^2 etc.)
   */
  vector->items = reallocarray(vector->items, sizeof(uint64_t), offset + 1);

  if (!vector->items)
    return -ENOMEM;

  vector->items[offset] = 0;
  vector->num_items = offset;

  return 0;
}

int bitvector_set_bit(BitVector *vector, size_t bit, bool value) {
  size_t offset, shift;
  int r;
  assert(vector);

  offset = bit >> 6;
  shift = bit & 63;

  r = _bitvector_realloc(vector, offset);
  if (r < 0)
    return r;

  if (value)
    vector->items[offset] |= (uint64_t)1 << (uint64_t)shift;
  else
    vector->items[offset] &= ~((uint64_t)1 << (uint64_t)shift);

  return 0;
}

int bitvector_get_bit(BitVector *vector, size_t bit, bool *value) {
  size_t offset, shift;
  assert(vector);
  assert(value);

  offset = bit >> 6;
  shift = bit & 63;

  if (offset >= vector->num_items)
    return -EINVAL;

  *value = vector->items[offset] & ((uint64_t)1 << (uint64_t)shift);

  return 0;
}

static void inline _set_range(uint64_t *data, size_t min, size_t max,
                              bool value) {
  if (value)
    *data |= ((1ull << (max - min)) - 1) << min;
  else
    *data &= ~(((1ull << (max - min)) - 1) << min);
}

int bitvector_set_bits(BitVector *vector, size_t min, size_t max, bool value) {
  size_t offset_min, offset_max, shift_min, shift_max;
  int r;
  assert(vector);
  assert(max > min);

  offset_min = min >> 6;
  shift_min = min & 63;
  offset_max = max >> 6;
  shift_max = max & 63;

  r = _bitvector_realloc(vector, offset_max);
  if (r < 0)
    return r;

  /* set within a single item */
  if (offset_min == offset_max)
    _set_range(&vector->items[offset_min], shift_min, shift_max, value);
  else {
    /* First set what remains at the lowest bit at start offset,
     * then set from first bit to the highest bit at end offset.
     * Intermediary items are assigned in a loop, 64 bits at   a
     * time.
     */
    _set_range(&vector->items[offset_min], shift_min, BV_MAX_SHIFT, value);
    _set_range(&vector->items[offset_max], 0, shift_max, value);
    for (offset_min++; offset_min < (offset_max - 1); offset_min++)
      vector->items[offset_min] = value ? BV_INT_MAX : 0;
  }

  return 0;
}

int bitvector_mask_bits(BitVector *vector, size_t start, uint64_t *bits,
                        size_t size) {
  size_t offset_min, offset_max, shift_min, shift_max;
  uint64_t *pb;
  int r;
  assert(vector);
  assert(bits);

  pb = bits + 1;
  offset_min = start >> 6;
  shift_min = start & 63;
  offset_max = (start + size) >> 6;
  shift_max = (start + size) & 63;

  /* mask within single item */
  if (offset_min == offset_max)
    vector->items[offset_min] &= BV_INT_MAX ^ (*bits << shift_min);
  else {
    /* Algorithm is the same as for `bitvector_set_bits`, except
     * that we do a mask operation.
     */
    vector->items[offset_min] &= BV_INT_MAX ^ (*bits << shift_min);
    vector->items[offset_max] &= BV_INT_MAX ^ (*bits << shift_max);
    for (offset_min++; offset_min < (offset_max - 1); offset_min++)
      vector->items[offset_min] &= *pb++;
  }

  return 0;
}

int bitvector_count_bits(BitVector *vector, size_t max, size_t *out_count) {
  size_t offset, shift, count;
  assert(vector);
  assert(out_count);

  count = 0;
  offset = max >> 6;
  shift = max & 63;

  /* count from start until the computed offset */
  if (offset)
    count += popcnt64_fast(vector->items, offset);

  /* finish by adding what remains in the last item */
  count += __builtin_popcountll(vector->items[offset] & ((1ull << shift) - 1));

  *out_count = count;

  return 0;
}

int bitvector_next_set_bit(BitVector *vector, size_t index, size_t *out_index) {
  size_t offset, shift, mask;
  int p, r;
  assert(vector);
  assert(out_index);

  offset = index >> 6;
  shift = index & 63;
  mask = ((1ull << (shift + 1)) - 1);

  /* next bit is in current item */
  if (vector->items[offset] >= mask) {
    /* first erase the lower bits so we hit the first upper bit set */
    p = __builtin_ffsll(vector->items[offset] & (BV_INT_MAX ^ mask));
    *out_index = (offset << 6) + (p - 1);
    return 0;
  }

  /* search the rest of the items */
  for (++offset; offset < vector->num_items; offset++) {
    p = __builtin_ffsll(vector->items[offset]);
    if (p) {
      *out_index = (offset << 6) + (p - 1);
      return 0;
    }
  }

  *out_index = BV_INT_MAX;
  return -ENOENT;
}

int bitvector_unref(BitVector *vector) {
  assert(vector);

  free((void *)vector->items);
  free((void *)vector);

  return 0;
}
