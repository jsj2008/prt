#include <prt/shared/kd_tree.h>

#define SQUARE(x) ((x) * (x))

static int _clear_tree_recursive(KdNode *node) {
  if (!node)
    return 0;

  _clear_tree_recursive(node->left);
  _clear_tree_recursive(node->right);

  free((void *)node->position);
  free((void *)node);

  return 0;
}

static int _clear_results(KdResultNode *r) {
  KdResultNode *a, *b;
  a = r;
  while (a) {
    b = a->next;
    free((void *)a);
    a = b;
  }
  return 0;
}

static int _insert_record(KdNode **node, const double *p, void *data, int dir,
                          int dim) {
  KdNode *n;
  int nd;
  assert(p);
  assert(node);

  if (!*node) {
    n = NEW0(KdNode);
    if (!n)
      return -ENOMEM;

    n->position = (KD_POSITION)calloc(dim, sizeof(*n->position));
    if (!n->position) {
      free((void *)n);
      return -ENOMEM;
    }

    memcpy(n->position, p, dim * sizeof(*n->position));
    n->data = data;
    n->direction = dir;
    *node = n;

    return 0;
  }

  n = *node;
  nd = (n->direction + 1) % dim;

  if (p[nd] > n->position[nd])
    return _insert_record(&(*node)->left, p, data, nd, dim);
  return _insert_record(&(*node)->right, p, data, nd, dim);
}

static int _hyper_rectangle_extend(KdHyperRect *r, const double *p) {
  size_t i;
  assert(r);
  assert(p);

  for (i = 0; i < r->dimension; ++i) {
    if (p[i] < r->min[i])
      r->min[i] = p[i];
    if (p[i] > r->max[i])
      r->max[i] = p[i];
  }

  return 0;
}

static int _hyper_rectangle_create(int dimension, const double *min,
                                   const double *max,
                                   KdHyperRect **out_rectangle) {
  KdHyperRect *r;
  assert(min);
  assert(max);
  assert(out_rectangle);

  r = NEW0(KdHyperRect);
  if (!r)
    return -ENOMEM;

  r->dimension = dimension;
  r->max = (KD_POSITION)calloc(dimension, sizeof(double));
  if (!r->max) {
    free((void *)r);
    return -ENOMEM;
  }
  r->min = (KD_POSITION)calloc(dimension, sizeof(double));
  if (!r->min) {
    free((void *)r->max);
    free((void *)r);
    return -ENOMEM;
  }

  memcpy(r->max, max, dimension * sizeof(double));
  memcpy(r->min, min, dimension * sizeof(double));

  *out_rectangle = r;

  return 0;
}

static int _hyper_rectangle_duplicate(KdHyperRect *original,
                                      KdHyperRect **out_new) {
  assert(original);
  return _hyper_rectangle_create(original->dimension, original->min,
                                 original->max, out_new);
}

static int _hyper_rectangle_unref(KdHyperRect *rectangle) {
  assert(rectangle);

  free((void *)rectangle->min);
  free((void *)rectangle->max);
  free((void *)rectangle);
  return 0;
}

static int _hyper_rectangle_distance_sq(KdHyperRect *r, const double *p,
                                        double *out_distance) {
  size_t i;
  double sum = 0.0;
  assert(r);
  assert(p);
  assert(out_distance);

  for (i = 0; i < r->dimension; ++i) {
    if (p[i] < r->min[i])
      sum += SQUARE(r->min[i] - p[i]);
    if (p[i] > r->max[i])
      sum += SQUARE(r->max[i] - p[i]);
  }

  *out_distance = sum;

  return 0;
}

static int _insert_result_node(KdResultNode *rn, KdNode *node,
                               double distance) {
  KdResultNode *r;
  assert(rn);

  r = NEW0(KdResultNode);
  if (!r)
    return -ENOMEM;

  r->node = node;
  r->distance_squared = distance;

  if (distance >= 0.0)
    while (rn->next && rn->next->distance_squared < distance)
      rn = rn->next;

  r->next = rn->next;
  rn->next = r;

  return 0;
}

