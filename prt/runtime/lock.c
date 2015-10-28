#include <prt/runtime/lock.h>
#include <assert.h>

#define NSEC_PER_SEC 1000000000
void time_add(struct timespec *a, struct timespec *b, struct timespec *result) {
  uint64_t sec = a->tv_sec + b->tv_sec;
  uint64_t nsec = a->tv_nsec + b->tv_nsec;
  assert(a);
  assert(b);
  assert(result);

  if (nsec >= NSEC_PER_SEC) {
    nsec -= NSEC_PER_SEC;
    sec++;
  }

  result->tv_sec = sec;
  result->tv_nsec = nsec;
}

void time_from_ms(struct timespec *ts, uint64_t msec) {
  assert(ts);

  ts->tv_sec = msec / 1000;
  ts->tv_nsec = (msec - ((msec / 1000) * 1000)) * 1000000;
}

void lock_init(Lock *p) {
  assert(p);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(p, &attr);
}

void lock_init_normal(Lock *p) {
  assert(p);

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
  pthread_mutex_init(p, &attr);
}

void lock_acquire(Lock *p) {
  assert(p);

  pthread_mutex_lock(p);
}

void lock_release(Lock *p) {
  assert(p);

  pthread_mutex_unlock(p);
}

int lock_acquire_timed(Lock *p, uint64_t msecs) {
  struct timespec t, sum, toadd;
  assert(p);

  time_from_ms(&toadd, msecs);
  clock_gettime(CLOCK_REALTIME, &t);
  time_add(&t, &toadd, &sum);
  return pthread_mutex_timedlock(p, &sum);
}

void lock_unref(Lock *p) {
  assert(p);

  pthread_mutex_destroy(p);
}

int waithandle_wait_timed(WaitHandle *wh, uint64_t msec) {
  struct timespec t, sum, toadd;
  int r;
  assert(wh);

  time_from_ms(&toadd, msec);
  clock_gettime(CLOCK_REALTIME, &t);
  time_add(&t, &toadd, &sum);
  r = sem_timedwait(wh, &sum);
  if (!r)
    return sem_post(wh);
  return r;
}

void waithandle_wait(WaitHandle *wh) {
  int r;
  assert(wh);

  r = sem_wait(wh);
  if (!r)
    sem_post(wh);
}
