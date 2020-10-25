/*
    Small library for reading and writing audio.

    This library forks an audio task and communicates with it
    via a shared memory segment.

    All what this library does can be done in principal
    with ordinary read/write calls on the sound device.

    The Linux audio driver uses so small buffers, however,
    that overruns/underruns are unavoidable in many cases.

    Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>

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

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

/* The shared memory things */

#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef FORK_NOT_THREAD
#include <pthread.h>
#endif

#include "mjpeg_logging.h"

#include "audiolib.h"


#ifdef FORK_NOT_THREAD
static int pid; /* pid of child */
static int shm_seg;
#else
static pthread_t capture_thread;
#endif
#define TIME_STAMP_TOL 100000  /* tolerance for timestamps in us */

#define N_SHM_BUFFS 256 /* Number of buffers, must be a power of 2 */
#define SHM_BUFF_MASK (N_SHM_BUFFS-1)
/* #define BUFFSIZE (8192) */
/* A.Stevens Jul 2000: Several drivers for modern PCI cards can't deliver
   frags larger than 4096 so lets not even try for 8192 byte buffer chunks
*/
#define BUFFSIZE (4096)
#define NBUF(x) ((x)&SHM_BUFF_MASK)

struct shm_buff_s
{
   volatile uint8_t audio_data[N_SHM_BUFFS][BUFFSIZE];
   volatile int used_flag[N_SHM_BUFFS];
   volatile struct timeval tmstmp[N_SHM_BUFFS];
   volatile int status[N_SHM_BUFFS];
   volatile int exit_flag;    /* set by parent */
   volatile int audio_status; /* set by audio task */
   volatile int audio_start;  /* trigger start in playing */
   volatile char error_string[4096];
} *shmemptr;

static int audio_buffer_size = BUFFSIZE;    /* The buffer size actually used */

/* The parameters for audio capture/playback */

static int initialized=0;
static int audio_capt;           /* Flag for capture/playback */
static int mmap_io;              /* Flag for using either mmap or read,write */
static int stereo;               /* 0: capture mono, 1: capture stereo */
static int audio_size;           /* size of an audio sample: 8 or 16 bits */
static int audio_rate;           /* sampling rate for audio */
static int audio_byte_rate;           /* sampling rate for audio Bps*/

/* Buffer counter */

static int n_audio;

/* Bookkeeping of the write buffers */

static char audio_left_buf[BUFFSIZE];  
static int audio_bytes_left;   /* Number of bytes in audio_left_buf */
static unsigned int n_buffs_output, n_buffs_error;
static struct timeval buffer_timestamp;
static int usecs_per_buff;

/* Forward declarations: */

void do_audio(void);
char *audio_strerror(void);
void set_timestamp(struct timeval tmstmp);
void swpcpy(char *dst, char *src, int num);


typedef void *(*start_routine_p)(void *);


/* some (internally used only) error numbers */

static int audio_errno = 0;

#define AUDIO_ERR_INIT     1  /* Not initialized */
#define AUDIO_ERR_INIT2    2  /* allready initialized */
#define AUDIO_ERR_ASIZE    3  /* audio size not 8 or 16 */
#define AUDIO_ERR_SHMEM    4  /* Error getting shared memory segment */
#define AUDIO_ERR_FORK     5  /* Can not fork audio task */
#define AUDIO_ERR_MODE     6  /* Wrong read/write mode */
#define AUDIO_ERR_BSIZE    7  /* Buffer size for read too small */
#define AUDIO_ERR_TMOUT    8  /* Timeout waiting for audio */
#define AUDIO_ERR_BOVFL    9  /* Buffer overflow when writing */
#define AUDIO_ERR_ATASK   99  /* Audio task died - more in shmemptr->error_string */

static char errstr[4096];

