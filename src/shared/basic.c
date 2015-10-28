#include <src/shared/basic.h>

uint32_t primes[] = {
    31, 127, 709, 4001, 8191, 17851, 38867, 84011, 131071, 524287,
};

uint32_t nearest_prime(uint32_t x) {
  size_t i;
  for (i = 0; i < COUNT(primes); ++i)
    if (primes[i] > x)
      return primes[i];
  return 0;
}

void _Log(LOG_LEVEL ll, const char *msg, ...) {
  if (LEVEL >= ll) {
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    puts("");
    va_end(args);
  }
}

void *reallocarray(void *optr, size_t nmemb, size_t size) {
  if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) && nmemb > 0 &&
      SIZE_MAX / nmemb < size) {
    errno = ENOMEM;
    return NULL;
  }

  return realloc(optr, size * nmemb);
}

int unhexchar(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return -EINVAL;
}

void *memdup(void *mem, size_t size) {
  void *p;

  assert(mem);
  assert(size);

  p = malloc(size);
  if (!p)
    return NULL;
  (void)memcpy(p, mem, size);

  return p;
}

char *str_strip_left(const char *str) {
  const char *p = str;

  while (p && !isalnum(*p))
    p++;

  return (char *)p;
}

void strv_free(void *strv) {
  void **p = (void **)strv;

  while (*p) {
    free(*p);
    p++;
  }

  free(strv);
}

size_t str_char_count(const char *s, int c) {
  const char *p;
  size_t count = 0;

  for (p = s; *p; p++)
    if (*p == c)
      count++;

  return count;
}

bool str_starts_with(const char *s, const char *w) {
  size_t wlen, slen;

  assert(s);
  assert(w);

  wlen = strlen(w);
  slen = strlen(s);

  if (wlen > slen)
    return false;

  return strncmp(s, w, wlen) == 0;
}

char *str_to_lower(const char *s) {
  char *x, *t, *p;

  assert(s);

  x = p = strdup(s);
  t = (char *)s;

  while (*x)
    *x++ = tolower(*t++);

  return p;
}

