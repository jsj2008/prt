#include <tests/common.h>
#include <src/shared/sparse_hash.h>

const char *strings[] = {"asd",      "dg",      "asgfasg", "sdgsd",  "sdxgsdg",
                         "sdgsdtjg", "wjtrf",   "fsaf",    "v26t2",  "626ggfd",
                         "dg2646",   "325dgsg", "236fd",   "3265sdf"};

int main(int argc, const char *argv[]) {
  SparseHash *sh;
  void *v;
  size_t i, c;
  int r, x;
  bool b;

  output1("[!] " PRD_HEADER " - sparsehash test");

  r = sparse_hash_new(1024, &sh);
  output(" [+] sparsehash created: %s", r == 0 ? "ok" : "ERROR");
  for (i = 0; i < COUNT(strings); ++i)
    sparse_hash_add(sh, HASH(strings[i]), INT_TO_PTR(i));

  for (i = 0; i < COUNT(strings); ++i) {
    r = sparse_hash_find(sh, HASH(strings[i]), &v);

    if (r == -ENOENT)
      output("  [-] item: (%s) not found!", strings[i]);
    else
      output("  [-] item: (%s) at: (%i)", strings[i], PTR_TO_INT(v));
  }

  output1(" [+] bucket diagram");
  printf("  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv "
         "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n "
         " ");
  for (i = 0; i < sh->capacity; ++i) {
    bitvector_get_bit(sh->vector, i, &b);
    printf("%s", b == 0 ? "·" : (b == 1 ? "•" : "o"));
    if (((i + 1) % 32) == 0)
      printf(" ");
    if (((i + 1) % 128) == 0)
      printf("\n  ");
  }
  printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ "
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

  output(" [-] removing item: (%s)", strings[5]);
  sparse_hash_remove(sh, HASH(strings[5]), NULL);

  for (i = 0; i < COUNT(strings); ++i) {
    r = sparse_hash_find(sh, HASH(strings[i]), &v);

    if (r == -ENOENT)
      output("  [-] item: (%s) not found! [%zu]", strings[i], HASH(strings[i]));
    else
      output("  [-] item: (%s) at: (%i)", strings[i], PTR_TO_INT(v));
  }

  output1(" [+] bucket diagram");
  printf("  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv "
         "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n "
         " ");
  c = 0;
  for (i = 0; i < sh->capacity; ++i) {
    bitvector_get_bit(sh->vector, i, &b);
    if (b) {
      x = PTR_TO_INT((sh->buckets[c] - 1)->value);
      c++;
      printf("%s", (x == 1 ? "•" : "⚫"));
    } else
      printf("·");

    if (((i + 1) % 32) == 0)
      printf(" ");
    if (((i + 1) % 128) == 0)
      printf("\n  ");
  }
  printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ "
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

  sparse_hash_unref(sh);

  output1("  - ALL TESTS PASSED! ♨ ");
  return 0;
}
