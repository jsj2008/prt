#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

/***************************
 Prerequisite Runtime v0.3
***************************/

#ifdef __cplusplus
extern "C" {
#endif

/*#define PRT_ARCH32*/
/*#define PRT_ARM*/

#define PRT_INTEL
#define PRT_ARCH64

#ifdef PRT_ARCH64
typedef uint64_t Id;
#else
typedef uint32_t Id;
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define INT_TO_PTR(i) (void *)((intptr_t)i)
#define PTR_TO_INT(p) (int)((intptr_t)p)
#define UINT_TO_PTR(i) (void *)((uintptr_t)i)
#define PTR_TO_UINT(p) (unsigned int)((uintptr_t)p)
#define LONG_TO_PTR(i) (void *)((intptr_t)i)
#define PTR_TO_LONG(p) (int64_t)((intptr_t)p)
#define ULONG_TO_PTR(i) (void *)((intptr_t)i)
#define PTR_TO_ULONG(p) (uint64_t)((intptr_t)p)

#define ADD_WRAP_SAFE(x, y)                                                    \
  ({                                                                           \
    size_t __d__ = x, __c__ = (size_t)x + (size_t)y;                           \
    assert(__d__ < __c__);                                                     \
    __c__;                                                                     \
  })

#define COUNT(x) (sizeof(x) / sizeof(x[0]))

#define NEW0(t) (t *) calloc(1, sizeof(t));
#define NEW0N(t, N) (t *) calloc(N, sizeof(t));

#define strjoina(x, ...)                                                       \
  ({                                                                           \
    const char *_x_[] = {x, __VA_ARGS__};                                      \
    char *_b_ = NULL, *_d_ = NULL;                                             \
    size_t _c_ = 0, _i_ = 0, _s_ = sizeof(_x_) / sizeof(_x_[0]);               \
    for (; _i_ < _s_ && _x_[_i_]; _i_++)                                       \
      _c_ = ADD_WRAP_SAFE(_c_, strlen(_x_[_i_]));                              \
    _b_ = _d_ = (char *)alloca(_c_);                                           \
    for (_i_ = 0; _i_ < _s_ && _x_[_i_]; _i_++)                                \
      _d_ = stpcpy(_d_, _x_[_i_]);                                             \
    _b_;                                                                       \
  })

#define dump_bits(val)                                                         \
  ({                                                                           \
    typeof(val) _one_ = 1;                                                     \
    printf("%zu: ", sizeof(val) * 8);                                          \
    for (int _i_ = (sizeof(val) * 8) - 1; _i_ >= 0; --_i_)                     \
      putchar((val & (_one_ << _i_)) != 0 ? '1' : '0');                        \
    putchar('\n');                                                             \
  })

/* fast implementation using reverse bit scan */
#define next_power2_64(x)                                                      \
  ({                                                                           \
    uint64_t _x_ = x, _r_ = 1;                                                 \
    if (_x_ && (_x_ << 1) != 0) {                                              \
      int _c_ = __builtin_clzll(_x_);                                          \
      _r_ = 1ull << (64 - _c_);                                                \
    }                                                                          \
    _r_;                                                                       \
  })

#define in_set(c, d, ...)                                                      \
  ({                                                                           \
    const typeof(d) _x_[] = {d, __VA_ARGS__};                                  \
    size_t _s_ = sizeof(_x_) / sizeof(_x_[0]), _i_ = 0;                        \
    bool _f_ = false;                                                          \
    for (; _i_ < _s_; ++_i_)                                                   \
      if (c == _x_[_i_]) {                                                     \
        _f_ = true;                                                            \
        break;                                                                 \
      }                                                                        \
    _f_;                                                                       \
  })

#define streq(a, b) strcmp((a), (b)) == 0

static void freec(void **p) {
  if (p)
    free(*(void **)p);
}
#define __free __attribute__((__cleanup__(freec)))

static void freestr(char **s) {
  if (s)
    free(*(void **)s);
}
#define __free_str __attribute__((__cleanup__(freestr)))

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))

typedef enum _LOG_LEVEL {
  LL_NONE,

  LL_ERROR,
  LL_WARN,
  LL_INFO,

  _LL_MAX = LL_INFO
} LOG_LEVEL;

extern uint32_t primes[];

/* 10 primes from 31 to 524287; this table is used
   for determining next number of buckets in a hashtable */
uint32_t nearest_prime(uint32_t x);

static LOG_LEVEL LEVEL = LL_INFO;

#define Log(a, ...) _Log(LEVEL, (a), __VA_ARGS__)
void _Log(LOG_LEVEL ll, const char *msg, ...);

void *reallocarray(void *optr, size_t nmemb, size_t size);
void *memdup(void *mem, size_t size);

int unhexchar(char c);
char *str_strip_left(const char *str);
size_t str_char_count(const char *s, int c);
bool str_starts_with(const char *s, const char *w);
char *str_to_lower(const char *s);
void strv_free(void *strv);

bool utf16_is_surrogate(uint16_t c);
bool utf16_is_trailing_surrogate(uint16_t c);
int utf8_encoded_valid_unichar(const char *str);
uint32_t utf16_surrogate_pair_to_unichar(uint16_t lead, uint16_t trail);
bool unichar_is_valid(uint32_t ch);
int utf8_encoded_to_unichar(const char *str);
size_t utf8_encode_unichar(char *out_utf8, uint32_t g);

uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed);
uint64_t murmur3_64(const char *key, size_t len, uint64_t seed);

#define MURMUR32_SEED (uint32_t)0xbac0bac0
#define MURMUR64_SEED (uint64_t)0xbac0f00dbac0f00d

#define MURMUR64L(s) murmur3_64(s, strlen(s), MURMUR64_SEED)
#define murmur2_64(a, b) murmur3_64(a, b, MURMUR64_SEED)

#define MURMUR32L(s) murmur3_32(s, strlen(s), MURMUR32_SEED)
#define murmur2_32(a, b) murmur3_32(a, b, MURMUR32_SEED)

#ifdef PRT_ARCH64
#define HASH(s) MURMUR64L(s)
#else
#define HASH(s) MURMUR32L(s)
#endif

#ifdef __cplusplus
}
#endif
