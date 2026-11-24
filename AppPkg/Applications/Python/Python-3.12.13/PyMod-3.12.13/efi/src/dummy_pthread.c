/** @file
    UEFI Python support: dummy_pthread.

    Copyright (c) 2011-2026, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    https://opensource.org/license/bsd-2-clause-patent.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <sys/time.h>
#include <errno.h>
#include <pthread.h>

#define PTHREAD_MAX_MUTEXES 256
#define PTHREAD_MAX_CONDS 256
#define PTHREAD_MAX_KEYS 256


static int mutexes[PTHREAD_MAX_MUTEXES] = {0};

int pthread_mutex_init(pthread_mutex_t *mutex,  
                       const pthread_mutexattr_t *attr)
{
   for(int i = 0; i < PTHREAD_MAX_MUTEXES; i++) {
      if(mutexes[i] == 0) {
         mutexes[i] = 1;
         *mutex = (pthread_mutex_t)(mutexes + i);
         return 0;
      }
   }

   return ENOMEM;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
   int *m = (int*)*mutex;
   if(m >= mutexes && m < mutexes + PTHREAD_MAX_MUTEXES) {
      *m = 0;
      return 0;
   }

   return EINVAL;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
   int *m = (int*)*mutex;
   if(m >= mutexes && m < mutexes + PTHREAD_MAX_MUTEXES) {
      if(*m > 1)
         return EDEADLK;
      *m += 1;
      return 0;
   } 

   return EINVAL;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
   int *m = (int*)*mutex;
   if(m >= mutexes && m < mutexes + PTHREAD_MAX_MUTEXES) {
      if(*m < 2)
         return EPERM;
      *m -= 1;
      return 0;
   } 

   return EINVAL;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
   int *m = (int*)*mutex;
   if(m >= mutexes && m < mutexes + PTHREAD_MAX_MUTEXES) {
      if(*m > 1)
         return EBUSY;
      *m += 1;
      return 0;
   } 

   return EINVAL;
}

static int conds[PTHREAD_MAX_CONDS] = {0};

int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr)
{
   for(int i = 0; i < PTHREAD_MAX_CONDS; i++) {
      if(conds[i] == 0) {
         conds[i] = 1;
         *cond = (pthread_cond_t)(conds + i);
         return 0;
      }
   }

   return ENOMEM;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
   int *c = (int*)*cond;
   if(c >= conds && c < conds + PTHREAD_MAX_CONDS) {
      *c = 0;
      return 0;
   }

   return EINVAL;
}

int pthread_cond_signal(pthread_cond_t *cond)
{
   int *c = (int*)*cond;
   if(c >= conds && c < conds + PTHREAD_MAX_CONDS) {
      return 0;
   } 

   return EINVAL;
}
   
int pthread_cond_broadcast(pthread_cond_t *cond)
{
   int *c = (int*)*cond;
   if(c >= conds && c < conds + PTHREAD_MAX_CONDS) {
      return 0;
   } 

   return EINVAL;
}
   
int pthread_cond_timedwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
   int *c = (int*)*cond;
   if(c >= conds && c < conds + PTHREAD_MAX_CONDS) {
      return 0;
   } 

   return EINVAL;
}
   
int pthread_cond_wait(pthread_cond_t *cond,
                      pthread_mutex_t *mutex)
{
   int *c = (int*)*cond;
   if(c >= conds && c < conds + PTHREAD_MAX_CONDS) {
      return 0;
   } 

   return EINVAL;
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
{
   return EINVAL;   
}

int pthread_detach(pthread_t thread)
{
   return EINVAL;
}

pthread_t pthread_self(void)
{
   return (pthread_t)1;
}

void __attribute__((__noreturn__)) pthread_exit(void *status)
{
   while(1);
}

static void* keys[PTHREAD_MAX_KEYS] = {0};

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
   for(int i = 0; i < PTHREAD_MAX_KEYS; i++) {
      if(keys[i] == NULL) {
         keys[i] = (void*)1;
         *key = (pthread_key_t)(keys + i);
         return 0;
      }
   }

   return ENOMEM;
}

int pthread_key_delete(pthread_key_t key)
{
   void **k = (void**)key;
   if(k >= keys && k < keys + PTHREAD_MAX_KEYS  ) {
      *k = NULL;
      return 0;
   } 

   return EINVAL;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
   void **k = (void**)key;
   if(k >= keys && k < keys + PTHREAD_MAX_KEYS  ) {
      *k = (void*)value;
      return 0;
   } 

   return EINVAL;
}

void *pthread_getspecific(pthread_key_t key)
{
   void **k = (void**)key;
   if(k >= keys && k < keys + PTHREAD_MAX_KEYS  ) {
      if(*k == (void*)1)
         return NULL;
      return *k;
   } 

   return NULL;
}

pthread_id_np_t pthread_getthreadid_np(void)
{
   return (pthread_id_np_t)1;
}
