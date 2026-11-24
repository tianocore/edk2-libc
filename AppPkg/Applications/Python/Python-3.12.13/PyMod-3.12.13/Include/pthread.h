/** @file
    UEFI Python support: pthread.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#pragma once

#include <stdint.h>

typedef uint64_t pthread_key_t;
typedef uint64_t pthread_mutex_t;
typedef uint64_t pthread_cond_t;
typedef uint64_t pthread_mutexattr_t;
typedef uint64_t pthread_condattr_t;
typedef uint64_t pthread_t;
typedef uint64_t pthread_attr_t;
typedef uint64_t pthread_id_np_t;

extern int pthread_cond_timedwait(pthread_cond_t *cond,
                                  pthread_mutex_t *mutex,
                                  const struct timespec *abstime);   
extern int pthread_mutex_init(pthread_mutex_t *mutex,  
                              const pthread_mutexattr_t *attr);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);   
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);

extern int pthread_cond_init(pthread_cond_t *cond,
                             const pthread_condattr_t *attr);
extern int pthread_cond_destroy(pthread_cond_t *cond);   
extern int pthread_cond_signal(pthread_cond_t *cond);   
extern int pthread_cond_broadcast(pthread_cond_t *cond);   
extern int pthread_cond_timedwait(pthread_cond_t *cond,
                                  pthread_mutex_t *mutex,
                                  const struct timespec *abstime);
extern int pthread_cond_wait(pthread_cond_t *cond,
                             pthread_mutex_t *mutex);


extern int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine)(void *), void *arg);
extern int pthread_detach(pthread_t thread);
extern pthread_t pthread_self(void);
extern void __attribute__((__noreturn__)) pthread_exit(void *status);

extern int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));  
extern int pthread_key_delete(pthread_key_t key);
extern int pthread_setspecific(pthread_key_t key, const void *value);
extern void *pthread_getspecific(pthread_key_t key);
extern pthread_id_np_t pthread_getthreadid_np(void); 

