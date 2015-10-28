#include <src/shared/popcnt.h>

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
uint64_t popcnt64_fast(uint64_t *p, size_t len) {
  size_t i;
  uint64_t c, mask;

  c = 0;
  mask = len & ~3;

  /* 4x unrolled loop */
  for (i = 0; i < mask; i += 4)
    __asm__("cnt %1, %1  \n\t"
            "add %1, %0  \n\t"
            "cnt %2, %2  \n\t"
            "add %2, %0  \n\t"
            "cnt %3, %3  \n\t"
            "add %3, %0  \n\t"
            "cnt %4, %4  \n\t"
            "add %4, %0  \n\t"
            : "+r"(c)
            : "r"(p[i]), "r"(p[i + 1]), "r"(p[i + 2]), "r"(p[i + 3]));

  /* add the remaining items (max 3) */
  for (i = 0; i < (len & 3); ++i)
    __asm__("cnt %1, %1  \n\t"
            "add %1, %0  \n\t"
            : "+r"(c)
            : "r"(p[mask + i]));

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