static int _find_nearest_slicing(KdNode *node, const double *p,
                                 KdNode **out_result, double *out_distance,
                                 KdHyperRect *hyperrect) {
  KdNode *near, *far;
  double *near_rect, *far_rect;
  double dx, distance, rd;
  size_t i;
  int direction, r;
  assert(node);
  assert(p);
  assert(hyperrect);
  assert(out_distance);

  direction = node->direction;
  dx = p[direction] - node->position[direction];
  if (dx <= 0.0) {
    near = node->left;
    far = node->right;
    near_rect = &hyperrect->max[direction];
    far_rect = &hyperrect->min[direction];
  } else {
    near = node->right;
    far = node->left;
    near_rect = &hyperrect->min[direction];
    far_rect = &hyperrect->max[direction];
  }

  if (near) {
    dx = *near_rect;
    *near_rect = node->position[direction];
    r = _find_nearest_slicing(near, p, out_result, out_distance, hyperrect);
    if (r < 0)
      return -1;
    *near_rect = dx;
  }

  distance = 0.0f;
  for (i = 0; i < hyperrect->dimension; ++i)
    distance += SQUARE(node->position[i] - p[i]);

  if (distance < *out_distance) {
    *out_result = node;
    *out_distance = distance;
  }

  if (far) {
    dx = *far_rect;
    *far_rect = node->position[direction];
    (void)_hyper_rectangle_distance_sq(hyperrect, p, &rd);

    if (rd < *out_distance) {
      r = _find_nearest_slicing(far, p, out_result, out_distance, hyperrect);
      if (r < 0)
        return r;
    }

    *far_rect = dx;
  }

  return 0;
}

static int _find_nearest(KdNode *node, const double *p, double range,
                         KdResultNode *rn, int ordered, int dimension,
                         size_t *out_numfound) {
  double distance, dx;
  size_t i, found, f;
  int r;
  assert(rn);
  assert(p);

  if (!node) {
    *out_numfound = 0;
    return 0;
  }

  found = 0;
  r = 0;
  distance = 0.0;
  dx = 0.0;

  for (i = 0; i < dimension; ++i)
    distance += SQUARE(node->position[i] - p[i]);

  if (distance <= SQUARE(range)) {
    r = _insert_result_node(rn, node, ordered ? distance : -1.0);
    if (r < 0)
      return r;
    found = 1;
  }

  dx = p[node->direction] - node->position[node->direction];
  r = _find_nearest(dx <= 0.0 ? node->left : node->right, p, range, rn, ordered,
                    dimension, &f);
  if (r < 0)
    return r;

  if (fabs(dx) < range) {
    found += f;
    r = _find_nearest(dx <= 0.0 ? node->right : node->left, p, range, rn,
                      ordered, dimension, &f);
    if (r < 0)
      return r;
    found += f;
  }

  *out_numfound = found;

  return 0;
}

int kd_tree_new(int dimensions, KdTree **out_tree) {
  KdTree *tree;
  assert(out_tree);

  tree = NEW0(KdTree);
  if (!tree)
    return -ENOMEM;

  tree->dimension = dimensions;
#ifdef KD_SYNCHRONIZED
  lock_init(&tree->lock);
#endif
  *out_tree = tree;

  return 0;
}

int kd_tree_clear(KdTree *tree) {
  assert(tree);
  assert(tree->root);

  return _clear_tree_recursive(tree->root);
}

int kd_tree_unref(KdTree *tree) {
  assert(tree);

  kd_tree_clear(tree);
  if (tree->rectangle)
    _hyper_rectangle_unref(tree->rectangle);

  free((void *)tree);

  return 0;
}

