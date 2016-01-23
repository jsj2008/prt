#pragma once

#include <prt/shared/basic.h>
#include <prt/runtime/lock.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BTREE_SYNCHRONIZED

/* Balanced AVL tree  */
typedef struct _BinaryTreeNode {
  void *key;
  void *data;

  struct _BinaryTreeNode *left;
  struct _BinaryTreeNode *right;
  struct _BinaryTreeNode *parent;

  uint32_t height;
} BinaryTreeNode;

typedef int (*BTCompare)(const void *, const void *);

typedef struct _BinaryTree {
  size_t num_items;
  BinaryTreeNode *root;
  BTCompare compare;
#ifdef BTREE_SYNCHRONIZED
  Lock lock;
#endif
} BinaryTree;

typedef struct _BTreeEnum {
  BinaryTree *tree;
  BinaryTreeNode *node;
} BinaryTreeEnum;

int binary_tree_new(BTCompare, BinaryTree **);
int binary_tree_unref(BinaryTree *);

/* all functions below lock/unlock */
int binary_tree_insert(BinaryTree *, void *, void *);
int binary_tree_find(BinaryTree *, void *, void **);
int binary_tree_delete_key(BinaryTree *, void *, void **);
int binary_tree_delete_node(BinaryTree *, BinaryTreeNode *);

/* lock tree */
int binary_tree_enum_new(BinaryTree *, BinaryTreeEnum **);
/* unlock tree */
int binary_tree_enum_unref(BinaryTreeEnum *);

int binary_tree_enum_next(BinaryTreeEnum *, void **);
int binary_tree_enum_previous(BinaryTreeEnum *, void **);
int binary_tree_enum_end(BinaryTreeEnum *, void **);
int binary_tree_enum_reset(BinaryTreeEnum *);
int binary_tree_enum_first(BinaryTreeEnum *, void **);
int binary_tree_enum_last(BinaryTreeEnum *, void **);

int binary_tree_to_array(BinaryTree *, void **, size_t *);

#ifdef __cplusplus
}
#endif
