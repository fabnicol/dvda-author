/*
 
  channel.hh - A class of synchonising FIFO's.  Using the default size of 1
  gives nice CSP / Occam style channels
  
 
  (C) 2003 Andrew Stevens
 
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
 
 
 */

#ifndef CHANNEL_HH
#define CHANNEL_HH
#include <pthread.h>


template<class T, unsigned int size = 1>
class Channel
{
public:
    Channel() :
            fullness(0),
            read(0),
            write(0),
            consumers_waiting(0),
            producers_waiting(0)
    {
#ifdef PTHREAD_MUTEX_ERRORCHECK
        pthread_mutexattr_t mu_attr;
        pthread_mutexattr_t *p_attr = &mu_attr;

        pthread_mutexattr_init(&mu_attr);
        pthread_mutexattr_settype( &mu_attr, PTHREAD_MUTEX_ERRORCHECK );
#else

        pthread_mutexattr_t *p_attr = NULL;
#endif

        pthread_mutex_init( &atomic, p_attr );
        pthread_cond_init( &addition, NULL );
        pthread_cond_init( &removal, NULL );
        pthread_cond_init( &waiting, NULL );
    }

    void Put( const T &in )
    {
        int e;

        e = pthread_mutex_lock( &atomic);
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "1 pthread_mutex_lock=%d\n", e);
            abort();
        }
#endif
        if( fullness == size )
        {
            ++producers_waiting;
            pthread_cond_signal( &waiting );
            while( fullness == size )
            {
                pthread_cond_wait( &removal, &atomic);
            }

            --producers_waiting;
        }
        ++fullness;
        buffer[write] = in;
        write = (write + 1) % size;
        pthread_cond_signal( &addition );
        e  = pthread_mutex_unlock( &atomic );
#ifndef NDEBUG
        if (e != 0)
        {
            fprintf(stderr, "1 pthread_mutex_unlock=%d\n", e);
            abort();
        }
#endif
    }

    void Get( T &out )
    {
        int e;

        e = pthread_mutex_lock( &atomic);
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "2 pthread_mutex_lock=%d\n", e);
            abort();
        }
#endif
        if( fullness == 0 )
        {
            ++consumers_waiting;
            pthread_cond_signal( &waiting );
            while( fullness == 0 )
            {
                pthread_cond_wait( &addition, &atomic );
            }
            --consumers_waiting;
        }
        --fullness;
        out = buffer[read];
        read = (read + 1) % size;
        pthread_cond_signal( &removal );
        e  = pthread_mutex_unlock( &atomic );
#ifndef NDEBUG
        if (e != 0)
        {
            fprintf(stderr, "2 pthread_mutex_unlock=%d\n", e);
            abort();
        }
#endif
    }

    void WaitForNewConsumers()
    {
        int e;
        e = pthread_mutex_lock( &atomic);
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "5 pthread_mutex_lock=%d\n", e);
            abort();
        }
#endif
        unsigned int wait_for = consumers_waiting+1;
        while( fullness > 0 || consumers_waiting < wait_for )
        {
            pthread_cond_wait( &waiting, &atomic);
        }
        e = pthread_mutex_unlock( &atomic );
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "5 pthread_mutex_unlock=%d\n", e);
            abort();
        }
#endif
    }

    void WaitUntilConsumersWaitingAtLeast( unsigned int wait_for )
    {
        int e;

        e = pthread_mutex_lock( &atomic);
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "3 pthread_mutex_lock=%d\n", e);
            abort();
        }
#endif
        while( fullness > 0 || consumers_waiting < wait_for )
        {
            pthread_cond_wait( &waiting, &atomic);
        }
        e  = pthread_mutex_unlock( &atomic );
#ifndef NDEBUG
        if (e != 0)
        {
            fprintf(stderr, "3 pthread_mutex_unlock=%d\n", e);
            abort();
        }
#endif
    }

    void WaitUntilProducersWaitingAtLeast( unsigned int wait_for )
    {
        int e;

        e = pthread_mutex_lock( &atomic);
#ifndef NDEBUG
        if ( e != 0)
        {
            fprintf(stderr, "4 pthread_mutex_lock=%d\n", e);
            abort();
        }
#endif
        while( fullness < size || producers_waiting < wait_for )
        {
            pthread_cond_wait( &waiting, &atomic);
        }
        e  = pthread_mutex_unlock( &atomic );
#ifndef NDEBUG
        if (e != 0)
        {
            fprintf(stderr, "4 pthread_mutex_unlock=%d\n", e);
            abort();
        }
#endif
    }
private:
    pthread_cond_t addition;
    pthread_cond_t removal;
    pthread_cond_t waiting;
    pthread_mutex_t atomic;
    volatile unsigned int fullness;
    volatile unsigned int read;
    volatile unsigned int write;
    volatile unsigned int consumers_waiting;
    volatile unsigned int producers_waiting;
    T buffer[size];
};

#endif // CHANNEL_HH