int kd_tree_insert(KdTree *tree, const double *p, const void *data) {
  int r;
  assert(tree);
  assert(p);

#ifdef KD_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  r = _insert_record(&tree->root, p, (void *)data, 0, tree->dimension);
  if (r < 0)
    goto out;

  if (tree->rectangle)
    r = _hyper_rectangle_extend(tree->rectangle, p);
  else
    r = _hyper_rectangle_create(tree->dimension, p, p, &tree->rectangle);

out:
#ifdef KD_SYNCHRONIZED
  lock_release(&tree->lock);
#endif
  return r;
}

int kd_tree_insertf(KdTree *tree, const float *p, const void *data) {
  int r;
  double *d;
  size_t i;
  bool stack;
  assert(tree);
  assert(p);

  stack = (tree->dimension * sizeof(double)) < 4096;

  if (stack)
    d = alloca(tree->dimension * sizeof(double));
  else
    d = calloc(tree->dimension, sizeof(double));

  for (i = 0; i < tree->dimension; ++i)
    d[i] = p[i];

  r = kd_tree_insert(tree, d, data);

  if (!stack)
    free((void *)d);

  return 0;
}

int kd_tree_insert3(KdTree *tree, double x, double y, double z,
                    const void *data) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_insert(tree, d, data);
}

int kd_tree_insert3f(KdTree *tree, float x, float y, float z,
                     const void *data) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_insert(tree, d, data);
}

static int _kd_iterator_free(KdIterator *iterator) {
  KdResultNode *tmp, *node;

  node = iterator->list->next;

  while (node) {
    tmp = node;
    node = node->next;
    free((void *)tmp);
  }

  free((void *)iterator->list);
  free((void *)iterator);

  return 0;
}

int kd_tree_nearest(KdTree *tree, const double *p, KdIterator **out_iterator) {
  KdHyperRect *hr;
  KdNode *n;
  KdIterator *it;
  double distance;
  size_t i;
  int r;
  assert(tree);
  assert(tree->rectangle);
  assert(p);
  assert(out_iterator);

#ifdef KD_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  r = 0;
  it = NEW0(KdIterator);
  if (!it) {
    r = -ENOMEM;
    goto out;
  }

  it->list = NEW0(KdResultNode);
  if (!it->list) {
    free((void *)it);
    r = -ENOMEM;
    goto out;
  }

  it->tree = tree;
  r = _hyper_rectangle_duplicate(tree->rectangle, &hr);
  if (r < 0)
    goto free_out;

  n = tree->root;
  distance = 0.0;
  for (i = 0; i < tree->dimension; ++i)
    distance += SQUARE(n->position[i] - p[i]);

  (void)_find_nearest_slicing(tree->root, p, &n, &distance, hr);

  _hyper_rectangle_unref(hr);
  if (n) {
    r = _insert_result_node(it->list, n, -1.0);
    if (r < 0)
      goto free_out;
    it->size = 1;
    r = kd_iterator_rewind(it);
    if (r < 0)
      goto free_out;
    *out_iterator = it;
  } else {
    _kd_iterator_free(it);
    r = -ENOENT;
  }

  return r;
free_out:
  _kd_iterator_free(it);
out:
#ifdef KD_SYNCHRONIZED
  /* keep locked on success; iterator will unlock */
  if (r < 0)
    lock_release(&tree->lock);
#endif
  return r;
}

int kd_tree_nearestf(KdTree *tree, const float *p, KdIterator **out_iterator) {
  int r;
  double *d;
  size_t i;
  bool stack;
  assert(tree);
  assert(p);
  assert(out_iterator);

  stack = (tree->dimension * sizeof(double)) < 4096;

  if (stack)
    d = alloca(tree->dimension * sizeof(double));
  else
    d = calloc(tree->dimension, sizeof(double));

  for (i = 0; i < tree->dimension; ++i)
    d[i] = p[i];

  r = kd_tree_nearest(tree, d, out_iterator);

  if (!stack)
    free((void *)d);

  return 0;
}

int kd_tree_nearest3(KdTree *tree, double x, double y, double z,
                     KdIterator **out_iterator) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_nearest(tree, d, out_iterator);
}