char *audio_strerror(void)
{
   switch(audio_errno)
   {
      case 0:
         strcpy(errstr,"No Error");
         break;
      case AUDIO_ERR_INIT:
         strcpy(errstr,"Audio not initialized");
         break;
      case AUDIO_ERR_INIT2:
         strcpy(errstr,"audio_init called but audio allready initialized");
         break;
      case AUDIO_ERR_ASIZE:
         strcpy(errstr,"audio sample size not 8 or 16");
         break;
      case AUDIO_ERR_SHMEM:
         strcpy(errstr,"Audio: Error getting shared memory segment");
         break;
      case AUDIO_ERR_FORK:
         strcpy(errstr,"Can not fork audio task");
         break;
      case AUDIO_ERR_MODE:
         strcpy(errstr,"Audio: Wrong read/write mode");
         break;
      case AUDIO_ERR_BSIZE:
         strcpy(errstr,"Audio: Buffer size for read too small");
         break;
      case AUDIO_ERR_TMOUT:
         strcpy(errstr,"Timeout waiting for audio initialization");
         break;
      case AUDIO_ERR_BOVFL:
         strcpy(errstr,"Buffer overflow writing audio");
         break;
      case AUDIO_ERR_ATASK:
         sprintf(errstr,"Audio task died. Reason: %s",shmemptr->error_string);
         break;
      default:
         strcpy(errstr,"Audio: Unknown error");
   }
   return errstr;
}

/*
 * audio_init: Initialize audio system.
 *
 *      a_read     0: User is going to write (output) audio
 *                 1: User is going to read (input) audio
 *      use_read_write 0: use mmap io as opposed to
 *                     1: read/write system calls
 *      a_stereo   0: mono, 1: stereo
 *      a_size     size of an audio sample: 8 or 16 bits
 *      a_rate     sampling rate for audio
 *
 *      returns 0 for success, -1 for failure
 *
 */

int audio_init(int a_read, int use_read_write,
               int a_stereo, int a_size, int a_rate)
{
   int i;

   /* Check if the audio task is allready initialized */

   if(initialized) { audio_errno = AUDIO_ERR_INIT2; return -1; }

   /* Checks of parameters */

   if (a_size != 8 && a_size != 16) { audio_errno = AUDIO_ERR_ASIZE; return -1; }

   if( use_read_write )
	   mjpeg_info( "Using read(2)/write(2) system call for capture/playpack");
   else
	   mjpeg_info( "Using mmap(2) system call for capture/playback");
   /* Copy our parameters into static space */

   audio_capt = a_read;
   mmap_io    = !use_read_write;
   stereo     = a_stereo;
   audio_size = a_size;
   audio_rate = a_rate;

   /* Reset counters */

   n_audio = 0;
   audio_bytes_left = 0;
   n_buffs_output   = 0;
   n_buffs_error    = 0;
   buffer_timestamp.tv_sec  = 0;
   buffer_timestamp.tv_usec = 0;

   /*
    * Calculate bytes/second of the audio stream
    */

   audio_byte_rate = audio_rate;
   if (stereo)         audio_byte_rate *= 2;
   if (audio_size==16) audio_byte_rate *= 2;

   /* Set audio buffer size */

   audio_buffer_size = BUFFSIZE;
   /* A.Stevens Jul 2000 modified to allow cards with max frag size of
	  4096.... if(tmp<88200) audio_buffer_size = 4096; */
   if(audio_byte_rate<44100) audio_buffer_size = BUFFSIZE/2;
   if(audio_byte_rate<22050) audio_buffer_size = BUFFSIZE/4;

   /* Do not change the following calculations,
      they are this way to avoid overflows ! */
   usecs_per_buff  = audio_buffer_size*100000/audio_byte_rate;
   usecs_per_buff *= 10;

#ifdef FORK_NOT_THREAD
   /* Allocate shared memory segment */

   shm_seg = shmget(IPC_PRIVATE, sizeof(struct shm_buff_s), IPC_CREAT | 0777);
   if(shm_seg < 0) { audio_errno = AUDIO_ERR_SHMEM; return -1; }

   /* attach the segment and get its address */

   shmemptr = (struct shm_buff_s *) shmat(shm_seg,0,0);
   if(shmemptr < 0) { audio_errno = AUDIO_ERR_SHMEM; return -1; }

   /* mark the segment as destroyed, it will be removed after
      the last process which had attached it is gone */

   if( shmctl( shm_seg, IPC_RMID, (struct shmid_ds *)0 ) == -1 )
   {
      audio_errno = AUDIO_ERR_SHMEM;
      return -1;
   }
#else
   shmemptr = (struct shm_buff_s *) malloc(sizeof(struct shm_buff_s));
   if( shmemptr == NULL )
	  { audio_errno = AUDIO_ERR_SHMEM; return -1; }
#endif
   /* set the flags in the shared memory */

   for(i=0;i<N_SHM_BUFFS;i++) shmemptr->used_flag[i] = 0;
   for(i=0;i<N_SHM_BUFFS;i++) shmemptr->status[i]    = 0;
   shmemptr->exit_flag    = 0;
   shmemptr->audio_status = 0;
   shmemptr->audio_start  = 0;

   /* do the fork */

#ifdef FORK_NOT_THREAD
   pid = fork();
   if(pid<0)
   {
      audio_errno = AUDIO_ERR_FORK;
      return -1;
   }

   /* the child goes into the audio task */

   if (pid==0)
   {
      /* The audio buffers in Linux are ridiculosly small,
         therefore the audio task must have a high priority,
         This call will fail if we are not superuser, we don't care.
       */

      setpriority(PRIO_PROCESS, getpid(), -20);

      /* Ignore SIGINT while capturing, the parent wants to catch it */

      if(audio_capt) signal(SIGINT,SIG_IGN);
      do_audio();
      exit(0);
   }
#else
   
   if( pthread_create( &capture_thread, NULL, (start_routine_p)do_audio, NULL) )
	 {
	   audio_errno = AUDIO_ERR_FORK;
	   return -1;
	 }
   
#endif
   /* Since most probably errors happen during initialization,
      we wait until the audio task signals either success or failure */

   for(i=0;;i++)
   {
      /* Check for timeout, 10 Seconds should be plenty */
      if(i>1000)
      {
#ifdef FORK_NOT_THREAD
         kill(pid,SIGKILL);
         shmemptr->exit_flag = 1;
         waitpid(pid,0,0);
#else
         shmemptr->exit_flag = 1;
		 pthread_cancel( capture_thread );
		 pthread_join( capture_thread, NULL );
#endif
         audio_errno = AUDIO_ERR_TMOUT;
         return -1;
      }
      if(shmemptr->audio_status<0)
      {
         audio_errno = AUDIO_ERR_ATASK;
         return -1;
      }
      if(shmemptr->audio_status>0) break;
      usleep(10000);
   }

   initialized = 1;
   return 0;
}

