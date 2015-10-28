#include <prt/shared/avl_tree.h>

int binary_tree_new(BTCompare comparison, BinaryTree **out_tree) {
  BinaryTree *t;
  t = NEW0(BinaryTree);
  if (!t)
    return -ENOMEM;

  t->compare = comparison;
#ifdef BTREE_SYNCHRONIZED
  lock_init(&t->lock);
#endif

  *out_tree = t;

  return 0;
}

static int create_node(BinaryTreeNode *parent, void *key, void *data,
                       BinaryTreeNode **out_node) {
  BinaryTreeNode *n;

  n = NEW0(BinaryTreeNode);
  if (!n)
    return -ENOMEM;

  n->parent = parent;
  n->key = key;
  n->data = data;

  *out_node = n;

  return 0;
}

static int find_insertion_point(BinaryTree *tree, void *key, int *last_compare,
                                BinaryTreeNode **out_point) {
  BinaryTreeNode *c;

  assert(last_compare);
  assert(tree);
  assert(tree->root);

  *last_compare = 0;

  c = tree->root;
  while (c) {
    *last_compare = tree->compare(c->key, key);
    if (*last_compare < 0 && c->left)
      c = c->left;
    else if (*last_compare > 0 && c->right)
      c = c->right;
    else {
      *out_point = c;
      return 0;
    }
  }

  return -EINVAL;
}

static int recalculate_height(BinaryTreeNode *node) {
  assert(node);

  if (node->left && node->right)
    node->height = MAX(node->left->height, node->right->height) + 1;
  else if (node->left)
    node->height = node->left->height + 1;
  else if (node->right)
    node->height = node->right->height + 1;
  else
    node->height = 1;

  return 0;
}

static int rotate_tree_left(BinaryTreeNode *r, BinaryTreeNode **out_rotated) {
  BinaryTreeNode *oldparent, *newtop;

  assert(r);

  oldparent = r->parent;
  newtop = r->right;

  r->right = newtop->left;
  if (r->right)
    r->right->parent = r;

  newtop->left = r;
  r->parent = newtop;

  newtop->parent = oldparent;
  if (oldparent && oldparent->left == r)
    oldparent->left = newtop;
  else if (oldparent)
    oldparent->right = newtop;

  (void)recalculate_height(r);
  (void)recalculate_height(newtop);

  *out_rotated = newtop;

  return 0;
}

static int rotate_tree_right(BinaryTreeNode *r, BinaryTreeNode **out_rotated) {
  BinaryTreeNode *oldparent, *newtop;

  assert(r);

  oldparent = r->parent;
  newtop = r->left;

  r->left = newtop->right;
  if (r->left)
    r->left->parent = r;

  newtop->right = r;
  r->parent = newtop;

  newtop->parent = oldparent;
  if (oldparent && oldparent->left == r)
    oldparent->left = newtop;
  else if (oldparent)
    oldparent->right = newtop;

  (void)recalculate_height(r);
  (void)recalculate_height(newtop);

  *out_rotated = newtop;

  return 0;
}

static int get_node_balance(BinaryTreeNode *node) {
  assert(node);

  if (node->left && node->right)
    return node->right->height - node->left->height;
  else if (node->left)
    return 0 - node->left->height;
  else if (node->right)
    return node->right->height;

  return 0;
}

static int balance_up(BinaryTree *tree, BinaryTreeNode *node) {
  int balance;

  while (true) {
    balance = get_node_balance(node);
    if (balance < -1) {
      if (get_node_balance(node->left) > 0)
        (void)rotate_tree_left(node->left, &node->left);
      (void)rotate_tree_right(node, &node);
    } else if (balance > 1) {
      if (get_node_balance(node->right) < 0)
        (void)rotate_tree_right(node->right, &node->right);
      (void)rotate_tree_left(node, &node);
    }

    recalculate_height(node);
    if (!node->parent) {
      tree->root = node;
      return 0;
    }
    node = node->parent;
  }

  return 0;
}

static int swap_nodes(BinaryTreeNode *a, BinaryTreeNode *b) {
  void *ad, *ak;

  assert(a);
  assert(b);

  ad = a->data;
  ak = a->key;

  a->data = b->data;
  a->key = b->key;
  b->data = ad;
  b->key = ak;

  return 0;
}

