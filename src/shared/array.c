#include <src/shared/array.h>

int array_new(Array **out_array) {
  Array *a;

  a = NEW0(Array);
  if (!a)
    return -ENOMEM;

  a->items = calloc(1, 128);
  if (!a->items) {
    free((void *)a);
    return -ENOMEM;
  }
  a->capacity = 128;
  *out_array = a;

  return 0;
}

int array_add(Array *array, const char *p, size_t size) {
  size_t s;
  assert(array);

  s = ADD_WRAP_SAFE(array->occupied, size);
  if (s >= array->capacity) {
    /* realloc the array */
    s = next_power2_64(s);
    array->items = reallocarray(array->items, s, 1);
    if (!array->items)
      return -ENOMEM;

    array->capacity = s;
  }

  memcpy(array->items + array->occupied, p, size);
  array->occupied += size;

  return 0;
}

int array_remove(Array *array, size_t start, size_t num) {
  size_t rest, next_size, copy;
  void *src, *dst;
  assert(array);
  assert(ADD_WRAP_SAFE(start, num) < array->occupied);

  src = array->items + start;
  dst = array->items + start + num;
  copy = array->occupied - start - num;
  rest = array->occupied - num;
  next_size = next_power2_64(rest);

  memmove(src, dst, copy);
  if (next_size != array->capacity) {
    /* release the unneeded space */
    array->items = reallocarray(array->items, next_size, 1);
    if (!array->items)
      return -ENOMEM;
  }

  return 0;
}

int array_num_items(Array *array, size_t item_size, size_t *out_num) {
  assert(array);
  assert(item_size);
  assert(out_num);
  *out_num = array->occupied / item_size;
  return 0;
}

int array_sort(Array *array, ArSort comparer, size_t element_size) {
  assert(array);
  qsort(array->items, element_size, array->occupied / element_size, comparer);
  return 0;
}

int array_unref(Array *array) {
  assert(array);
  free((void *)array->items);
  free((void *)array);
  return 0;
}
