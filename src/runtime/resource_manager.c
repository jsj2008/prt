#include <src/runtime/resource_manager.h>

struct LoadArgs {
  ResourceManager *rm;
  const char *path;
  RmLoadCallback callback;
  WaitHandle *waithandle;
  struct LoadThread *thread;
  void *cbcontext;
};

/* @func `_free_file_resource`
 * @desc Releases file resource object
 *
 * @param(fr) File resource
 *
 * @ret nothing
 */
void _free_file_resource(FileResource *fr) { (void)file_resource_unref(fr); }

/* @func `_resource_loader`
 * @desc Used to asynchronously load resource
 *
 * @param(load_args) Load arguments
 *
 * @ret `NULL`
 */
void *_resource_loader(void *load_args) {
  struct LoadArgs *args;
  FileResource *fr;
  RmLoadCallback loadcb;
  void *loadcb_context;
  WaitHandle *handle;
  const char *path;
  int r;

  args = (struct LoadArgs *)load_args;
  path = strjoina(args->rm->work_dir, "/", args->path);
  /* search cache first */
  r = hashtable_find(args->rm->resources, HASH(path), (void *)path,
                     (void **)&fr);
  if (r == 0)
    goto out;

  r = file_resource_new(path, &fr);
  if (r < 0) {
    Log("async_resource_loader error: %i\n", r);
    fr = NULL;
    goto out;
  }

  r = hashtable_add_str(args->rm->resources, path, (void *)fr);
  if (r < 0) {
    file_resource_unref(fr);
    Log("async_resource_loader add error: %i\n", r);
    fr = NULL;
  }

out:
  /* lock, remove our thread from job list, release memory, unlock */
  lock_acquire(&args->rm->loads_lock);
  list_del(&args->thread->list);
  free((void *)args->thread);
  lock_release(&args->rm->loads_lock);
  /* unlocked */

  __sync_fetch_and_sub(&args->rm->num_loads, 1);

  /* save callback + context and waithandle so we can delete `args`
   * in case the callback crashes we'll not leak any memory, but
   * handle will be left _unsignalled_ */
  loadcb = args->callback;
  loadcb_context = args->cbcontext;
  handle = args->waithandle;

  free((void *)args);

  /* invoke callback if present */
  if (loadcb)
    loadcb(fr, loadcb_context);

  /* we're done, signal the waiter */
  if (handle)
    waithandle_signal(handle);

  return NULL;
}

/* @func `_insert_thread`
 * @desc Creates a new workload based on pthreads
 *
 * @param(manager)    Resource manager object
 * @param(path)       Relative path to the resource
 * @param(cb)         Optional callback executed when loading finishes
 * @param(cbcontext)  Optional callback context to be passed
 * @param(waithandle) Optional pointer to a `WaitHandle` object
 *
 * @ret 0 on success or error code
 */
int _insert_thread(ResourceManager *manager, const char *path,
                   RmLoadCallback cb, void *cbcontext, WaitHandle *waithandle) {
  struct LoadArgs *args;
  struct LoadThread *thread;
  int r;

  thread = NEW0(struct LoadThread);
  if (!thread)
    return -ENOMEM;

  args = NEW0(struct LoadArgs);
  if (!args) {
    free((void *)thread);
    return -ENOMEM;
  }

  args->callback = cb;
  args->path = path;
  args->rm = manager;
  args->waithandle = waithandle;
  args->cbcontext = cbcontext;
  args->thread = thread;

  /* lock `manager` and insert a new thread into the list */
  lock_acquire(&manager->loads_lock);
  list_add_tail(&thread->list, &manager->loads);
  lock_release(&manager->loads_lock);
  /* unlocked */

  r = pthread_create(&thread->thread, NULL, _resource_loader, args);
  if (r < 0) {
    lock_acquire(&manager->loads_lock);
    list_del(&thread->list);
    lock_release(&manager->loads_lock);

    free((void *)args);
    free((void *)thread);
    return r;
  }

  __sync_fetch_and_add(&manager->num_loads, 1);

  return 0;
}

/* @func `resource_manager_new`
 * @desc Creates a new resource manager object
 *
 * @param(workdir)     Absolute working directory path
 * @param(out_manager) Receives the created object
 *
 * @ret 0 on success or error code
 */
int resource_manager_new(const char *workdir, ResourceManager **out_manager) {
  ResourceManager *manager;
  int r;
  assert(workdir);

  manager = NEW0(ResourceManager);
  if (!manager)
    return -ENOMEM;

  manager->work_dir = strdup(workdir);
  if (!manager->work_dir) {
    free((void *)manager);
    return -ENOMEM;
  }

  r = hashtable_new(127, &manager->resources);
  if (r < 0) {
    free((void *)manager->work_dir);
    free((void *)manager);
    return r;
  }

  INIT_LIST_HEAD(&manager->loads);
  manager->destructor = _free_file_resource;
  *out_manager = manager;

  return 0;
}

/* add fails on duplicate insert */
int resource_manager_add(ResourceManager *manager, FileResource *fr) {
  assert(manager);
  assert(fr);

  return hashtable_add_str(manager->resources, fr->path, fr);
}

/* either loads from path or returns a cached file resource */
int resource_manager_load(ResourceManager *manager, const char *path,
                          FileResource **out_fr) {
  const char *p;
  FileResource *fr;
  int r;
  assert(manager);
  assert(path);

  p = strjoina(manager->work_dir, "/", path);
  r = hashtable_find(manager->resources, HASH(p), (void *)p, (void **)&fr);
  if (!r) {
    /* cache hit, update `out_fr` if not null */
    if (*out_fr)
      *out_fr = fr;
    return 0;
  }

  r = file_resource_new(p, &fr);
  if (r < 0)
    return r;

  r = resource_manager_add(manager, fr);
  if (r < 0) {
    file_resource_unref(fr);
    return r;
  }

  if (out_fr)
    *out_fr = fr;

  return 0;
}

/* waithandle is signalled after cb is invoked */
int resource_manager_load_async(ResourceManager *manager, const char *path,
                                RmLoadCallback cb, void *cbcontext,
                                WaitHandle *waithandle) {
  assert(manager);
  assert(path);

  /* own the `waithandle`  */
  waithandle_init(waithandle);
  return _insert_thread(manager, path, cb, cbcontext, waithandle);
}

int resource_manager_remove(ResourceManager *manager, FileResource *fr) {
  return hashtable_remove(manager->resources, HASH(fr->path), (void *)fr->path,
                          NULL);
}

int resource_manager_unref(ResourceManager *manager) {
  HtIterator *it;
  void *resource;
  size_t i, loads;
  int r;
  struct list_head *h, *t;
  assert(manager);

  loads = manager->num_loads;

  /* wait for queued jobs to finish */
  list_for_each_safe(h, t, &manager->loads)
      assert(pthread_join(((struct LoadThread *)h)->thread, NULL) == 0);

  /* check that we are race-free */
  assert(manager->num_loads == 0);

  /* if the destructor is set it means we *own* the values
   * and hence should release them */
  if (manager->destructor) {
    (void)hashtable_iterate(manager->resources, &it);
    while (hashtable_iterator_next(it, NULL, &resource)) {
      manager->destructor(resource);
    }
  }

  r = hashtable_unref(manager->resources);
  if (r < 0)
    return r;

  free((void *)manager);

  return 0;
}
