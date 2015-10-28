#include <tests/common.h>
#include <prt/shared/bit_vector.h>

int main(int argc, const char *argv[]) {
  BitVector *vec;
  size_t i, c;
  int r;
  bool v;

  output1("[!] " PRD_HEADER " - bitvector test");

  r = bitvector_new(64, &vec);

  output(" [+] bitvector created: %s", r == 0 ? "ok" : "ERROR");
  if (r < 0)
    return r;

  for (i = 0; i < 1024 * 64; ++i)
    bitvector_set_bit(vec, i, i % 2);

  /*
    printf(" (0, 1) ");
    for (i = 1; i < 2048; ++i) {
      bitvector_count_bits(vec, i, &c);
      printf("%zu ", c);
      if ((i+1) % 2 == 0)
        printf("\n (%zu, %zu) ", i, i+1);
    }
    output1("");

    bitvector_count_bits(vec, 2, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 46, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 47, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 48, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 49, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 50, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1725, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1726, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1727, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1728, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1729, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1730, &c);
    output(" [+] set bits: %zu", c);
    bitvector_count_bits(vec, 1024 * 64, &c);
    output(" [+] set bits: %zu", c);
  */
  for (i = 0; i < 1024; ++i) {
    if (vec->items[i] != 0xaaaaaaaaaaaaaaaa) {
      output(" [!] mismatching bit pattern: 0x%lx != 0xaaaaaaaaaaaaaaaa",
             vec->items[i]);
      return 1;
    }
  }

  for (i = 0; i < 1024 * 64; ++i) {
    bitvector_set_bit(vec, i, i % 4);
  }

  for (i = 0; i < 1024; ++i) {
    if (vec->items[i] != 0xeeeeeeeeeeeeeeee) {
      output(" [!] mismatching bit pattern: 0x%lx != 0xeeeeeeeeeeeeeeee",
             vec->items[i]);
      return 1;
    }
  }

  printf("  > ");
  for (i = 0; i < 1024 * 64; ++i) {
    bitvector_get_bit(vec, i, &v);
    printf("%i", v);
    if (((i + 1) % 32) == 0)
      printf(" ");
    if (((i + 1) % 64) == 0)
      printf("\n  > ");
  }
  printf(" bitvector dump complete!\n");

  output1("  - ALL TESTS PASSED!");
  return 0;
}
