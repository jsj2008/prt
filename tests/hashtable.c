#include <tests/common.h>
#include <src/shared/hashtable.h>

const char *strings[] = {"asd",      "dg",      "asgfasg", "sdgsd",  "sdxgsdg",
                         "sdgsdtjg", "wjtrf",   "fsaf",    "v26t2",  "626ggfd",
                         "dg2646",   "325dgsg", "236fd",   "3265sdf"};

//#define HASH(a) murmur3_32(a, strlen(a), 0xdeadbeef)

int main(int argc, const char *argv[]) {
  Hashtable *h;
  HtIterator *it;
  void *key;
  void *v;
  size_t i, c;
  int r;
  bool b;

  output1("[!] " PRD_HEADER " - hashtable test");

  r = hashtable_new(1024, &h);
  output(" [+] hashtable created: %s", r == 0 ? "ok" : "ERROR");
  for (i = 0; i < COUNT(strings); ++i)
    hashtable_add(h, HASH(strings[i]), (void *)strings[i], INT_TO_PTR(i));

  for (i = 0; i < COUNT(strings); ++i) {
    r = hashtable_find(h, HASH(strings[i]), (void *)strings[i], &v);

    if (r == -ENOENT)
      output("  [-] item: (%s) not found!", strings[i]);
    else
      output("  [-] item: (%s) at: (%i)", strings[i], PTR_TO_INT(v));
  }

  output1(" [+] bucket diagram");
  printf("  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv "
         "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n "
         " ");
  for (i = 0; i < h->num_buckets; ++i) {
    printf("%s", !h->buckets[i].num_items ? "·" : "•");
    if (((i + 1) % 32) == 0)
      printf(" ");
    if (((i + 1) % 128) == 0)
      printf("\n  ");
  }
  printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ "
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

  output(" [-] removing item: (%s)", strings[5]);
  hashtable_remove(h, HASH(strings[5]), (void *)strings[5], NULL);

  for (i = 0; i < COUNT(strings); ++i) {
    r = hashtable_find(h, HASH(strings[i]), (void *)strings[i], &v);

    if (r == -ENOENT)
      output("  [-] item: (%s) not found!", strings[i]);
    else
      output("  [-] item: (%s) at: (%i)", strings[i], PTR_TO_INT(v));
  }

  output1(" [+] bucket diagram");
  printf("  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv "
         "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n "
         " ");
  for (i = 0; i < h->num_buckets; ++i) {
    printf("%s", !h->buckets[i].num_items ? "·" : "•");
    if (((i + 1) % 32) == 0)
      printf(" ");
    if (((i + 1) % 128) == 0)
      printf("\n  ");
  }
  printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ "
         "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");

  output1(" [+] iteration test");
  r = hashtable_iterate(h, &it);
  while (hashtable_iterator_next(it, &key, &v)) {
    output("  [x] iterator item: %s %i", (const char *)key, PTR_TO_INT(v));
  }

  hashtable_iterator_unref(it);

  hashtable_unref(h);

  output1("  - ALL TESTS PASSED!");
  return 0;
}