/*
 * audio_shutdown: Shutdown audio system
 *
 * It is important that this routine is called whenever the host
 * program finished, or else there will be the audio task
 * left over, having the sound device still opened and preventing
 * other programs from using sound.
 *
 */

void audio_shutdown(void)
{
   if(!initialized) return;

   /* show the child we want to exit */

   shmemptr->exit_flag = 1;
#ifdef FORK_NOT_THREAD
   waitpid(pid,0,0);
#else
   pthread_join( capture_thread, NULL );
#endif

   initialized = 0;
}

long audio_get_buffer_size(void)
{
   return audio_buffer_size;
}

/*
 * audio_start: Actually trigger the start of audio after all
 *              initializations have been done.
 *              Only for playing!
 *
 *      returns 0 for success, -1 for failure
 */

void audio_start(void)
{
   /* signal the audio task that we want to start */

   shmemptr->audio_start = 1;
}

/*
 * set_timestamp:
 * Set buffer timestamp either to the value of the tmstmp parameter
 * or calculate it from previous value
 */

void
set_timestamp(struct timeval tmstmp)
{
   if( tmstmp.tv_sec != 0 )
   {
      /* Time stamp is realiable */
      buffer_timestamp = tmstmp;
   }
   else
   {
      /* Time stamp not reliable - calculate from previous */
      if(buffer_timestamp.tv_sec != 0)
      {
         buffer_timestamp.tv_usec += usecs_per_buff;
         while(buffer_timestamp.tv_usec>=1000000)
         {
            buffer_timestamp.tv_usec -= 1000000;
            buffer_timestamp.tv_sec  += 1;
         }
      }
   }
}


/*
 * swpcpy: like memcpy, but bytes are swapped during copy
 */

void swpcpy(char *dst, char *src, int num)
{
   int i;

   num &= ~1; /* Safety first */

   for(i=0;i<num;i+=2)
   {
      dst[i  ] = src[i+1];
      dst[i+1] = src[i  ];
   }
}

