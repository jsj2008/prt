#pragma once

#include <prt/shared/basic.h>
#include <prt/shared/hashtable.h>
#include <prt/runtime/resources.h>
#include <prt/runtime/lock.h>
#include <prt/shared/list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RmFreeItem)(FileResource *);
typedef void (*RmLoadCallback)(FileResource *, void *);

struct LoadThread {
  struct list_head list;
  pthread_t thread;
};

typedef struct _ResourceManager {
  char *work_dir;
  Hashtable *resources;
  Lock loads_lock;
  struct list_head loads;
  size_t num_loads;
  RmFreeItem destructor;
} ResourceManager;

int resource_manager_new(const char *workdir, ResourceManager **out_manager);
int resource_manager_add(ResourceManager *manager, FileResource *fr);
int resource_manager_load(ResourceManager *manager, const char *path,
                          FileResource **out_fr);
int resource_manager_load_async(ResourceManager *manager, const char *path,
                                RmLoadCallback cb, void *cbcontext,
                                WaitHandle *waithandle);
int resource_manager_remove(ResourceManager *manager, FileResource *fr);
int resource_manager_unref(ResourceManager *manager);

#ifdef __cplusplus
}
#endif
