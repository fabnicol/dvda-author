/*

  Synchrolib - various useful synchronisation primitives
  

  (C) 2001 Andrew Stevens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


   Damn: shouldn't have to write this - but whenever you need
   something, well *that's* when you don't have Internet access.

   Bloody annyong that pthread_mutex's lock/unlock is only supposed to
   work properly if the same thread does the locking and unlocking. Gaah!
 */

#ifndef SYNCHROLIB_H
#define SYNCHROLIB_H
#include <pthread.h>

/* Synchronisation condition */

typedef struct _sync_guard {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	volatile int predicate;
} sync_guard_t;

typedef struct _semaphore {
	pthread_mutex_t mutex;
	pthread_cond_t raised;
	volatile int count;
} mp_semaphore_t;



#define SEMAPHORE_INITIALIZER { PTHREAD_MUTEX_INITIALIZER, \
                                PTHREAD_COND_INITIALIZER, \
                                0 }
#define GUARD_INITIALIZER    { PTHREAD_MUTEX_INITIALIZER, \
                                PTHREAD_COND_INITIALIZER, \
                                0 }
void sync_guard_init( sync_guard_t *guard, int init );

void sync_guard_test( sync_guard_t *guard);

void sync_guard_update( sync_guard_t *guard, int predicate );


void mp_semaphore_init( mp_semaphore_t *sema, int init_count );

void mp_semaphore_wait( mp_semaphore_t *sema);

void mp_semaphore_signal( mp_semaphore_t *sema, int count );

void mp_semaphore_set( mp_semaphore_t *sema );


#endif
