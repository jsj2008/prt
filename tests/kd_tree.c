#include <tests/common.h>
#include <prt/shared/kd_tree.h>

#define AS_STRING(r) (r == 0 ? "OK" : "FAILED")

int main(int argc, const char *argv[]) {
  size_t c;
  int r;
  bool v;
  void *data;
  KdTree *tree;
  KdIterator *iterator;

  output1("[!] " PRD_HEADER " - kd-tree test");

  r = kd_tree_new(3, &tree);
  output(" [~] Tree instantiation: %s", AS_STRING(r));

  r = kd_tree_insert3(tree, 1.0, 1.0, 1.0, ULONG_TO_PTR(132));
  output("  [+] insertion: %s", AS_STRING(r));
  r = kd_tree_insert3(tree, 1.5, 1.5, 1.5, ULONG_TO_PTR(133));
  output("  [+] insertion: %s", AS_STRING(r));
  r = kd_tree_insert3(tree, 1.75, 1.75, 1.75, ULONG_TO_PTR(134));
  output("  [+] insertion: %s", AS_STRING(r));
  r = kd_tree_insert3(tree, 1.8, 1.8, 1.8, ULONG_TO_PTR(135));
  output("  [+] insertion: %s", AS_STRING(r));

  r = kd_tree_nearest3(tree, 1.3, 1.3, 1.3, &iterator);
  output("  [+] nearest search: %s", AS_STRING(r));
  r = kd_iterator_data(iterator, &data);
  output("  [+] iterator get search: %s", AS_STRING(r));
  output("   [>] data item found(133): %zu", PTR_TO_ULONG(data));
  r = kd_iterator_free(iterator);
  output("  [+] iterator free: %s", AS_STRING(r));

  /*
   * We search from (1, 1, 1) with range=1.2991 == range^2=1.6876
   *
   * (1,    1,    1)    - (1, 1, 1) = 0   ^2*3 = 0       |     0 < 1.6876
   * (1.5,  1.5,  1.5)  - (1, 1, 1) = 0.5 ^2*3 = 0.75    |  0.75 < 1.6876
   * (1.75, 1.75, 1.75) - (1, 1, 1) = 0.75^2*3 = 1.6875  |1.6875 < 1.6876
   * (1.8,  1.8,  1.75) - (1, 1, 1) = 0.8 ^2*3 = 1.92    |  1.92 > 1.6876
   */
  r = kd_tree_nearest_range3(tree, 1.0, 1.0, 1.0, 1.2991, &iterator);
  output("  [+] nearest range search: %s", AS_STRING(r));
  output("   [>] found items: %zu", iterator->size);

  do {
    r = kd_iterator_data(iterator, &data);
    output("  [+] iterator get search: %s", AS_STRING(r));
    output("    [v] item: %zu", PTR_TO_ULONG(data));
  } while (kd_iterator_next(iterator));

  r = kd_iterator_free(iterator);
  output("  [+] iterator free: %s", AS_STRING(r));

  return 0;
}