/*
 * audio_read: Get audio data, if available
 *             Behaves like a nonblocking read
 *
 *    buf      Buffer where to copy the data
 *    size     Size of the buffer
 *    swap     Flag if to swap the endian-ness of 16 bit data
 *    tmstmp   returned: timestamp when buffer was finished reading
 *                       contains 0 if time could not be reliably determined
 *    status   returned: 0 if buffer is corrupted
 *                       1 if buffer is ok.
 *             tmstmp and status are set only if something was read
 *
 *    returns  the number of bytes in the buffer,
 *             0 if nothing available
 *            -1 if an error occured
 *
 */

int audio_read( uint8_t *buf, int size, int swap, 
			    struct timeval *tmstmp, int *status)
{
   if(!initialized) { audio_errno = AUDIO_ERR_INIT; return -1; }

   /* Is audio task still ok ? */

   if(shmemptr->audio_status < 0) { audio_errno = AUDIO_ERR_ATASK; return -1; }

   if(!audio_capt) { audio_errno = AUDIO_ERR_MODE;  return -1; }

   if(size<audio_buffer_size) { audio_errno = AUDIO_ERR_BSIZE; return -1; }

   /* Check if a new audio sample is ready */

   if(shmemptr->used_flag[NBUF(n_audio)])
   {
      /* Got an audio sample, copy it to the output buffer */

      if(swap && audio_size==16)
         swpcpy((void*)buf,(void*)shmemptr->audio_data[NBUF(n_audio)],audio_buffer_size);
      else
         memcpy((void*)buf,(void*)shmemptr->audio_data[NBUF(n_audio)],audio_buffer_size);

      /* set the other return values */

      set_timestamp(shmemptr->tmstmp[NBUF(n_audio)]);
      if(tmstmp) *tmstmp = buffer_timestamp;
      if(status) *status = shmemptr->status[NBUF(n_audio)] > 0;
      
      /* reset used_flag, increment n-audio */

      shmemptr->status[NBUF(n_audio)]    = 0;
      shmemptr->used_flag[NBUF(n_audio)] = 0;
      n_audio++;
      return audio_buffer_size;
   }

   return 0;
}

static void update_output_status(void)
{
   while(shmemptr->status[NBUF(n_buffs_output)])
   {
      if(shmemptr->status[NBUF(n_buffs_output)] < 0) n_buffs_error++;
      set_timestamp(shmemptr->tmstmp[NBUF(n_buffs_output)]);
      shmemptr->status[NBUF(n_buffs_output)] = 0;
      n_buffs_output++;
   }
}

void audio_get_output_status(struct timeval *tmstmp, unsigned int *nb_out, unsigned int *nb_err)
{
   if(tmstmp) *tmstmp = buffer_timestamp;
   if(nb_out) *nb_out = n_buffs_output;
   if(nb_err) *nb_err = n_buffs_error;
}
   

/*
 * audio_write: Buffer audio data for output
 *              Behaves like a nonblocking write
 *
 *    buf       Buffer with audio data
 *    size      Size of the buffer
 *    swap      Flag if to swap the endian-ness of 16 bit data
 *
 *    returns   the number of bytes actually written
 *              -1 if an error occured
 *
 *              If the number of bytes actually written is smaller
 *              than size, the audio ringbuffer is completely filled
 *
 */

