#include <assert.h>

#define COUNT(x) (sizeof(x) / sizeof(x[0]))
#define PRD_HEADER " sp3ctral "

#define output1(x)                                                             \
  ({                                                                           \
    printf(x);                                                                 \
    printf("\n");                                                              \
  })
#define output(x, ...)                                                         \
  ({                                                                           \
    printf(x, __VA_ARGS__);                                                    \
    printf("\n");                                                              \
  })
