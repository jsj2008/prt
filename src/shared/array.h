#pragma once

#include <src/shared/basic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Array {
  uint8_t *items;
  size_t occupied;
  size_t capacity;
} Array;

typedef int (*ArSort)(const void *, const void *);

#define ARRAY_GET(a, i, t) (((t *)((a)->items)) + (i))
/* DOESNT WORK
#define ARRAY_ADD(a, i)                                                        \
  ({                                                                           \
    if (!array_add(a, (const char *)&i, sizeof(i)))                            \
      (void *)((a)->items + ((a)->occupied - sizeof(i)));                      \
    else                                                                       \
      (void *)NULL;                                                            \
  })
*/
#define ARRAY_ADD(a, i) array_add(a, (const char *)&i, sizeof(i))

int array_new(Array **out_array);
int array_add(Array *array, const char *p, size_t size);
int array_remove(Array *array, size_t start, size_t num);
int array_num_items(Array *array, size_t item_size, size_t *out_num);
int array_sort(Array *array, ArSort comparer, size_t element_size);
int array_unref(Array *array);

#ifdef __cplusplus
}
#endif