int audio_write(uint8_t *buf, int size, int swap)
{
   int nb;

   if(!initialized) { audio_errno = AUDIO_ERR_INIT; return -1; }

   /* Is audio task still ok ? */

   if(shmemptr->audio_status < 0) { audio_errno = AUDIO_ERR_ATASK; return -1; }

   if(audio_capt) { audio_errno = AUDIO_ERR_MODE;  return -1; }

   update_output_status();

   /* If the number of bytes we got isn't big enough to fill
      the next buffer, copy buf into audio_left_buf */

   if (audio_bytes_left+size < audio_buffer_size)
   {
      memcpy(audio_left_buf+audio_bytes_left,buf,size);
      audio_bytes_left += size;
      return size;
   }

   nb = 0;

   /* if audio_left_buf contains something, output that first */

   if (audio_bytes_left)
   {
      memcpy(audio_left_buf+audio_bytes_left,buf,audio_buffer_size-audio_bytes_left);

      if(shmemptr->used_flag[NBUF(n_audio)])
      {
         audio_errno = AUDIO_ERR_BOVFL;
         return -1;
      }

      if(swap && audio_size==16)
         swpcpy((void*)shmemptr->audio_data[NBUF(n_audio)],audio_left_buf,audio_buffer_size);
      else
         memcpy((void*)shmemptr->audio_data[NBUF(n_audio)],audio_left_buf,audio_buffer_size);

      shmemptr->used_flag[NBUF(n_audio)] = 1;

      nb = audio_buffer_size-audio_bytes_left;
      audio_bytes_left = 0;
      n_audio++;
   }

   /* copy directly to the shmem buffers */

   while(size-nb >= audio_buffer_size)
   {
      if(shmemptr->used_flag[NBUF(n_audio)])
      {
         audio_errno = AUDIO_ERR_BOVFL;
         return -1;
      }

      if(swap && audio_size==16)
         swpcpy((void*)shmemptr->audio_data[NBUF(n_audio)],(void*)(buf+nb),audio_buffer_size);
      else
         memcpy((void*)shmemptr->audio_data[NBUF(n_audio)],(void*)(buf+nb),audio_buffer_size);

      shmemptr->used_flag[NBUF(n_audio)] = 1;

      nb += audio_buffer_size;
      n_audio++;
   }

   /* copy the remainder into audio_left_buf */

   if(nb<size)
   {
      audio_bytes_left = size-nb;
      memcpy(audio_left_buf,buf+nb,audio_bytes_left);
   }

   return size;
}

/*
 * The audio task
 */

#ifdef HAVE_SYS_SOUNDCARD_H
static void system_error(const char *str, int fd, int use_strerror)
{
   if(use_strerror)
      sprintf((char*)shmemptr->error_string, "Error %s - %s",str,strerror(errno));
   else
      sprintf((char*)shmemptr->error_string, "Error %s",str);

   shmemptr->audio_status = -1;
   if( fd >= 0 )
	   close(fd);
#ifdef FORK_NOT_THREAD
      exit(1);
#else
	  pthread_exit(NULL);
#endif
}
#endif /* HAVE_SYS_SOUNDCARD_H */

#ifdef HAVE_SYS_SOUNDCARD_H
void do_audio(void)
{

   int fd = -1;
   int tmp, ret, caps, afmt, frag;
   int nbdone, nbque, ndiff, nbpend, nbset, maxdiff;

   uint8_t *buf = NULL;
   fd_set selectset;

   struct count_info count;
   struct audio_buf_info info;
   struct timeval tv;

   const char *audio_dev_name;

#ifndef FORK_NOT_THREAD
   struct sched_param schedparam;
   sigset_t blocked_signals;

   /* Set the capture thread in a reasonable state - cancellation enabled
      and asynchronous, SIGINT's ignored... */
   /* PTHREAD_CANCEL_ASYNCHRONOUS is evil. */
/*   if( pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL) )
   {
      system_error( "Bad pthread_setcancelstate", fd, 0 );
   }
   if( pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL) )
   {
      system_error( "Bad pthread_setcanceltype", fd, 0 );
   }*/

   sigaddset( &blocked_signals, SIGINT );
   if( pthread_sigmask( SIG_BLOCK, &blocked_signals, NULL ))
   {
      system_error( "Bad pthread_sigmask", fd, 0 );
   }
#endif
	 

/*
 * Fragment size and max possible number of frags
 */ 

   switch (audio_buffer_size)
   {
   case 8192: frag = 0x7fff000d; break;
   case 4096: frag = 0x7fff000c; break;
   case 2048: frag = 0x7fff000b; break;
   case 1024: frag = 0x7fff000a; break;
   default:
	 system_error("Audio internal error - audio_buffer_size",fd,0);
   }
   /* if somebody plays with BUFFSIZE without knowing what he does ... */
   if (audio_buffer_size>BUFFSIZE)
      system_error("Audio internal error audio_buffer_size > BUFFSIZE",fd,0);

/*
 * Open Audio device, set number of frags wanted
 */

   audio_dev_name = getenv("LAV_AUDIO_DEV");
   if(!audio_dev_name) audio_dev_name = "/dev/dsp";

   if(audio_capt)
      fd=open(audio_dev_name, O_RDONLY, 0);
   else
      fd=open(audio_dev_name, O_RDWR,   0);

   if (fd<0) system_error(audio_dev_name,fd,1);

   ret = ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &frag);
   if(ret<0) system_error("in ioctl SNDCTL_DSP_SETFRAGMENT", fd, 1);

