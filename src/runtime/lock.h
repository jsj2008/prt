#pragma once

#include <src/shared/basic.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void time_add(struct timespec *a, struct timespec *b, struct timespec *result);
void time_from_ms(struct timespec *ts, uint64_t msec);

typedef sem_t WaitHandle;
typedef pthread_mutex_t Lock;

void lock_init(Lock *p);
void lock_init_normal(Lock *p);
void lock_acquire(Lock *p);
void lock_release(Lock *p);
void lock_unref(Lock *p);
int lock_acquire_timed(Lock *p, uint64_t msecs);

#define waithandle_init(wh)                                                    \
  ({                                                                           \
    assert(wh);                                                                \
    sem_init(wh, 0, 0);                                                        \
  })
#define waithandle_signal(wh)                                                  \
  ({                                                                           \
    assert(wh);                                                                \
    sem_post(wh);                                                              \
  })
#define waithandle_unref(wh)                                                   \
  ({                                                                           \
    assert(wh);                                                                \
    sem_destroy(wh);                                                           \
  })
void waithandle_wait(WaitHandle *wh);
int waithandle_wait_timed(WaitHandle *wh, uint64_t msec);

#ifdef __cplusplus
}
#endif
