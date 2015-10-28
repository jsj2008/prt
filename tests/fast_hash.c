#include <tests/common.h>
#include <src/shared/fast_hash.h>

struct Data {
  char *key;
  char *value;
};

struct Data test_data[] = {{"some", "val1"},
                           {"random", "val2"},
                           {"test", "val3"},
                           {"keys", "val4"},
                           {"lets", "add"},
                           {"plenty", "of"},
                           {"other", "key"},
                           {"values", "stuff"}};

int main(int argc, const char *argv[]) {
  FastHashBuilder *fhb;
  FastHash *fh;
  void *val;
  size_t i;
  int r;

  output1("[!] " PRD_HEADER " - fasthash test");

  r = fasthash_builder_new(&fhb);
  output(" [+] fasthash builder created: %s", r == 0 ? "yes" : "no");
  for (i = 0; i < COUNT(test_data); ++i) {
    r = fasthash_builder_add(fhb, test_data[i].key, test_data[i].value);
    if (r < 0) {
      output(" [!] error appending key: (%s) value: (%s)", test_data[i].key,
             test_data[i].value);
      return r;
    }
    output1("  [+] key/value added");
  }
  output1("");

  r = fasthash_build(fhb, &fh);
  output(" [+] fasthash created: %s", r == 0 ? "yes" : "no");

  for (i = 0; i < fh->num_items / 2; ++i)
    output("  [>] item: %s", *((void **)fh->items + i * 2));

  output1("");

  r = fasthash_find(fh, test_data[3].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[0].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[2].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[1].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[4].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[7].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[6].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  r = fasthash_find(fh, test_data[5].key, &val);
  output(" [+] fasthash item found: %s\n  -  %s", r == 0 ? "yes" : "no",
         (const char *)val);
  output1("");
  r = fasthash_builder_unref(fhb);
  output(" [+] fasthash builder removed: %s", r == 0 ? "yes" : "no");
  output1("  - ALL TESTS PASSED!");
  return 0;
}