/*
 * Setup sampling parameters.
 */

   afmt = (audio_size==16) ? AFMT_S16_LE : AFMT_U8;
   tmp = afmt;
   ret = ioctl(fd, SNDCTL_DSP_SETFMT, &tmp);
   if(ret<0 || tmp!=afmt) system_error("setting sound format",fd,0);

   tmp = stereo; /* 0=mono, 1=stereo */
   ret = ioctl(fd, SNDCTL_DSP_STEREO, &tmp);
   if(ret<0 || tmp!=stereo) system_error("setting mono/stereo",fd,0);

   tmp = audio_rate;
   ret = ioctl(fd, SNDCTL_DSP_SPEED, &tmp);
   if(ret<0) {
       system_error("setting sound rate",fd,0);
   } else if(tmp != audio_rate) {
       mjpeg_warn("Sound card told us it's using rate %dHz instead of %dHz", tmp, audio_rate);
   }

/* Calculate number of bytes corresponding to TIME_STAMP_TOL */

   maxdiff = audio_byte_rate / (1000000/TIME_STAMP_TOL);

/*
 * Check that the device has capability to do mmap and trigger
 */
   
   if(mmap_io) {
	   ret = ioctl(fd, SNDCTL_DSP_GETCAPS, &caps);
	   if(ret<0) system_error("getting audio device capabilities",fd,1);
	   if (!(caps & DSP_CAP_TRIGGER) || !(caps & DSP_CAP_MMAP))
		   system_error("Soundcard cant do mmap or trigger",fd,0);
   }

/*
 * Get the size of the input/output buffer and do the mmap
 */

   if (audio_capt)
      ret = ioctl(fd, SNDCTL_DSP_GETISPACE, &info);
   else
      ret = ioctl(fd, SNDCTL_DSP_GETOSPACE, &info);

   if(ret<0) system_error("in ioctl SNDCTL_DSP_GET[IO]SPACE",fd,1);

   if (info.fragsize != audio_buffer_size)
      system_error("Soundcard fragment size unexpected",fd,0);

/*
 * Original comment:
 * Normally we should get at least 8 fragments (if we use 8KB buffers)
 * or even more if we use 4KB od 2 KB buffers
 * We consider 4 fragments as the absolut minimum here!
 * 
 * A.Stevens Jul 2000: I'm a bit puzzled by the above.  A 4096 byte
 * buffer takes 1/20th second to fill at 44100 stereo.  So provide we
 * empty one frag in less than this we should be o.k. hardly onerous.
 * Presumably the problem was that this code wasn't running real-time
 * and so could get starved on a load system.
 * Anyway, insisting on 8 frags of 8192 bytes puts us sure out of luck
 * drivers for quite a few modern PCI soundcards ... so lets try for 2
 * and see what real-time scheduling can do!
 */

   if (info.fragstotal < 2)
   {
	   system_error("Could not get enough audio buffer fragments",fd,0);
   }

   tmp = info.fragstotal*info.fragsize;

   if( mmap_io )
   {
	   if (audio_capt)
		   buf=mmap(NULL, tmp, PROT_READ , MAP_SHARED, fd, 0);
	   else
		   buf=mmap(NULL, tmp, PROT_WRITE, MAP_SHARED, fd, 0);

	   if (buf==MAP_FAILED)
		   system_error("mapping audio buffer "
                        "(consider using read/write instead of mmap)",fd, 1);
	   
	   /*
		* Put device into hold
		*/

	   tmp = 0;
	   ret = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	   if(ret<0) system_error("in ioctl SNDCTL_DSP_SETTRIGGER",fd,1);

   }


/*
 * Signal the parent that initialization is done
 */

   shmemptr->audio_status = 1;