static int find_replacement_node(BinaryTreeNode *node,
                                 BinaryTreeNode **out_node) {
  BinaryTreeNode *r;

  assert(node);

  if (node->right) {
    r = node->right;
    while (r->left)
      r = r->left;
  } else if (node->left) {
    r = node->left;
    while (r->right)
      r = r->right;
  } else
    r = node;

  *out_node = r;

  return 0;
}

static int delete_leaf_node(BinaryTree *tree, BinaryTreeNode *node) {
  BinaryTreeNode *parent;

  assert(tree);
  assert(node);

  parent = node->parent;
  if (!parent)
    tree->root = NULL;
  else {
    if (parent->left == node)
      parent->left = NULL;
    else if (parent->right == node)
      parent->right = NULL;

    (void)balance_up(tree, parent);
  }

  free((void *)node);
  tree->num_items--;

  return 0;
}

int binary_tree_insert(BinaryTree *tree, void *key, void *value) {
  BinaryTreeNode *n, *i;
  int comparison, r = 0;

  assert(tree);

#ifdef BTREE_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  /* first node */
  if (!tree->root) {
    r = create_node(NULL, key, value, &n);
    if (r < 0)
      goto out;

    tree->root = n;
    tree->num_items++;

    goto out;
  }

  r = find_insertion_point(tree, key, &comparison, &i);
  if (r < 0) {
    free((void *)n);
    goto out;
  }

  if (comparison < 0) {
    r = create_node(i, key, value, &i->left);
    if (r < 0)
      goto out;
  } else if (comparison > 0) {
    r = create_node(i, key, value, &i->right);
    if (r < 0)
      goto out;
  } else {
    r = -EEXIST;
    goto out;
  }

  tree->num_items++;
  r = balance_up(tree, i);
out:
#ifdef BTREE_SYNCHRONIZED
  lock_release(&tree->lock);
#endif
  return r;
}

int binary_tree_find(BinaryTree *tree, void *key, void **out_data) {
  void *ret;
  BinaryTreeNode *c;
  int comparison, r;

  assert(tree);
  assert(key);

#ifdef BTREE_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  r = find_insertion_point(tree, key, &comparison, &c);
  if (r < 0 || comparison != 0)
    goto out;

  *out_data = c->data;
out:
#ifdef BTREE_SYNCHRONIZED
  lock_release(&tree->lock);
#endif
  return r;
}

int binary_tree_delete_key(BinaryTree *tree, void *key, void **out_data) {
  BinaryTreeNode *swap, *c;
  void *data;
  int comparison, r;

  assert(tree);
  assert(key);

#ifdef BTREE_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  r = find_insertion_point(tree, key, &comparison, &c);
  if (r < 0 || comparison != 0)
    goto out;

  data = c->data;

  (void)find_replacement_node(c, &swap);
  (void)swap_nodes(c, swap);
  c = swap;

  while (c->left || c->right) {
    (void)find_replacement_node(c, &swap);
    (void)swap_nodes(c, swap);
    c = swap;
  }

  (void)delete_leaf_node(tree, c);

  *out_data = data;
out:
#ifdef BTREE_SYNCHRONIZED
  lock_release(&tree->lock);
#endif
  return r;
}

int binary_tree_delete_node(BinaryTree *tree, BinaryTreeNode *node) {
  BinaryTreeNode *swap;

  assert(tree);
  assert(node);

  (void)find_replacement_node(node, &swap);
  (void)swap_nodes(node, swap);
  node = swap;

  while (node->left || node->right) {
    (void)find_replacement_node(node, &swap);
    (void)swap_nodes(node, swap);
    node = swap;
  }

  return delete_leaf_node(tree, node);
}

int binary_tree_unref(BinaryTree *tree) {
  assert(tree);

  while (tree->root)
    binary_tree_delete_node(tree, tree->root);

#ifdef BTREE_SYNCHRONIZED
  lock_unref(&tree->lock);
#endif

  free((void *)tree);

  return 0;
}

static int leftmost(BinaryTree *tree, BinaryTreeNode **out_node) {
  BinaryTreeNode *n;

  assert(tree);

  n = tree->root;
  while (n && n->left)
    n = n->left;
  *out_node = n;
  return 0;
}

static int rightmost(BinaryTree *tree, BinaryTreeNode **out_node) {
  BinaryTreeNode *n;

  assert(tree);

  n = tree->root;
  while (n && n->right)
    n = n->right;
  *out_node = n;
  return 0;
}