int kd_tree_nearest3f(KdTree *tree, float x, float y, float z,
                      KdIterator **out_iterator) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_nearest(tree, d, out_iterator);
}

int kd_tree_nearest_range(KdTree *tree, const double *p, double range,
                          KdIterator **out_iterator) {
  KdHyperRect *hr;
  KdNode *n;
  KdIterator *it;
  double distance;
  size_t found;
  int r;
  assert(tree);
  assert(p);
  assert(out_iterator);

#ifdef KD_SYNCHRONIZED
  lock_acquire(&tree->lock);
#endif

  r = 0;
  it = NEW0(KdIterator);
  if (!it) {
    r = -ENOMEM;
    goto out;
  }

  it->list = NEW0(KdResultNode);
  if (!it->list) {
    free((void *)it);
    r = -ENOMEM;
    goto out;
  }

  it->tree = tree;
  r = _find_nearest(tree->root, p, range, it->list, 0, tree->dimension, &found);
  if (r < 0) {
    _kd_iterator_free(it);
    goto out;
  }

  it->size = found;
  *out_iterator = it;

  r = kd_iterator_rewind(it);
out:
#ifdef KD_SYNCHRONIZED
  /* keep locked on success; iterator will unlock */
  if (r < 0)
    lock_release(&tree->lock);
#endif
  return r;
}

int kd_tree_nearest_rangef(KdTree *tree, const float *p, float range,
                           KdIterator **out_iterator) {
  int r;
  double *d;
  size_t i;
  bool stack;
  assert(tree);
  assert(p);
  assert(out_iterator);

  stack = (tree->dimension * sizeof(double)) < 4096;

  if (stack)
    d = alloca(tree->dimension * sizeof(double));
  else
    d = calloc(tree->dimension, sizeof(double));

  for (i = 0; i < tree->dimension; ++i)
    d[i] = p[i];

  r = kd_tree_nearest_range(tree, d, range, out_iterator);

  if (!stack)
    free((void *)d);

  return 0;
}

int kd_tree_nearest_range3f(KdTree *tree, float x, float y, float z,
                            float range, KdIterator **out_iterator) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_nearest_range(tree, d, range, out_iterator);
}

int kd_tree_nearest_range3(KdTree *tree, double x, double y, double z,
                           double range, KdIterator **out_iterator) {
  double d[3];
  d[0] = x;
  d[1] = y;
  d[2] = z;
  return kd_tree_nearest_range(tree, d, range, out_iterator);
}

int kd_iterator_free(KdIterator *iterator) {
  assert(iterator);

#ifdef KD_SYNCHRONIZED
  lock_release(&iterator->tree->lock);
#endif
  return _kd_iterator_free(iterator);
}

int kd_iterator_rewind(KdIterator *iterator) {
  assert(iterator);
  iterator->next = iterator->list->next;
  return 0;
}

bool kd_iterator_end(KdIterator *iterator) {
  assert(iterator);
  return iterator->next == NULL;
}

bool kd_iterator_next(KdIterator *iterator) {
  assert(iterator);
  iterator->next = iterator->next->next;
  return iterator->next != NULL;
}

int kd_iterator_item(KdIterator *iterator, double *p, void **out_data) {
  assert(iterator);

  if (iterator->next) {
    if (p)
      memcpy(p, iterator->next->node->position,
             iterator->tree->dimension * sizeof(*p));
    *out_data = iterator->next->node->data;
    return 0;
  }

  return -ENOENT;
}

int kd_iterator_itemf(KdIterator *iterator, float *p, void **out_data) {
  size_t i;
  assert(iterator);
  assert(out_data);

  if (iterator->next) {
    if (p)
      for (i = 0; i < iterator->tree->dimension; ++i)
        p[i] = iterator->next->node->position[i];
    *out_data = iterator->next->node->data;
    return 0;
  }

  return -ENOENT;
}

int kd_iterator_data(KdIterator *iterator, void **out_data) {
  return kd_iterator_item(iterator, NULL, out_data);
}