/*
 * nbdone is the number of buffers processed by the audio driver
 *        so far (ie. the number of buffers read or written)
 * nbque  is the number of buffers which have been queued so far
 *        for playing (not used in audio capture)
 * nbset  Number of buffers set (with real data or 0s)
 *
 * If we do playback: Wait until the first buffer arrives
 */

   nbdone = 0;
   nbque  = 0;
   nbset  = 0;
   if(!audio_capt)
   {
      while(!shmemptr->audio_start)
      {
         usleep(10000);
         if(shmemptr->exit_flag)
		 {
#ifndef FORK_NOT_THREAD
			 exit(0);
#else
			 pthread_exit(NULL);
#endif
		 }
      }
      /* Copy as many buffers as are allready here */
      for(nbque=0;nbque<info.fragstotal;nbque++)
      {
         if(!shmemptr->used_flag[NBUF(nbque)]) break;
         if (mmap_io) {
            memcpy(buf+nbque*info.fragsize,
                   (void*) shmemptr->audio_data[NBUF(nbque)],
                   info.fragsize);
         } else {
            write(fd,(void *)shmemptr->audio_data[NBUF(nbque)],
                  info.fragsize);
         }
         /* Mark the buffer as free */
         shmemptr->used_flag[NBUF(nbque)] = 0;
      }
      for(nbset=nbque;nbset<info.fragstotal;nbset++)
         if (mmap_io) {
            memset(buf+nbset*info.fragsize,0,info.fragsize);
         } else {
            char buf[info.fragsize];
            memset(buf,0,info.fragsize);
            write(fd,buf,info.fragsize);
         }
   }

#ifndef FORK_NOT_THREAD
   /* Now we're ready to go move to Real-time scheduling... */
   schedparam.sched_priority = 1;

   if(setpriority(PRIO_PROCESS, 0, -20)) { /* Give myself maximum priority */ 
      mjpeg_warn("Unable to set negative priority for audio thread.");
   }
   if( (ret = pthread_setschedparam( pthread_self(), SCHED_FIFO, &schedparam ) ) ) {
      mjpeg_warn("Pthread Real-time scheduling for audio thread could not be enabled."); 
   }
#endif

