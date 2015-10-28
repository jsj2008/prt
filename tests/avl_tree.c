#include <tests/common.h>
#include <src/shared/avl_tree.h>

int pointer_compare(void *a, void *b) {
  intptr_t d = (intptr_t)a - (intptr_t)b;
  if (d == 0)
    return 0;
  else if (d < 0)
    return -1;
  return 1;
}

struct Data {
  char *key;
  char *value;
};

struct Data test_data[] = {
    {"some", "val1"}, {"random", "val2"}, {"test", "val3"}, {"keys", "val4"}};

int test_single_entry(BinaryTree *t, size_t index) {
  void *value;
  int r;

  output(
      " [?] attempting to retrieve item with key: (%s), expected value: (%s)",
      test_data[index].key, test_data[index].value);
  r = binary_tree_find(t, test_data[index].key, &value);
  if (r < 0) {
    output1("   [!] NOT FOUND!!");
    return r;
  }

  output("  [~] found value: (%s)", (const char *)value);
  if (streq((const char *)value, test_data[index].value))
    output1("   - CORRECT");
  else {
    output1("   - INCORRECT");
    return -1;
  }

  return 0;
}

int main(int argc, const char *argv[]) {
  BinaryTreeEnum *en;
  BinaryTree *tree;
  void *val, *all;
  int r;
  size_t i, numall;

  output1("[!] " PRD_HEADER " - AVL tree test");

  r = binary_tree_new((BTCompare)pointer_compare, &tree);
  if (r < 0) {
    output1(" [!] tree creation failed!");
    return r;
  }
  output1(" [+] tree created");

  for (i = 0; i < COUNT(test_data); ++i) {
    r = binary_tree_insert(tree, test_data[i].key, test_data[i].value);
    if (r < 0) {
      output("  [!] error inserting into the tree (%s, %s)", test_data[i].key,
             test_data[i].value);
      return r;
    }

    output("  [+] data pair (%zu) inserted", i);
  }
  output1("");

  output1(" [+] attempting duplicate insert");
  r = binary_tree_insert(tree, test_data[1].key, test_data[i].value);
  if (!r) {
    output1("  [!] duplicate insertion succeeded");
    output1("   - ERROR");
    return -1;
  }
  output("  [+] duplicate insert failed with -EEXIST: %s",
         r == -EEXIST ? "yes" : "no");
  output1("");

  for (i = 0; i < COUNT(test_data); ++i) {
    r = test_single_entry(tree, i);
    if (r < 0)
      return r;
  }
  output1("");

  r = binary_tree_to_array(tree, &all, &numall);
  output(" [+] to array: %s [count: %zu]", r == 0 ? "ok" : "ERROR", numall);
  if (r < 0)
    return r;

  for (i = 0; i < numall; ++i) {
    output("  [?] array dump: (%zu): %s", i, *((char **)all + i));
  }

  output1("");

  output(" [!] removing by key: (%s)", test_data[2].key);
  r = binary_tree_delete_key(tree, test_data[2].key, &val);
  if (r < 0) {
    output1(" [!] key removal failed");
    return r;
  }
  output("  [+] key: (%s) with value: (%s) was removed", test_data[2].key,
         (const char *)val);
  output1("");

  r = binary_tree_enum_new(tree, &en);
  if (r < 0) {
    output1(" [!] tree enumerator creation failed!");
    return r;
  }
  output1(" [+] tree enumerator created");

  output(" [+] elements: (%zu)", tree->num_items);

  r = binary_tree_enum_first(en, &val);
  if (r < 0) {
    output1(" [!] tree enumerator couldn't obtain the first element");
    return r;
  }
  output(" [+] tree enumerator first: (%s)", (const char *)val);

  r = binary_tree_enum_last(en, &val);
  if (r < 0) {
    output1(" [!] tree enumerator couldn't obtain the last element");
    return r;
  }
  output(" [+] tree enumerator last: (%s)", (const char *)val);

  i = 0;
  do {
    r = binary_tree_enum_next(en, &val);
    if (r < 0) {
      output1("  [!] enumerator next error");
      return r;
    }
    if (!val)
      break;

    output("  [+] enumerated value at (%zu) is (%s)", i, (const char *)val);
    i++;
  } while (val);

  r = binary_tree_enum_unref(en);
  if (r < 0) {
    output1(" [!] tree enumerator destruction failed");
    return r;
  }
  output1(" [-] tree enumerator destroyed");

  r = binary_tree_unref(tree);
  if (r < 0) {
    output1(" [!] tree destruction failed");
    return r;
  }
  output1(" [-] tree destroyed");
  output1("  - ALL TESTS PASSED!");
  return 0;
}
