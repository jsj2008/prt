#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <src/shared/basic.h>
#include <src/runtime/lock.h>

#ifndef KD_FLOAT
#define KD_FLOAT double
#endif

#define KD_SYNCHRONIZED
#define KD_POSITION KD_FLOAT *

typedef struct _KdHyperRect {
  int dimension;
  KD_POSITION min;
  KD_POSITION max;
} KdHyperRect;

typedef struct _KdNode {
  KD_POSITION position;
  int direction;
  void *data;

  struct _KdNode *left, *right;
} KdNode;

typedef struct _KdResultNode {
  KdNode *node;
  KD_FLOAT distance_squared;
  struct _KdResultNode *next;
} KdResultNode;

typedef struct _KdTree {
  int dimension;
  KdNode *root;
  KdHyperRect *rectangle;
#ifdef KD_SYNCHRONIZED
  Lock lock;
#endif
} KdTree;

typedef struct _KdIterator {
  KdTree *tree;
  KdResultNode *list;
  KdResultNode *next;
  size_t size;
} KdIterator;

int kd_tree_new(int, KdTree **);
int kd_tree_clear(KdTree *);
int kd_tree_unref(KdTree *);

int kd_tree_insert(KdTree *, const double *, const void *);
int kd_tree_insertf(KdTree *, const float *, const void *);
int kd_tree_insert3(KdTree *, double, double, double, const void *);
int kd_tree_insert3f(KdTree *, float, float, float, const void *);

int kd_tree_nearest(KdTree *, const double *, KdIterator **);
int kd_tree_nearestf(KdTree *, const float *, KdIterator **);
int kd_tree_nearest3(KdTree *, double, double, double, KdIterator **);
int kd_tree_nearest3f(KdTree *, float, float, float, KdIterator **);

int kd_tree_nearest_range(KdTree *, const double *, double, KdIterator **);
int kd_tree_nearest_rangef(KdTree *, const float *, float, KdIterator **);
int kd_tree_nearest_range3(KdTree *, double, double, double, double,
                           KdIterator **);
int kd_tree_nearest_range3f(KdTree *, float, float, float, float,
                            KdIterator **);

int kd_iterator_free(KdIterator *);
int kd_iterator_rewind(KdIterator *);
bool kd_iterator_last(KdIterator *);
bool kd_iterator_next(KdIterator *);

int kd_iterator_item(KdIterator *, double *, void **);
int kd_iterator_itemf(KdIterator *, float *, void **);
int kd_iterator_data(KdIterator *, void **);
#ifdef __cplusplus
}
#endif