/*
 * Fire up audio device for mmap capture playback (not necessary for read)
 */

   if( mmap_io )
   {
	   if(audio_capt)
		   tmp = PCM_ENABLE_INPUT;
	   else
		   tmp = PCM_ENABLE_OUTPUT;

	   ret = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	   if(ret<0) system_error("in ioctl SNDCTL_DSP_SETTRIGGER",fd,1);
   }


   /* The recording/playback loop */

   while(1)
   {
      /* Wait until new audio data can be read/written */


	   if( mmap_io )
	   {
		   FD_ZERO(&selectset);
		   FD_SET(fd, &selectset);
           retry:
		   if(audio_capt)
			   ret = select(fd+1, &selectset, NULL, NULL, NULL);
		   else 
			   ret = select(fd+1, NULL, &selectset, NULL, NULL);
		   
		   if(ret<0)
                   {
                           if (errno == EINTR)
                                goto retry;
                           else
                                system_error("waiting on audio with select",fd,1);
                   }
	   }
	   else
	   {
          if (audio_capt) {
             if( read(fd, (void *)shmemptr->audio_data[NBUF(nbdone)], info.fragsize ) 
                     != info.fragsize )
             {
                system_error( "Sound driver returned partial fragment!\n", fd,1 );
             }
          }
	   }

      /* Get time - this time is after at least one buffer has been
         recorded/played (because select did return), and before the
         the audio status obtained by the following ioctl */

      gettimeofday(&tv,NULL);

      /* Get the status of the sound buffer */
	  usleep(1000);
      if(audio_capt)
         ret = ioctl(fd, SNDCTL_DSP_GETIPTR, &count);
      else
         ret = ioctl(fd, SNDCTL_DSP_GETOPTR, &count);

      if (ret<0) system_error("in ioctl SNDCTL_DSP_GET[IO]PTR",fd,1);

      /* Get the difference of minimum number of bytes after the select call
         and bytes actually present - this gives us a measure of accuracy
         of the time in tv.
		 TODO
         Note: count.bytes can overflow in extreme situations (more than
               3 hrs recording with 44.1 KHz, 16bit stereo), ndiff should
               be calculated correctly.
      */

      ndiff = count.bytes - audio_buffer_size*(nbdone+1);

	  /* Uncomment this and run testrec if you're getting audio capture 
		 problems...
	   */
	  /*
		mjpeg_info( "CB=%08d ND=%06d BL=%03d NB=%d", count.bytes, ndiff, count.blocks, NBUF(nbdone) );
	  */
      if(ndiff>maxdiff)
	  {
		  tv.tv_sec = tv.tv_usec = 0;
	  }
	  else
	  {
		  /* Adjust timestamp to take into account delay between sync
			 and now indicated by ndiff */
		  tv.tv_usec -= ndiff * 1000000 / audio_byte_rate;
		  if( tv.tv_usec < 0 )
		  {
			  tv.tv_usec += 1000000;
			  tv.tv_sec -= 1;
		  }
	  }

      if(audio_capt)
      {
         /* if exit_flag is set, exit immediatly */

         if(shmemptr->exit_flag)
		   {
            shmemptr->audio_status = -1;
			close(fd);
#ifdef FORK_NOT_THREAD
            exit(0);
#else
			pthread_exit( NULL );
#endif
         }

         /* copy the ready buffers to our audio ring buffer */
		 
		 if( mmap_io )
			 nbpend = count.blocks;
		 else
			 nbpend = 1;

         while(nbpend)
         {

            /* Check if buffer nbdone in the ring buffer is free */

            if(shmemptr->used_flag[NBUF(nbdone)])
               system_error("Audio ring buffer overflow",fd,0);
			
			if( mmap_io )
				memcpy((void*) shmemptr->audio_data[NBUF(nbdone)],
					   buf+(nbdone%info.fragstotal)*info.fragsize,
					   info.fragsize);

            /* Get the status of the sound buffer after copy,
               this permits us to see if an overrun occured */

            ret = ioctl(fd, SNDCTL_DSP_GETIPTR, &count);
            if(ret<0) system_error("in ioctl SNDCTL_DSP_GETIPTR",fd,1);
			
			if( mmap_io )
				nbpend += count.blocks;

            /* if nbpend >= total frags, a overrun most probably occured */
            shmemptr->status[NBUF(nbdone)] = (nbpend >= info.fragstotal) ? -1 : 1;
            shmemptr->tmstmp[NBUF(nbdone)] = tv;
            shmemptr->used_flag[NBUF(nbdone)] = 1;

            nbdone++;
            nbpend--;
            /* timestamps of all following buffers are unreliable */
            tv.tv_sec = tv.tv_usec = 0;
         }
      }
      else
      {
         /* Update the number and status of frags(=buffers) already output */

         nbpend = count.blocks;

         while(nbpend)
         {
            /* check for overflow of the status flags in the ringbuffer */
            if(shmemptr->status[NBUF(nbdone)])
               system_error("Audio ring buffer overflow",fd,0);
            /* We have a buffer underrun during write if nbdone>=nbque */
            shmemptr->tmstmp[NBUF(nbdone)] = tv;
            shmemptr->status[NBUF(nbdone)] = (nbdone<nbque) ? 1 : -1;
            nbdone++;
            nbpend--;
            /* timestamps of all following buffers are unreliable */
            tv.tv_sec = tv.tv_usec = 0;
         }

         /* If exit_flag is set and all buffers are played, exit */

         if(shmemptr->exit_flag && nbdone >= nbque)
         {
            shmemptr->audio_status = -1;
			close(fd);
#ifdef FORK_NOT_THREAD
            exit(0);
#else
			pthread_exit( NULL );
#endif
         }

         /* Fill into the soundcard memory as many buffers
            as fit and are available */

         while(nbque-nbdone < info.fragstotal)
         {
            if(!shmemptr->used_flag[NBUF(nbque)]) break;

            if(nbque>nbdone)
               if (mmap_io) {
                  memcpy(buf+(nbque%info.fragstotal)*info.fragsize,
                         (void*) shmemptr->audio_data[NBUF(nbque)],
                         info.fragsize);
               } else {
                  write(fd,(void *)shmemptr->audio_data[NBUF(nbque)],
                        info.fragsize);
               }

            /* Mark the buffer as free */
            shmemptr->used_flag[NBUF(nbque)] = 0;

            nbque++;
         }
         if(nbset<nbque) nbset = nbque;
         while(nbset-nbdone < info.fragstotal)
         {
            if (mmap_io) {
               memset(buf+(nbset%info.fragstotal)*info.fragsize,0,info.fragsize);
            } else {
               char buf[info.fragsize];
               memset(buf,0,info.fragsize);
               write(fd,buf,info.fragsize);
            }
            nbset++;
         }
      }
   }
}
#else
void do_audio()
{
  fprintf(stderr, "Unsupported audio system in audiolib.c: no soundcard.hn");
}
#endif