static int in_order_next(BinaryTreeNode *node, BinaryTreeNode **out_node) {
  BinaryTreeNode *n;

  assert(node);

  if (node->right) {
    n = node->right;
    while (n->left)
      n = n->left;
    *out_node = n;
    return 0;
  }
  n = node;
  while (n->parent) {
    if (n->parent->left == n) {
      *out_node = n->parent;
      return 0;
    }
    n = n->parent;
  }
  *out_node = NULL;
  return 0;
}

static int in_order_previous(BinaryTreeNode *node, BinaryTreeNode **out_node) {
  BinaryTreeNode *n;

  assert(node);

  if (node->left) {
    n = node->left;
    while (n->right)
      n = n->right;
    *out_node = n;
    return 0;
  }
  n = node;
  while (n->parent) {
    if (n->parent->right == n) {
      *out_node = n->parent;
      return 0;
    }
    n = n->parent;
  }
  *out_node = NULL;
  return 0;
}

/* locks the tree */
int binary_tree_enum_new(BinaryTree *tree, BinaryTreeEnum **out_enum) {
  BinaryTreeEnum *en;

  assert(tree);

  en = NEW0(BinaryTreeEnum);
  if (!en)
    return -ENOMEM;

  en->tree = tree;

#ifdef BTREE_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif
  *out_enum = en;
  return 0;
}

int binary_tree_enum_first(BinaryTreeEnum *enu, void **out_data) {
  BinaryTreeNode *n;

  assert(enu);

  (void)leftmost(enu->tree, &n);
  if (!n)
    *out_data = NULL;
  else
    *out_data = n->data;

  return 0;
}

int binary_tree_enum_last(BinaryTreeEnum *enu, void **out_data) {
  BinaryTreeNode *n;

  assert(enu);

  (void)rightmost(enu->tree, &n);
  if (!n)
    *out_data = NULL;
  else
    *out_data = n->data;

  return 0;
}

int binary_tree_enum_next(BinaryTreeEnum *enu, void **out_data) {
  BinaryTreeNode *n;

  assert(enu);

  if (!enu->node)
    (void)leftmost(enu->tree, &n);
  else
    (void)in_order_next(enu->node, &n);

  if (!n) {
    *out_data = NULL;
    return 0;
  }

  enu->node = n;
  *out_data = n->data;

  return 0;
}

/* same as above but returns both key/values for use when serializing the tree
 */
static int binary_tree_enum_next_kv(BinaryTreeEnum *enu, void **out_key,
                                    void **out_data) {
  BinaryTreeNode *n;

  assert(enu);

  if (!enu->node)
    (void)leftmost(enu->tree, &n);
  else
    (void)in_order_next(enu->node, &n);

  if (!n) {
    *out_data = NULL;
    return 0;
  }

  enu->node = n;

  *out_key = n->key;
  *out_data = n->data;

  return 0;
}

int binary_tree_enum_previous(BinaryTreeEnum *enu, void **out_data) {
  assert(enu);

  (void)in_order_previous(enu->node, &enu->node);
  if (!enu->node) {
    *out_data = NULL;
    return 0;
  }

  *out_data = enu->node->data;
  return 0;
}

int binary_tree_enum_end(BinaryTreeEnum *enu, void **out_data) {
  assert(enu);
  (void)rightmost(enu->tree, &enu->node);
  *out_data = enu->node->data;
  return 0;
}

int binary_tree_enum_reset(BinaryTreeEnum *enu) {
  assert(enu);
  enu->node = NULL;
  return 0;
}

/* unlocks the tree */
int binary_tree_enum_unref(BinaryTreeEnum *enu) {
  assert(enu);
#ifdef BTREE_SYNCHRONIZED
  lock_unref(&enu->tree->lock);
#endif
  free((void *)enu);
  return 0;
}

int binary_tree_to_array(BinaryTree *tree, void **out_array, size_t *out_size) {
  BinaryTreeEnum *en;
  void **items, *key, *value;
  size_t i;
  int r;

  assert(tree);

  r = binary_tree_enum_new(tree, &en);
  if (r < 0)
    return r;

  items = calloc(tree->num_items * 2, sizeof(void *));
  if (!items) {
    (void)binary_tree_enum_unref(en);
    return -ENOMEM;
  }

  for (i = 0; i < tree->num_items; ++i) {
    (void)binary_tree_enum_next_kv(en, &key, &value);

    items[i * 2 + 0] = key;
    items[i * 2 + 1] = value;
  }

  *out_size = tree->num_items * 2;
  *out_array = (void *)items;

  return binary_tree_enum_unref(en);
}