uint64_t murmur3_64(const char *key, size_t len, uint64_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  const uint64_t *data = (const uint64_t *)key;
  const uint64_t *end = data + (len / 8);

  while (data != end) {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char *data2 = (const unsigned char *)data;

  switch (len & 7) {
  case 7:
    h ^= (uint64_t)(data2[6]) << 48;
  case 6:
    h ^= (uint64_t)(data2[5]) << 40;
  case 5:
    h ^= (uint64_t)(data2[4]) << 32;
  case 4:
    h ^= (uint64_t)(data2[3]) << 24;
  case 3:
    h ^= (uint64_t)(data2[2]) << 16;
  case 2:
    h ^= (uint64_t)(data2[1]) << 8;
  case 1:
    h ^= (uint64_t)(data2[0]);
    h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed) {
  static const uint32_t c1 = 0xcc9e2d51;
  static const uint32_t c2 = 0x1b873593;
  static const uint32_t r1 = 15;
  static const uint32_t r2 = 13;
  static const uint32_t m = 5;
  static const uint32_t n = 0xe6546b64;

  uint32_t hash = seed;

  const int nblocks = len / 4;
  const uint32_t *blocks = (const uint32_t *)key;
  int i;
  for (i = 0; i < nblocks; i++) {
    uint32_t k = blocks[i];
    k *= c1;
    k = (k << r1) | (k >> (32 - r1));
    k *= c2;

    hash ^= k;
    hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
  }

  const uint8_t *tail = (const uint8_t *)(key + nblocks * 4);
  uint32_t k1 = 0;

  switch (len & 3) {
  case 3:
    k1 ^= tail[2] << 16;
  case 2:
    k1 ^= tail[1] << 8;
  case 1:
    k1 ^= tail[0];

    k1 *= c1;
    k1 = (k1 << r1) | (k1 >> (32 - r1));
    k1 *= c2;
    hash ^= k1;
  }

  hash ^= len;
  hash ^= (hash >> 16);
  hash *= 0x85ebca6b;
  hash ^= (hash >> 13);
  hash *= 0xc2b2ae35;
  hash ^= (hash >> 16);

  return hash;
}

inline bool utf16_is_surrogate(uint16_t c) {
  return (0xd800 <= c && c <= 0xdfff);
}

inline bool utf16_is_trailing_surrogate(uint16_t c) {
  return (0xdc00 <= c && c <= 0xdfff);
}

inline uint32_t utf16_surrogate_pair_to_unichar(uint16_t lead, uint16_t trail) {
  return ((lead - 0xd800) << 10) + (trail - 0xdc00) + 0x10000;
}

static int utf8_encoded_expected_len(const char *str) {
  unsigned char c;

  assert(str);

  c = (unsigned char)str[0];
  if (c < 0x80)
    return 1;
  if ((c & 0xe0) == 0xc0)
    return 2;
  if ((c & 0xf0) == 0xe0)
    return 3;
  if ((c & 0xf8) == 0xf0)
    return 4;
  if ((c & 0xfc) == 0xf8)
    return 5;
  if ((c & 0xfe) == 0xfc)
    return 6;

  return 0;
}

bool unichar_is_valid(uint32_t ch) {

  if (ch >= 0x110000) /* End of unicode space */
    return false;
  if ((ch & 0xFFFFF800) == 0xD800) /* Reserved area for UTF-16 */
    return false;
  if ((ch >= 0xFDD0) && (ch <= 0xFDEF)) /* Reserved */
    return false;
  if ((ch & 0xFFFE) == 0xFFFE) /* BOM (Byte Order Mark) */
    return false;

  return true;
}

int utf8_encoded_to_unichar(const char *str) {
  int unichar, len, i;

  assert(str);

  len = utf8_encoded_expected_len(str);

  switch (len) {
  case 1:
    return (int)str[0];
  case 2:
    unichar = str[0] & 0x1f;
    break;
  case 3:
    unichar = (int)str[0] & 0x0f;
    break;
  case 4:
    unichar = (int)str[0] & 0x07;
    break;
  case 5:
    unichar = (int)str[0] & 0x03;
    break;
  case 6:
    unichar = (int)str[0] & 0x01;
    break;
  default:
    return -EINVAL;
  }

  for (i = 1; i < len; i++) {
    if (((int)str[i] & 0xc0) != 0x80)
      return -EINVAL;
    unichar <<= 6;
    unichar |= (int)str[i] & 0x3f;
  }

  return unichar;
}

static int utf8_unichar_to_encoded_len(int unichar) {

  if (unichar < 0x80)
    return 1;
  if (unichar < 0x800)
    return 2;
  if (unichar < 0x10000)
    return 3;
  if (unichar < 0x200000)
    return 4;
  if (unichar < 0x4000000)
    return 5;

  return 6;
}

int utf8_encoded_valid_unichar(const char *str) {
  int len, unichar, i;

  assert(str);

  len = utf8_encoded_expected_len(str);
  if (len == 0)
    return -EINVAL;

  /* ascii is valid */
  if (len == 1)
    return 1;

  /* check if expected encoded chars are available */
  for (i = 0; i < len; i++)
    if ((str[i] & 0x80) != 0x80)
      return -EINVAL;

  unichar = utf8_encoded_to_unichar(str);

  /* check if encoded length matches encoded value */
  if (utf8_unichar_to_encoded_len(unichar) != len)
    return -EINVAL;

  /* check if value has valid range */
  if (!unichar_is_valid(unichar))
    return -EINVAL;

  return len;
}

size_t utf8_encode_unichar(char *out_utf8, uint32_t g) {
  if (g < (1 << 7)) {
    if (out_utf8)
      out_utf8[0] = g & 0x7f;
    return 1;
  } else if (g < (1 << 11)) {
    if (out_utf8) {
      out_utf8[0] = 0xc0 | ((g >> 6) & 0x1f);
      out_utf8[1] = 0x80 | (g & 0x3f);
    }
    return 2;
  } else if (g < (1 << 16)) {
    if (out_utf8) {
      out_utf8[0] = 0xe0 | ((g >> 12) & 0x0f);
      out_utf8[1] = 0x80 | ((g >> 6) & 0x3f);
      out_utf8[2] = 0x80 | (g & 0x3f);
    }
    return 3;
  } else if (g < (1 << 21)) {
    if (out_utf8) {
      out_utf8[0] = 0xf0 | ((g >> 18) & 0x07);
      out_utf8[1] = 0x80 | ((g >> 12) & 0x3f);
      out_utf8[2] = 0x80 | ((g >> 6) & 0x3f);
      out_utf8[3] = 0x80 | (g & 0x3f);
    }
    return 4;
  }

  return 0;
}
