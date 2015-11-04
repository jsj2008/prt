#include <prt/shared/popcnt.h>

#ifdef PRT_INTEL
uint64_t popcnt64_fast(uint64_t *p, size_t len) {
  size_t i;
  uint64_t cnt[4] = {0}, c, mask;

  c = 0;
  mask = len & ~3;

  /* 4x unrolled loop */
  for (i = 0; i < mask; i += 4)
    __asm__("popcnt %4, %4  \n\t"
            "add %4, %0     \n\t"
            "popcnt %5, %5  \n\t"
            "add %5, %1     \n\t"
            "popcnt %6, %6  \n\t"
            "add %6, %2     \n\t"
            "popcnt %7, %7  \n\t"
            "add %7, %3     \n\t"
            : "+r"(cnt[0]), "+r"(cnt[1]), "+r"(cnt[2]), "+r"(cnt[3])
            : "r"(p[i]), "r"(p[i + 1]), "r"(p[i + 2]), "r"(p[i + 3]));

  /* add the remaining items (max 3) */
  for (i = 0; i < (len & 3); ++i)
    __asm__("popcnt %1, %1  \n\t"
            "add %1, %0     \n\t"
            : "+r"(c)
            : "r"(p[mask + i]));

  return cnt[0] + cnt[1] + cnt[2] + cnt[3] + c;
}
#endif

#ifdef PRT_ARM
#ifdef PRT_ARCH64
uint64_t bsf64_fast(uint64_t *p, size_t len, size_t offset) {
  unsigned long long *d = p;
  unsigned int masked = 0, i = 0;
  int c = 0;
  assert(offset < 64);

  masked = len & ~3;
  for (; i < masked; i += 4) {
    __asm__("LD1 {v0.2D}, [%1], #16           \n\t"
            "CLZ v0.16b, v0.16b               \n\t"
            "UMOV x0, v0.d[0]                 \n\t"
            "UMOV x1, v0.d[1]                 \n\t"
            "CMP  x0, #0                      \n\t"
            "B.EQ out                         \n\t"
            ""
            "out:                             \n\t"
            : "+r"(c), "+r"(d), "r"(offset)::"x0", "v0", "v1", "v2", "v3");
  }

/*
  for (; i < len; ++i)
    __asm__("LD1  {v0.D}[0], [%1], #8 \n\t"
            "CNT  v0.8b, v0.8b        \n\t"
            "UADDLV h1, v0.8b         \n\t"
            "UMOV x0, v1.d[0]         \n\t"
            "ADD %0, x0, %0           \n\t"
            : "+r"(c), "+r"(d)::"x0", "v0", "v1");
            */
out:
  return c;
}

uint64_t popcnt64_fast(uint64_t *p, size_t len) {
  unsigned long long *d = p;
  unsigned int masked = 0, i = 0;
  int c = 0;

  masked = len & ~3;
  for (; i < masked; i += 4)
    __asm__("LD1 {v0.2D, v1.2D}, [%1], #32    \n\t"
            "CNT v0.16b, v0.16b               \n\t"
            "CNT v1.16b, v1.16b               \n\t"
            "UADDLV h2, v0.16b                \n\t"
            "UADDLV h3, v1.16b                \n\t"
            "ADD d2, d3, d2                   \n\t"
            "UMOV x0, v2.d[0]                 \n\t"
            "ADD %0, x0, %0                   \n\t"
            : "+r"(c), "+r"(d)::"x0", "v0", "v1", "v2", "v3");

  for (; i < len; ++i)
    __asm__("LD1  {v0.D}[0], [%1], #8 \n\t"
            "CNT  v0.8b, v0.8b        \n\t"
            "UADDLV h1, v0.8b         \n\t"
            "UMOV x0, v1.d[0]         \n\t"
            "ADD %0, x0, %0           \n\t"
            : "+r"(c), "+r"(d)::"x0", "v0", "v1");

  return c;
}

#else

uint64_t popcnt64_fast(uint64_t *p, size_t len) {
  size_t i;
  uint64_t c, mask;

  c = 0;
  mask = len & ~3;

  /* 4x unrolled loop */
  for (i = 0; i < mask; i += 4)
    __asm__("vcnt %1, %1  \n\t"
            "add %1, %0   \n\t"
            "vcnt %2, %2  \n\t"
            "add %2, %0   \n\t"
            "vcnt %3, %3  \n\t"
            "add %3, %0   \n\t"
            "vcnt %4, %4  \n\t"
            "add %4, %0   \n\t"
            : "+r"(c)
            : "r"(p[i]), "r"(p[i + 1]), "r"(p[i + 2]), "r"(p[i + 3]));

  /* add the remaining items (max 3) */
  for (i = 0; i < (len & 3); ++i)
    __asm__("vcnt %1, %1  \n\t"
            "add %1, %0   \n\t"
            : "+r"(c)
            : "r"(p[mask + i]));

  return c;
}

#endif
#endif
