/*
 * liblavrec - a librarified Linux Audio Video Record-application
 *
 * Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
 * Extended by:     Gernot Ziegler  <gz@lysator.liu.se>
 *               &  Wolfgang Scherr <scherr@net4you.net>
 *               &  Ronald Bultje   <rbultje@ronald.bitfreak.net>
 *               &  many others
 * 
 * A library for recording MJPEG video from hardware MJPEG
 * video devices such as the Pinnacle/Miro DC10(+), Iomega
 * Buz, the Linux Media Labs LML33, the Matrox Marvel G200,
 * Matrox Marvel G400 and the Rainbow Runner G-series.
 * Can also be used for video-capture from BTTV-devices
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/statfs.h>
#include <mjpeg_types.h>
#include <sys/vfs.h>
#include <stdlib.h>

/* Because of some really cool feature in video4linux1, also known as
 * 'not including sys/types.h and sys/time.h', we had to include it
 * ourselves. In all their intelligence, these people decided to fix
 * this in the next version (video4linux2) in such a cool way that it
 * breaks all compilations of old stuff...
 * The real problem is actually that linux/time.h doesn't use proper
 * macro checks before defining types like struct timeval. The proper
 * fix here is to either fuck the kernel header (which is what we do
 * by defining _LINUX_TIME_H, an innocent little hack) or by fixing it
 * upstream, which I'll consider doing later on. If you get compiler
 * errors here, check your linux/time.h && sys/time.h header setup.
 */
#define _LINUX_TIME_H
#include <linux/videodev.h>
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#include <videodev_mjpeg.h>
#include <pthread.h>

#include "mjpeg_logging.h"
#include "liblavrec.h"
#include "lav_io.h"
#include "audiolib.h"
#include "jpegutils.h"

/* On some systems MAP_FAILED seems to be missing */
#ifndef MAP_FAILED
#define MAP_FAILED ( (caddr_t) -1 )
#endif

#define MJPEG_MAX_BUF 256

#define MIN_QUEUES_NEEDED 2 /* minimal number of queues needed to sync */

#define NUM_AUDIO_TRIES 500 /* makes 10 seconds with 20 ms pause beetween tries */

#define MIN_MBYTES_FREE 10        /* Minimum number of Mbytes that should
                                     stay free on the file system, this is also
                                     only a guess */
#define MIN_MBYTES_FREE_OPEN 20   /* Minimum number of Mbytes that have to be
                                     free in the filesystem when opening a new file */
#define CHECK_INTERVAL 50         /* Interval for checking free space on file system */

#define VALUE_NOT_FILLED -10000

struct YUVP_convert
{
	unsigned char	*mmap;
	unsigned char	*YUVP_buff;
};

typedef struct {
   int    interlaced;                         /* is the video interlaced (even/odd-first)? */
   int    width;                              /* width of the captured frames */
   int    height;                             /* height of the captured frames */
   double spvf;                               /* seconds per video frame */
   int    video_fd;                           /* file descriptor of open("/dev/video") */
   int    has_audio;                          /* whether it has an audio ability */
   struct mjpeg_requestbuffers breq;          /* buffer requests */
   struct mjpeg_sync bsync;
   struct video_mbuf softreq;                 /* Software capture (YUV) buffer requests */
   uint8_t *MJPG_buff;                         /* the MJPEG buffer */
   struct video_mmap mm;                      /* software (YUV) capture info */
   struct YUVP_convert	*YUVP_convert;        /* Software (YUVP) info */
   unsigned char *YUV_buff;                   /* in case of software encoding: the YUV buffer */
   lav_file_t *video_file;                    /* current lav_io.c file we're recording to */
   lav_file_t *video_file_old;                /* previous lav_io.c file we're recording to (finish audio/close) */
   int    num_frames_old;

   uint8_t AUDIO_buff[AUDIO_BUFFER_SIZE];      /* the audio buffer */
   struct timeval audio_t0;
   int    astat;
   long   audio_offset;
   struct timeval audio_tmstmp;
   int    mixer_set;                          /* whether the mixer settings were changed */
   int    audio_bps;                          /* bytes per second for audio stream */
   long   audio_buffer_size;                  /* audio stream buffer size */
   double spas;                               /* seconds per audio sample */
   double sync_lim;                           /* upper limit of 'out-of-sync' - if higher, quit */
   video_capture_stats* stats;                /* the stats */

   uint64_t   MBytes_fs_free;                 /* Free disk space when that was last checked */
   uint64_t   bytes_output_cur;               /* Bytes output to the current output file */
   uint64_t   bytes_last_checked;             /* Number of bytes that were output when the
                                                 free space was last checked */
   int    mixer_volume_saved;                 /* saved recording volume before setting mixer */
   int    mixer_recsrc_saved;                 /* saved recording source before setting mixer */
   int    mixer_inplev_saved;                 /* saved output volume before setting mixer */

   /* the JPEG video encoding thread mess */
   struct encoder_info_s * encoders;          /* for software encoding recording */
   pthread_mutex_t encoding_mutex;            /* for software encoding recording */
   int buffer_valid[MJPEG_MAX_BUF];           /* Non-zero if buffer has been filled */
   int buffer_completed[MJPEG_MAX_BUF];       /* Non-zero if buffer has been compressed/written */
   pthread_cond_t buffer_filled[MJPEG_MAX_BUF];
   pthread_cond_t buffer_completion[MJPEG_MAX_BUF];

   /* thread for correctly timestamping the V4L/YUV buffers */
   pthread_t software_sync_thread;            /* the thread */
   pthread_mutex_t software_sync_mutex;       /* the mutex */
   sig_atomic_t please_stop_syncing;
   unsigned long buffers_queued;		/* evil hack for BTTV-0.8 */
   int software_sync_ready[MJPEG_MAX_BUF];    /* whether the frame has already been synced on */
   pthread_cond_t software_sync_wait[MJPEG_MAX_BUF]; /* wait for frame to be synced on */
   struct timeval software_sync_timestamp[MJPEG_MAX_BUF];

   /* some mutex/cond stuff to make sure we have enough queues left */
   pthread_mutex_t queue_mutex;
   unsigned short queue_left;
   short is_queued[MJPEG_MAX_BUF];
   pthread_cond_t queue_wait;

   int    output_status;

   pthread_mutex_t state_mutex;
   int    state;                              /* recording, paused or stoppped */

   pthread_t capture_thread;
} video_capture_setup;


/* Identity record for software encoding worker thread...
 * Given N workers Worker i compresses frame i,i+n,i+2N and so on.
 * There may not be more workers than there are capture buffers - 1.
 */

typedef struct encoder_info_s {
   lavrec_t *info;
   unsigned int encoder_id;
   unsigned int num_encoders;
   pthread_t thread;
} encoder_info_t;

/* Forward definitions */
static int
lavrec_queue_buffer (lavrec_t *info, unsigned long *num);

static int
lavrec_handle_audio (lavrec_t *info, struct timeval *timestamp);



/******************************************************
 * lavrec_msg()
 *   simplicity function which will give messages
 ******************************************************/

static void lavrec_msg(int type, lavrec_t *info, const char format[], ...) GNUC_PRINTF(3,4);
static void lavrec_msg(int type, lavrec_t *info, const char format[], ...)
{
   char buf[1024];
   va_list args;

   va_start(args, format);
   vsnprintf(buf, sizeof(buf)-1, format, args);
   va_end(args);

   if (!info) /* we can't let errors pass without giving notice */
      mjpeg_error("%s", buf);
   else if (info->msg_callback)
      info->msg_callback(type, buf);
   else if (type == LAVREC_MSG_ERROR)
      mjpeg_error("%s", buf);
}


/******************************************************
 * lavrec_change_state()
 *   changes the recording state
 ******************************************************/

static int
lavrec_change_state_if(lavrec_t *info, int new_state, int require_state)
{
   int okay;
  
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   pthread_mutex_lock(&settings->state_mutex);

   if ((okay = settings->state == require_state) != 0)
   {
      settings->state = new_state;
      if (info->state_changed)
	 info->state_changed(new_state);
   }

   pthread_mutex_unlock(&settings->state_mutex);
   return okay;
}

static void
lavrec_change_state(lavrec_t *info, int new_state)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   pthread_mutex_lock(&settings->state_mutex);

   settings->state = new_state;
   if (info->state_changed)
      info->state_changed(new_state);

   pthread_mutex_unlock(&settings->state_mutex);
}


/******************************************************
 * set_mixer()
 *   set the sound mixer:
 *    flag = 1 : set for recording from the line input
 *    flag = 0 : restore previously saved values
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_set_mixer(lavrec_t *info, int flag)
{
   int fd, var;
   unsigned int sound_mixer_read_input;
   unsigned int sound_mixer_write_input;
   unsigned int sound_mask_input;
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   /* Avoid restoring anything when nothing was set */
   if (flag==0 && settings->mixer_set==0) return 1;

   /* Open the audio device */
   fd = open(info->mixer_dev, O_RDONLY);
   if (fd == -1)
   {
      lavrec_msg(LAVREC_MSG_WARNING, info,
         "Unable to open sound mixer \'%s\', try setting the sound mixer with another tool!!!",
         info->mixer_dev);
      return 1; /* 0 means error, so although we failed return 1 */
   }

   switch(info->audio_src)
   {
      case 'm':
         sound_mixer_read_input  = SOUND_MIXER_READ_MIC;
         sound_mixer_write_input = SOUND_MIXER_WRITE_MIC;
         sound_mask_input        = SOUND_MASK_MIC;
         break;
      case 'c':
         sound_mixer_read_input  = SOUND_MIXER_READ_CD;
         sound_mixer_write_input = SOUND_MIXER_WRITE_CD;
         sound_mask_input        = SOUND_MASK_CD;
         break;
      case 'l':
         sound_mixer_read_input  = SOUND_MIXER_READ_LINE;
         sound_mixer_write_input = SOUND_MIXER_WRITE_LINE;
         sound_mask_input        = SOUND_MASK_LINE;
         break;
      case '1':
         sound_mixer_read_input  = SOUND_MIXER_READ_LINE1;
         sound_mixer_write_input = SOUND_MIXER_WRITE_LINE1;
         sound_mask_input        = SOUND_MASK_LINE1;
         break;
      case '2':
         sound_mixer_read_input  = SOUND_MIXER_READ_LINE2;
         sound_mixer_write_input = SOUND_MIXER_WRITE_LINE2;
         sound_mask_input        = SOUND_MASK_LINE2;
         break;
      case '3':
         sound_mixer_read_input  = SOUND_MIXER_READ_LINE3;
         sound_mixer_write_input = SOUND_MIXER_WRITE_LINE3;
         sound_mask_input        = SOUND_MASK_LINE3;
         break;
      default:
         lavrec_msg(LAVREC_MSG_WARNING, info,
            "Unknown sound source: \'%c\'", info->audio_src);
         close(fd);
         return 1; /* 0 means error, so although we failed return 1 */
   }

   if(flag==1)
   {
      int nerr = 0;

      /* Save the values we are going to change */
      if (settings->mixer_set == 0) {
         if (ioctl(fd, SOUND_MIXER_READ_VOLUME, &(settings->mixer_volume_saved)) == -1) nerr++;
         if (ioctl(fd, SOUND_MIXER_READ_RECSRC, &(settings->mixer_recsrc_saved)) == -1) nerr++;
         if (ioctl(fd, sound_mixer_read_input , &(settings->mixer_inplev_saved)) == -1) nerr++;
         settings->mixer_set = 1;

         if (nerr)
         {
            lavrec_msg (LAVREC_MSG_WARNING, info,
               "Unable to save sound mixer settings");
            lavrec_msg (LAVREC_MSG_WARNING, info,
               "Restore your favorite setting with another tool after capture");
            settings->mixer_set = 0; /* prevent us from resetting nonsense settings */
         }
      }

      /* Set the recording source, audio-level and (if wanted) mute */
      nerr = 0;

      var = sound_mask_input;
      if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &var) == -1) nerr++;
      var = 256*info->audio_level + info->audio_level; /* left and right channel */
      if (ioctl(fd, sound_mixer_write_input, &var) == -1) nerr++;
      if(info->mute) {
         var = 0;
         if (ioctl(fd, SOUND_MIXER_WRITE_VOLUME, &var) == -1) nerr++;
      }

      if (nerr)
      {
         lavrec_msg (LAVREC_MSG_WARNING, info,
            "Unable to set the sound mixer correctly");
         lavrec_msg (LAVREC_MSG_WARNING, info,
            "Audio capture might not be successfull (try another mixer tool!)");
      }

   }
   else
   {
      int nerr = 0;

      /* Restore previously saved settings */
      if (ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &(settings->mixer_recsrc_saved)) == -1) nerr++;
      if (ioctl(fd, sound_mixer_write_input, &(settings->mixer_inplev_saved)) == -1) nerr++;
      if(info->mute)
         if (ioctl(fd, SOUND_MIXER_WRITE_VOLUME, &(settings->mixer_volume_saved)) == -1) nerr++;

      if (nerr)
      {
         lavrec_msg (LAVREC_MSG_WARNING, info,
            "Unable to restore sound mixer settings");
         lavrec_msg (LAVREC_MSG_WARNING, info,
            "Restore your favorite setting with another tool");
      }
      settings->mixer_set = 0;
   }

   close(fd);

   return 1;
}


/******************************************************
 * lavrec_autodetect_signal()
 *   (try to) autodetect signal/norm
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_autodetect_signal(lavrec_t *info)
{
   struct mjpeg_status bstat;
   int i;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   lavrec_msg(LAVREC_MSG_INFO, info, "Auto detecting input and norm ...");

   if (info->software_encoding && (info->video_norm==3 || info->video_src==-1))
   {
      lavrec_msg(LAVREC_MSG_DEBUG, info,
         "Using current input signal settings for non-MJPEG card");
      return 1;
   }

   if (info->video_src == -1) /* detect video_src && norm */
   {
      int n = 0;

      for(i=0;i<2;i++)
      {
         lavrec_msg (LAVREC_MSG_INFO, info,
               "Trying %s ...", (i==2)?"TV tuner":(i==0?"Composite":"S-Video"));

         bstat.input = i;
         if (ioctl(settings->video_fd,MJPIOC_G_STATUS,&bstat) < 0)
         {
            lavrec_msg (LAVREC_MSG_ERROR, info,
               "Error getting video input status: %s",
               (const char*)strerror(errno));
            return 0;
         }

         if (bstat.signal)
         {
            lavrec_msg (LAVREC_MSG_INFO, info,
               "Input present: %s %s",
               bstat.norm==0? "PAL":(info->video_norm==1?"NTSC":"SECAM"),
               bstat.color?"color":"no color");
            info->video_src = i;
            info->video_norm = bstat.norm;
            n++;
         }
         else
         {
            lavrec_msg (LAVREC_MSG_INFO, info,
               "No signal ion specified input");
         }
      }

      switch(n)
      {
         case 0:
            lavrec_msg (LAVREC_MSG_ERROR, info,
               "No input signal ... exiting");
            return 0;
         case 1:
            lavrec_msg (LAVREC_MSG_INFO, info,
               "Detected %s %s",
               info->video_norm==0? "PAL":(info->video_norm==1?"NTSC":"SECAM"),
               info->video_src==0?"Composite":(info->video_src==1?"S-Video":"TV tuner"));
            break;
         default:
            lavrec_msg (LAVREC_MSG_ERROR, info,
               "Input signal on more thn one input source... exiting");
            return 0;
      }

   }
   else if (info->video_norm == 3) /* detect norm only */
   {

      lavrec_msg (LAVREC_MSG_INFO, info,
         "Trying to detect norm for %s ...",
         (info->video_src==2) ? "TV tuner" : (info->video_src==0?"Composite":"S-Video"));

      bstat.input = info->video_src;
      if (ioctl(settings->video_fd,MJPIOC_G_STATUS,&bstat) < 0)
      {
         lavrec_msg (LAVREC_MSG_ERROR, info,
            "Error getting video input status: %s",strerror(errno));
         return 0;
      }

      info->video_norm = bstat.norm;

      lavrec_msg (LAVREC_MSG_INFO, info,
         "Detected %s",
         info->video_norm==0? "PAL":(info->video_norm==1?"NTSC":"SECAM"));
   }

   return 1;
}


/******************************************************
 * lavrec_get_free_space()
 *   get the amount of free disk space
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static uint64_t lavrec_get_free_space(video_capture_setup *settings)
{
   uint64_t blocks_per_MB;
   struct statfs statfs_buf;
   uint64_t MBytes_fs_free;

   /* check the disk space again */
   if (statfs(settings->stats->output_filename, &statfs_buf))
      MBytes_fs_free = 2047; /* some fake value */
   else
   {
      blocks_per_MB = (1024*1024) / statfs_buf.f_bsize;
      MBytes_fs_free = statfs_buf.f_bavail/blocks_per_MB;
   }
   settings->bytes_last_checked = settings->bytes_output_cur;

   return MBytes_fs_free;
}


/******************************************************
 * lavrec_close_files_on_error()
 *   Close the output file(s) if an error occured.
 *   We don't care about further errors.
 ******************************************************/
                         
static void lavrec_close_files_on_error(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if(settings->output_status > 0 && settings->video_file)
   {
      lav_close(settings->video_file);
      settings->video_file = NULL;
   }
   if(settings->output_status > 1 && settings->video_file_old)
   {
      lav_close(settings->video_file_old);
      settings->video_file_old = NULL;
   }

   lavrec_msg(LAVREC_MSG_WARNING, info, "Closing file(s) and exiting - "
      "output file(s) my not be readable due to error");
}


/******************************************************
 * lavrec_output_video_frame()
 *   outputs a video frame and does all the file handling
 *   necessary like opening new files and closing old ones.
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

#define OUTPUT_VIDEO_ERROR_RETURN \
if (settings->output_status==2)   \
{                                 \
   settings->output_status = 3;   \
   return 1;                      \
}                                 \
else                              \
   return 0;

static int lavrec_output_video_frame(lavrec_t *info, uint8_t *buff, long size, long count)
{
   int n;
   int OpenNewFlag = 0;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if(settings->output_status == 3) return 1; /* Only audio is still active */

   /* Check space on filesystem if we have filled it up
    * or if we have written more than CHECK_INTERVAL bytes since last check
    */
   if (settings->output_status > 0)
   {
      n = (settings->bytes_output_cur - settings->bytes_last_checked)>>20; /* in MBytes */
      if( n > CHECK_INTERVAL || n > settings->MBytes_fs_free - MIN_MBYTES_FREE )
         settings->MBytes_fs_free = lavrec_get_free_space(settings);
   }

   /* Check if it is time to exit */
   if (settings->state == LAVREC_STATE_STOP) 
      lavrec_msg(LAVREC_MSG_INFO, info, "Signal caught, stopping recording");
   if (settings->stats->num_frames * settings->spvf > info->record_time && info->record_time >= 0)
   {
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Recording time reached, stopping");
      lavrec_change_state(info, LAVREC_STATE_STOP);
   }

   /* Check if we have to open a new output file */
   if (settings->output_status > 0 && (settings->bytes_output_cur>>20) > info->max_file_size_mb)
   {
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Max filesize reached, opening next output file");
      OpenNewFlag = 1;
   }
   if( info->max_file_frames > 0 &&  settings->stats->num_frames % info->max_file_frames == 0)
   {
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Max number of frames reached, opening next output file");
      OpenNewFlag = 1;
   }
   if (settings->output_status > 0 && settings->MBytes_fs_free < MIN_MBYTES_FREE)
   {
      lavrec_msg(LAVREC_MSG_INFO, info,
         "File system is nearly full, trying to open next output file");
      OpenNewFlag = 1;
   }

   /* JPEG = always open new file */
   if (info->video_format == 'j')
     OpenNewFlag = 1;

   /* If a file is open and we should open a new one or exit, close current file */
   if (settings->output_status > 0 && (OpenNewFlag || settings->state == LAVREC_STATE_STOP))
   {
      if (info->audio_size)
      {
         /* Audio is running - flag that the old file should be closed */
         if(settings->output_status != 1)
         {
            /* There happened something bad - the old output file from the
             * last file change is not closed. We try to close all files and exit
             */
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Audio too far behind video. Check if audio works correctly!");
            lavrec_close_files_on_error(info);
            return -1;
         }
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Closing current output file for video, waiting for audio to be filled");
         settings->video_file_old = settings->video_file;
         settings->video_file = NULL;
         settings->num_frames_old = settings->stats->num_frames;
         if (settings->state == LAVREC_STATE_STOP)
         {
            settings->output_status = 3;
            return 1;
         }
         else
            settings->output_status = 2;
      }
      else
      {
         if (settings->video_file)
         {
            if (lav_close(settings->video_file))
            {
               settings->video_file = NULL;
               lavrec_msg(LAVREC_MSG_ERROR, info,
                  "Error closing video output file %s, may be unuseable due to error",
                  settings->stats->output_filename);
               return 0;
            }
            settings->video_file = NULL;
         }
         if (settings->state == LAVREC_STATE_STOP) return 0;
      }
   }

   /* Open new output file if needed */
   if (settings->output_status==0 || OpenNewFlag )
   {
      /* Get next filename */
      if (info->num_files == 0)
      {
         sprintf(settings->stats->output_filename, info->files[0],
            ++settings->stats->current_output_file);
      }
      else
      {
         if (settings->stats->current_output_file >= info->num_files)
         {
            if (info->video_format == 'j')
            {
               settings->stats->current_output_file = 0;
            }
            else
            {
               lavrec_msg(LAVREC_MSG_WARNING, info,
                  "Number of given output files reached");
               OUTPUT_VIDEO_ERROR_RETURN;
            }
         }
         strncpy(settings->stats->output_filename,
            info->files[settings->stats->current_output_file++],
            sizeof(settings->stats->output_filename));
      }
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Opening output file %s", settings->stats->output_filename);
         
      /* Open next file */
      settings->video_file = lav_open_output_file(settings->stats->output_filename, info->video_format,
         settings->width, settings->height, settings->interlaced,
         (info->video_norm==1? 30000.0/1001.0 : 25.0),
         info->audio_size, (info->stereo ? 2 : 1), info->audio_rate);
      if (!settings->video_file)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error opening output file %s: %s", settings->stats->output_filename, lav_strerror());
         OUTPUT_VIDEO_ERROR_RETURN;
      }

      if (settings->output_status == 0) settings->output_status = 1;

      /* Check space on filesystem. Exit if not enough space */
      settings->bytes_output_cur = 0;
      settings->MBytes_fs_free = lavrec_get_free_space(settings);
      if(settings->MBytes_fs_free < MIN_MBYTES_FREE_OPEN)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Not enough space for opening new output file");

         /* try to close and remove file, don't care about errors */
         if (settings->video_file)
         {
            lav_close(settings->video_file);
            settings->video_file = NULL;
            remove(settings->stats->output_filename);
         }
         OUTPUT_VIDEO_ERROR_RETURN;
      }
   }

   /* Output the frame count times */
   if (lav_write_frame(settings->video_file,buff,size,count))
   {
      /* If an error happened, try to close output files and exit */
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error writing to output file %s: %s", settings->stats->output_filename, lav_strerror());
      lavrec_close_files_on_error(info);
      return 0;
   }

   /* Update counters. Maybe frame its written only once,
    * but size*count is the save guess
    */
   settings->bytes_output_cur += size*count; 
   settings->stats->num_frames += count;

   /*
	* If the user has specified flushing of file buffers
	* flush every time the specified number of unflushed frames has
	* been reached.
	*/

   if( info->flush_count > 0 &&  settings->stats->num_frames % info->flush_count == 0)
   {
	   int fd = lav_fileno( settings->video_file );
	   if( fd >= 0 )
		   fdatasync(fd);
   }
   return 1;
}

static int video_captured(lavrec_t *info, uint8_t *buff, long size, long count)
{
   if (info->files)
      return lavrec_output_video_frame(info, buff, size, count);
   else
      info->video_captured(buff, size, count);
   return 1;
}


/******************************************************
 * lavrec_output_audio_to_file()
 *   writes audio data to a file
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_output_audio_to_file(lavrec_t *info, uint8_t *buff, long samps, int old)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if(samps==0) return 1;

   /* Output data */
   if (lav_write_audio(old?settings->video_file_old:settings->video_file,buff,samps))
   {
      /* If an error happened, try to close output files and exit */
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error writing to output file: %s", lav_strerror());
      lavrec_close_files_on_error(info);
      return 0;
   }

   /* update counters */
   settings->stats->num_asamps += samps;
   if (!old) settings->bytes_output_cur += samps * settings->audio_bps;

   return 1;
}


/******************************************************
 * lavrec_output_audio_samples()
 *   outputs audio samples to files
 *
 * return value: 1 on success, 0 or -1 on error
 ******************************************************/

static int lavrec_output_audio_samples(lavrec_t *info, uint8_t *buff, long samps)
{
   long diff = 0;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   /* Safety first */
   if(!settings->output_status)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "**INTERNAL ERROR: Output audio but no file open");
      return -1;
   }

   if(settings->output_status<2)
   {
      /* Normal mode, just output the sample */
      return lavrec_output_audio_to_file(info, buff, samps, 0);
   }

   /* if we come here, we have to fill up the old file first */
   diff = (settings->num_frames_old * settings->spvf -
      settings->stats->num_asamps * settings->spas) * info->audio_rate;
   
   if(diff<0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "**INTERNAL ERROR: Audio output ahead video output");
      return -1;
   }

   if(diff >= samps)
   {
      /* All goes to old file */
      return lavrec_output_audio_to_file(info, buff, samps, 1);
   }

   /* diff samples go to old file */
   if (!lavrec_output_audio_to_file(info, buff, diff, 1))
      return 0;

   /* close old file */
   lavrec_msg(LAVREC_MSG_DEBUG, info, "Audio is filled - closing old file");
   if (settings->video_file_old)
   {
      if (lav_close(settings->video_file_old))
      {
         settings->video_file_old = NULL;
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error closing video output file, may be unuseable due to error: %s",
            lav_strerror());
         return 0;
      }
      settings->video_file_old = NULL;
   }

   /* Check if we are ready */
   if (settings->output_status==3) return 0;

   /* remaining samples go to new file */
   settings->output_status = 1;
   return lavrec_output_audio_to_file(info, buff+diff*settings->audio_bps, samps-diff, 0);
}

static int audio_captured(lavrec_t *info, uint8_t *buff, long samps)
{
   if (info->files)
      return lavrec_output_audio_samples(info, buff, samps);
   else
      info->audio_captured(buff, samps);
   return 1;
}

static void frame_YUV422_to_planar_42x(uint8_t *output, uint8_t *input,
                                       int width, int height, int chroma)
{
    int i, j, w2;
    uint8_t *y, *cb, *cr;

    w2 = width/2;
    y=output;
    cb=(output + width*height);
    cr=(output + (3*width*height)/2);

    for (i=0; i<height;) {
        /* process two scanlines (one from each field, interleaved) */
        /* ...top-field scanline */
        for (j=0; j<w2; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);
            *(y++) =  *(input++);
            *(cr++) = *(input++);
        }
        i++;
        /* ...bottom-field scanline */
        for (j=0; j<w2; j++) {
            /* packed YUV 422 is: Y[i] U[i] Y[i+1] V[i] */
            *(y++) =  *(input++);
            *(cb++) = *(input++);
            *(y++) =  *(input++);
            *(cr++) = *(input++);
        }
        i++;
        if (chroma == Y4M_CHROMA_422)
          continue;
        /* process next two scanlines (one from each field, interleaved) */
        /* ...top-field scanline */
        for (j=0; j<w2; j++) {
          /* skip every second line for U and V */
          *(y++) = *(input++);
          input++;
          *(y++) = *(input++);
          input++;
        }
        i++;
        /* ...bottom-field scanline */
        for (j=0; j<w2; j++) {
          /* skip every second line for U and V */
          *(y++) = *(input++);
          input++;
          *(y++) = *(input++);
          input++;
        }
        i++;
    }
}

/******************************************************
 * lavrec_encoding_thread()
 *   The software encoding thread
 ******************************************************/

static void *lavrec_encoding_thread(void* arg)
{
   encoder_info_t *w_info = (encoder_info_t *)arg;
   lavrec_t *info = w_info->info; 
   video_capture_setup *settings = (video_capture_setup *)info->settings;
   struct timeval timestamp[MJPEG_MAX_BUF];
   int jpegsize;
   unsigned long current_frame = w_info->encoder_id;
   unsigned long predecessor_frame;
   uint8_t * source;
   uint8_t * buffer=NULL;

   lavrec_msg(LAVREC_MSG_DEBUG, info,
      "Starting software encoding thread");

   /* Allow easy shutting down by other processes... */
   /* PTHREAD_CANCEL_ASYNCHRONOUS is evil
      pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
      pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
   */

   while (settings->state == LAVREC_STATE_RECORDING)
   {
      pthread_cleanup_push((void (*)(void*))pthread_mutex_unlock, &settings->encoding_mutex);
      pthread_mutex_lock(&(settings->encoding_mutex));
      while (settings->buffer_valid[current_frame] == -1)
      {
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Encoding thread: sleeping for new frames (waiting for frame %ld)", 
            current_frame);
         pthread_cond_wait(&(settings->buffer_filled[current_frame]),
            &(settings->encoding_mutex));
         if (settings->please_stop_syncing) {
            pthread_mutex_unlock(&(settings->encoding_mutex));
            pthread_exit(NULL);
         }
      }
      memcpy(&(timestamp[current_frame]), &(settings->bsync.timestamp), sizeof(struct timeval));

      if (settings->buffer_valid[current_frame] > 0)
      {
	 /* There is no cancellation point in this block, but just to make sure... */
	 pthread_cleanup_push((void (*)(void*))pthread_mutex_lock, &settings->encoding_mutex);
         pthread_mutex_unlock(&(settings->encoding_mutex));

         if( settings->YUVP_convert )
         {
              memcpy( settings->YUVP_convert[current_frame].YUVP_buff, settings->YUVP_convert[current_frame].mmap, (info->geometry->h * info->geometry->w * 2) );
              source=settings->YUV_buff + (info->geometry->h * info->geometry->w * 2)*current_frame;
              frame_YUV422_to_planar_42x( source, settings->YUVP_convert[current_frame].YUVP_buff, info->geometry->w, info->geometry->h, Y4M_CHROMA_422 );
         }
         else
         {
              source=settings->YUV_buff+settings->softreq.offsets[current_frame];
         }

         jpegsize = encode_jpeg_raw(settings->MJPG_buff+current_frame*settings->breq.size,
            settings->breq.size, info->quality, settings->interlaced,
            Y4M_CHROMA_422, info->geometry->w, info->geometry->h,
            source,
            source+(info->geometry->w*info->geometry->h),
            source+(info->geometry->w*info->geometry->h*3/2));

         if (jpegsize<0)
         {
            if( buffer != NULL )
            {
                 free( buffer );
                 buffer=NULL;
            }
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error encoding frame to JPEG");
            lavrec_change_state(info, LAVREC_STATE_STOP);
	    pthread_exit(0);
         }

         pthread_cleanup_pop(1);
      }
      else
      {
	 jpegsize = 0;		/* Just toss the frame */
      }
      
      
      /* Writing of video and audio data is non-reentrant and must
       * occur in-order - acquire lock and wait for preceding
       * frame's encoder to have completed writing that frames data
       *
       * Note that we need to queue the buffers in order, too,
       * so we need to sync up here even if we're discarding
       * the frame.
       */
      predecessor_frame = ( (current_frame + settings->softreq.frames-1)
			    % settings->softreq.frames );

      while( !settings->buffer_completed[predecessor_frame] )
      {
	 pthread_cond_wait(&(settings->buffer_completion[predecessor_frame]),
			   &(settings->encoding_mutex));
      }

      if (jpegsize > 0)
      {
         if (video_captured(info,
			    settings->MJPG_buff+(settings->breq.size*current_frame),
			    jpegsize,
			    settings->buffer_valid[current_frame]) != 1)
         {
            if( buffer != NULL )
            {
                 free( buffer );
                 buffer=NULL;
            }
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error writing the frame");
            lavrec_change_state(info, LAVREC_STATE_STOP);
            pthread_exit(0);
         }
      }

#if 0
      if (!lavrec_queue_buffer(info, &current_frame))
      {
         if( buffer != NULL )
         {
              free( buffer );
              buffer=NULL;
         }
         if (info->files)
            lavrec_close_files_on_error(info);
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error re-queuing buffer: %s", strerror(errno));
         lavrec_change_state(info, LAVREC_STATE_STOP);
         pthread_exit(0);
      }
      /* Mark the capture buffer as once again as in progress for capture */
      settings->buffer_valid[current_frame] = -1;
#endif
      /* hack for BTTV-0.8 - give it a status that tells us to queue it in another thread */
      settings->buffer_valid[current_frame] = -2;

      if (!lavrec_handle_audio(info, &(timestamp[current_frame])))
         lavrec_change_state(info, LAVREC_STATE_STOP);

      /* Mark this frame as having completed compression and writing,
       * signal any encoders waiting for this completion so they can write
       * out their own results, and release lock.
       */
      settings->buffer_completed[current_frame] = 1;
      pthread_cond_broadcast(&(settings->buffer_completion[current_frame]));

      current_frame = (current_frame+w_info->num_encoders)%settings->softreq.frames;
      pthread_cleanup_pop(1);
   }

   if( buffer != NULL )
   {
      free( buffer );
      buffer=NULL;
   }
   pthread_exit(NULL);
   return(NULL);
}

int get_size_offset( int fd, size_t *length, off_t *offset, unsigned int frame )
{
	struct v4l2_buffer	buf;
	int			retval;
	
	buf.index=frame;
	buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	retval=ioctl( fd, VIDIOC_QUERYBUF, &buf );
	if( retval != 0 )
		return retval;

	*length=buf.length;
	*offset=buf.m.offset;
	return 0;
}

int set_format_part1( int fd, uint16_t width, uint16_t height )
{
	struct v4l2_format	format;
	int			retval;

	memset( &format, 0, sizeof( format ) );

	format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	retval=ioctl( fd, VIDIOC_G_FMT, &format );
	if( retval != 0 )
		return retval;

	format.fmt.pix.width=width;
	format.fmt.pix.height=height;
	format.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;
	format.fmt.pix.field=V4L2_FIELD_ANY;
	format.fmt.pix.bytesperline=0;
	retval=ioctl( fd, VIDIOC_S_FMT, &format );
	if( retval != 0 )
		return retval;

	return 0;
}

int set_format_part2( int fd, unsigned int frame )
{
	struct v4l2_buffer	buf;
	int			retval;
	enum v4l2_buf_type	captype=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	 
	memset( &buf, 0, sizeof( buf ) );
	buf.index=frame;
	buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	retval=ioctl( fd, VIDIOC_QUERYBUF, &buf );
	if( retval != 0 )
		return retval;

        retval=ioctl(fd, VIDIOC_QBUF, &buf);
        if( retval != 0 )
            return retval;

	retval=ioctl( fd, VIDIOC_STREAMON, &captype );
	if( retval != 0 )
		return retval;

	return 0;
}

/******************************************************
 * lavrec_software_init()
 *   Some software-MJPEG encoding specific initialization
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_software_init(lavrec_t *info)
{
   struct video_capability vc;
   int i;
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if (ioctl(settings->video_fd, VIDIOCGCAP, &vc) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error getting device capabilities: %s", strerror(errno));
      return 0;
   }
   /* vc.maxwidth is often reported wrong - let's just keep it broken (sigh) */
   /*if (vc.maxwidth != 768 && vc.maxwidth != 640) vc.maxwidth = 720;*/

   /* set some "subcapture" options - cropping is done later on (during capture) */
   if(!info->geometry->w)
      info->geometry->w = ((vc.maxwidth==720&&info->horizontal_decimation!=1)?704:vc.maxwidth)/4;
   if(!info->geometry->h)
      info->geometry->h = (info->video_norm==1 ? 480 : 576)/4;

   if (info->geometry->w > vc.maxwidth)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Image width (%d) bigger than maximum (%d)!",
         info->geometry->w, vc.maxwidth);
      return 0;
   }
   if ((info->geometry->w%16)!=0) 
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Image width (%d) not multiple of 16 (required for JPEG encoding)!",
	 info->geometry->w);
      return 0;
   }
   if (info->geometry->h > (info->video_norm==1 ? 480 : 576)) 
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Image height (%d) bigger than maximum (%d)!",
         info->geometry->h, (info->video_norm==1 ? 480 : 576));
      return 0;
   }

   /* RJ: Image height must only be a multiple of 8, but geom_height
    * is double the field height
    */
   if ((info->geometry->h%16)!=0) 
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Image height (%d) not multiple of 16 (required for JPEG encoding)!",
         info->geometry->h);
      return 0;
   }

   settings->mm.width = settings->width = info->geometry->w;
   settings->mm.height = settings->height = info->geometry->h;
   settings->mm.format = VIDEO_PALETTE_YUV422P;

   if (info->geometry->h > (info->video_norm==1?320:384))
      settings->interlaced = Y4M_ILACE_TOP_FIRST; /* all interlaced BT8x8 capture seems top-first ?? */
   else
      settings->interlaced = Y4M_ILACE_NONE;

   lavrec_msg(LAVREC_MSG_INFO, info,
      "Image size will be %dx%d, %d field(s) per buffer",
      info->geometry->w, info->geometry->h,
      (settings->interlaced == Y4M_ILACE_NONE)?1:2);

   /* request buffer info */
   if (ioctl(settings->video_fd, VIDIOCGMBUF, &(settings->softreq)) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error getting buffer information: %s", strerror(errno));
      return 0;
   }
   if (settings->softreq.frames < MIN_QUEUES_NEEDED)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "We need at least %d buffers, but we only got %d",
         MIN_QUEUES_NEEDED, settings->softreq.frames);
      return 0;
   }
   lavrec_msg(LAVREC_MSG_INFO, info,
      "Got %d YUV-buffers of size %d KB", settings->softreq.frames,
      settings->softreq.size/(1024*settings->softreq.frames));

   if( info->software_encoding == 2 )
   {
       int   loop;
       size_t size;
       off_t  offset;
       settings->YUVP_convert=(struct YUVP_convert *) malloc( settings->softreq.frames * sizeof( struct YUVP_convert ) );
       if( settings->YUVP_convert == NULL )
       {
           lavrec_msg (LAVREC_MSG_ERROR, info,
              "Malloc error, you\'re probably out of memory");
           return 0;
       }
       settings->YUVP_convert[0].YUVP_buff=(unsigned char *) malloc( settings->mm.width * settings->mm.height * 2 * settings->softreq.frames );
       if( settings->YUVP_convert[0].YUVP_buff == NULL )
       {
           lavrec_msg (LAVREC_MSG_ERROR, info,
              "Malloc error for temp buffers, you\'re probably out of memory");
           free( settings->YUVP_convert );
           return 0;
       }

       for( loop=1; loop < settings->softreq.frames; loop++ )
           settings->YUVP_convert[loop].YUVP_buff=settings->YUVP_convert[0].YUVP_buff + (loop * settings->mm.width * settings->mm.height * 2);

       if( set_format_part1( settings->video_fd, settings->mm.width, settings->mm.height ) )
       {
           free( settings->YUVP_convert[0].YUVP_buff );
           free( settings->YUVP_convert );
           lavrec_msg( LAVREC_MSG_ERROR, info, "ioctl error on set_format_part1.\n" );
           return 0;
       }

       for( loop=0; loop < settings->softreq.frames; loop++ )
       {
          if( get_size_offset( settings->video_fd, &size, &offset, loop ) )
          {
              lavrec_msg( LAVREC_MSG_ERROR, info, "Can't get mmap settings" );
              free( settings->YUVP_convert[0].YUVP_buff );
              free( settings->YUVP_convert );
              return 0;
          }
          settings->YUVP_convert[loop].mmap=mmap( 0, size, PROT_READ|PROT_WRITE, MAP_SHARED, settings->video_fd,
                                                offset );
          if( settings->YUVP_convert[loop].mmap == NULL )
          {
              lavrec_msg (LAVREC_MSG_ERROR, info,
                 "Packed YUV mmap error");
              free( settings->YUVP_convert[0].YUVP_buff );
              free( settings->YUVP_convert );
              return 0;
          }
       }

       settings->YUV_buff=(uint8_t *) malloc( settings->softreq.size * info->num_encoders );
       if( settings->YUV_buff == NULL )
       {
           free( settings->YUVP_convert[0].YUVP_buff );
           free( settings->YUVP_convert );
           lavrec_msg( LAVREC_MSG_ERROR, info, "malloc error on YUV_buff.\n" );
           return 0;
       }
   }
   else
   {
       settings->YUVP_convert=NULL;
       /* Map the buffers */
       settings->YUV_buff = mmap(0, settings->softreq.size, 
          PROT_READ|PROT_WRITE, MAP_SHARED, settings->video_fd, 0);
       if (settings->YUV_buff == MAP_FAILED)
       {
          lavrec_msg(LAVREC_MSG_ERROR, info,
             "Error mapping video buffers: %s", strerror(errno));
          return 0;
       }
   }

   /* set up buffers for software encoding thread */
   if (info->MJPG_numbufs > MJPEG_MAX_BUF)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Too many buffers (%d) requested, maximum is %d",
         info->MJPG_numbufs, MJPEG_MAX_BUF);
      return 0;
   }

   /* Check number of JPEG compression worker threads is consistent with
    * with the number of buffers available
    */
   if (info->num_encoders > info->MJPG_numbufs-1 )
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "More encoding workers (%d) than number of buffers-1 (%d)",
         info->num_encoders,
         info->MJPG_numbufs-1);
      return 0;
   }
   settings->breq.count = info->MJPG_numbufs;
   settings->breq.size = info->MJPG_bufsize*1024;
   settings->MJPG_buff = (uint8_t *) malloc(sizeof(uint8_t)*settings->breq.size*settings->breq.count);
   if (!settings->MJPG_buff)
   {
      if( settings->YUVP_convert )
      {
        free( settings->YUVP_convert[0].YUVP_buff );
	free( settings->YUVP_convert );
      }
      lavrec_msg (LAVREC_MSG_ERROR, info,
         "Malloc error, you\'re probably out of memory");
      return 0;
   }
   lavrec_msg(LAVREC_MSG_INFO, info,
      "Created %ld MJPEG-buffers of size %ld KB",
      settings->breq.count, settings->breq.size/1024);

   /* set up software JPEG-encoding thread */
   pthread_mutex_init(&(settings->encoding_mutex), NULL);
   for (i=0;i<MJPEG_MAX_BUF;i++)
   {
      pthread_cond_init(&(settings->buffer_filled[i]), NULL);
      pthread_cond_init(&(settings->buffer_completion[i]), NULL);
   }

   /* queue setup */
   pthread_mutex_init(&(settings->queue_mutex), NULL);
   pthread_cond_init(&(settings->queue_wait), NULL);

   return 1;
}


/******************************************************
 * lavrec_hardware_init()
 *   Some hardware-MJPEG encoding specific initialization
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_hardware_init(lavrec_t *info)
{
   struct video_capability vc;
   struct mjpeg_params bparm;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if (ioctl(settings->video_fd, VIDIOCGCAP, &vc) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error getting device capabilities: %s", strerror(errno));
      return 0;
   }
   /* vc.maxwidth is often reported wrong - let's just keep it broken (sigh) */
   if (vc.maxwidth != 768 && vc.maxwidth != 640) vc.maxwidth = 720;

   /* Query and set params for capture */
   if (ioctl(settings->video_fd, MJPIOC_G_PARAMS, &bparm) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error getting video parameters: %s", strerror(errno));
      return 0;
   }
   bparm.input = info->video_src;
   bparm.norm = info->video_norm;
   bparm.quality = info->quality;

   /* Set decimation and image geometry params - only if we have weird options */
   if (info->geometry->x != VALUE_NOT_FILLED ||
      info->geometry->y != VALUE_NOT_FILLED ||
      (info->geometry->h != 0 && info->geometry->h != (info->video_norm==1 ? 480 : 576)) ||
      (info->geometry->w != 0 && info->geometry->w != vc.maxwidth) ||
      info->horizontal_decimation != info->vertical_decimation)
   {
      bparm.decimation = 0;
      if(!info->geometry->w) info->geometry->w = ((vc.maxwidth==720&&info->horizontal_decimation!=1)?704:vc.maxwidth);
      if(!info->geometry->h) info->geometry->h = info->video_norm==1 ? 480 : 576;
      bparm.HorDcm = info->horizontal_decimation;
      bparm.VerDcm = (info->vertical_decimation==4) ? 2 : 1;
      bparm.TmpDcm = (info->vertical_decimation==1) ? 1 : 2;
      bparm.field_per_buff = (info->vertical_decimation==1) ? 2 : 1;

      bparm.img_width  = info->geometry->w;
      bparm.img_height = info->geometry->h/2;

      if (info->geometry->x != VALUE_NOT_FILLED)
         bparm.img_x = info->geometry->x;
      else
         bparm.img_x = (vc.maxwidth - bparm.img_width)/2;

      if (info->geometry->y != VALUE_NOT_FILLED)
         bparm.img_y = info->geometry->y/2;
      else
         bparm.img_y = ( (info->video_norm==1 ? 240 : 288) - bparm.img_height)/2;

      if (info->geometry->w + bparm.img_x > vc.maxwidth)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Image width+offset (%d) bigger than maximum (%d)!",
            info->geometry->w + bparm.img_x, vc.maxwidth);
         return 0;
      }
      if ((info->geometry->w%(bparm.HorDcm*16))!=0) 
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Image width (%d) not multiple of %d (required for JPEG)!",
            info->geometry->w, bparm.HorDcm*16);
         return 0;
      }
      if (info->geometry->h + bparm.img_y > (info->video_norm==1 ? 480 : 576)) 
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Image height+offset (%d) bigger than maximum (%d)!",
            info->geometry->h + info->geometry->y,
            (info->video_norm==1 ? 480 : 576));
         return 0;
      }

      /* RJ: Image height must only be a multiple of 8, but geom_height
       * is double the field height
       */
      if ((info->geometry->h%(bparm.VerDcm*16))!=0) 
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Image height (%d) not multiple of %d (required for JPEG)!",
            info->geometry->h, bparm.VerDcm*16);
         return 0;
      }

   }
   else
   {
      bparm.decimation = info->horizontal_decimation;
   }

   /* Care about field polarity and APP Markers which are needed for AVI
    * and Quicktime and may be for other video formats as well
    */
   if(info->vertical_decimation > 1)
   {
      /* for vertical decimation > 1 no known video format needs app markers,
       * we need also not to care about field polarity
       */
      bparm.APP_len = 0; /* No markers */
   }
   else
   {
      int n;
      bparm.APPn = lav_query_APP_marker(info->video_format);
      bparm.APP_len = lav_query_APP_length(info->video_format);

      /* There seems to be some confusion about what is the even and odd field ... */
      /* madmac: 20010810: According to Ronald, this is wrong - changed now to EVEN */
      bparm.odd_even = lav_query_polarity(info->video_format) == Y4M_ILACE_TOP_FIRST;
      for(n=0; n<bparm.APP_len && n<60; n++) bparm.APP_data[n] = 0;
   }

   if (ioctl(settings->video_fd, MJPIOC_S_PARAMS, &bparm) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error setting video parameters: %s", strerror(errno));
      return 0;
   }

   settings->width = bparm.img_width/bparm.HorDcm;
   settings->height = bparm.img_height/bparm.VerDcm*bparm.field_per_buff;
   settings->interlaced = (bparm.field_per_buff>1);

   lavrec_msg(LAVREC_MSG_INFO, info,
      "Image size will be %dx%d, %d field(s) per buffer",
      settings->width, settings->height, bparm.field_per_buff);

   /* Request buffers */
   settings->breq.count = info->MJPG_numbufs;
   settings->breq.size = info->MJPG_bufsize*1024;
   if (ioctl(settings->video_fd, MJPIOC_REQBUFS,&(settings->breq)) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error requesting video buffers: %s", strerror(errno));
      return 0;
   }
   lavrec_msg(LAVREC_MSG_INFO, info,
      "Got %ld buffers of size %ld KB", settings->breq.count, settings->breq.size/1024);

   /* Map the buffers */
   settings->MJPG_buff = mmap(0, settings->breq.count*settings->breq.size, 
      PROT_READ|PROT_WRITE, MAP_SHARED, settings->video_fd, 0);
   if (settings->MJPG_buff == MAP_FAILED)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error mapping video buffers: %s", strerror(errno));
      return 0;
   }

   return 1;
}


/******************************************************
 * lavrec_init()
 *   initialize, open devices and start streaming
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_init(lavrec_t *info)
{
   struct video_channel vch;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   /* are there files to capture to? */
   if (info->files) /* yes */
   {
/*
 * If NO filesize limit was specifically given then allow unlimited size.
 * ODML extensions will handle the AVI files and Quicktime has had 64bit
 * filesizes for a long time
*/
      if (info->max_file_size_mb < 0)
         info->max_file_size_mb = MAX_MBYTES_PER_FILE_64;
      lavrec_msg(LAVREC_MSG_DEBUG, info,
         "Maximum size per file will be %d MB", info->max_file_size_mb);

      if (info->video_captured || info->audio_captured)
      {
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Custom audio-/video-capture functions are being ignored for file-capture");
      }
   }
   else /* no, so we need the custom actions */
   {
      if (!info->video_captured || (!info->audio_captured && info->audio_size))
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "No video files or custom video-/audio-capture functions given");
         return 0;
      }
   }

   /* Special settings for single frame captures */
   if(info->single_frame)
      info->MJPG_numbufs = 4;

   /* time lapse/single frame captures don't want audio */
   if((info->time_lapse > 1 || info->single_frame) && info->audio_size)
   {
      lavrec_msg(LAVREC_MSG_DEBUG, info,
         "Time lapse or single frame capture mode - audio disabled");
      info->audio_size = 0;
   }

   /* set the sound mixer */
   if (info->audio_size && info->audio_level >= 0)
      lavrec_set_mixer(info, 1);

   /* Initialize the audio system if audio is wanted.
    * This involves a fork of the audio task and is done before
    * the video device and the output file is opened
    */
   settings->audio_bps = 0;
   if (info->audio_size)
   {
      if (audio_init(1,info->use_read, info->stereo,info->audio_size,info->audio_rate))
      {
         lavrec_set_mixer(info, 0);
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error initializing Audio: %s",audio_strerror());
         return 0;
      }
      settings->audio_bps = info->audio_size / 8;
      if (info->stereo) settings->audio_bps *= 2;
      settings->audio_buffer_size = audio_get_buffer_size();
   }

   /* back to normal user - only root needed during audio setup */
   if (getuid() != geteuid())
   {
      if (setuid(getuid()) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Failed to set effective user-ID: %s",
            strerror(errno));
         return 0;
      }
   }

   /* open the video device */
   settings->video_fd = open(info->video_dev, O_RDWR);
   if (settings->video_fd < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error opening video-device (%s): %s",
         info->video_dev, strerror(errno));
      return 0;
   }

   /* we might have to autodetect the video-src/norm */
   if (lavrec_autodetect_signal(info) == 0)
      return 0;

   if (info->software_encoding && info->video_src == -1)
      vch.channel = 0;
   else
      vch.channel = info->video_src;
   vch.norm = info->video_norm;
   if (info->video_norm != 3 && info->video_src != -1)
   {
      if (ioctl(settings->video_fd, VIDIOCSCHAN, &vch) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error setting channel: %s", strerror(errno));
         return 0;
      }
   }
   if (ioctl(settings->video_fd, VIDIOCGCHAN, &vch) < 0)
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Error getting channel info: %s", strerror(errno));
      return 0;
   }
   settings->has_audio = (vch.flags & VIDEO_VC_AUDIO);
   info->video_norm = vch.norm; /* the final norm */

   /* set channel if we're tuning */
   if (vch.flags & VIDEO_VC_TUNER && info->tuner_frequency)
   {
      unsigned long outfreq;
      outfreq = info->tuner_frequency*16/1000;
      if (ioctl(settings->video_fd, VIDIOCSFREQ, &outfreq) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error setting tuner frequency: %s", strerror(errno));
         return 0;
      }
   }

   /* Set up tuner audio if this is a tuner. I think this should be done
    * AFTER the tuner device is selected
    */
   if (settings->has_audio) 
   {
      struct video_audio vau;

      /* get current */
      if (ioctl(settings->video_fd,VIDIOCGAUDIO, &vau) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error getting tuner audio params: %s", strerror(errno));
         return 0;
      }
      /* unmute so we get sound to record
       * this is done without checking current state because the
       * current mga driver doesn't report mute state accurately
       */
      lavrec_msg(LAVREC_MSG_INFO, info, "Unmuting tuner audio...");
      vau.flags &= (~VIDEO_AUDIO_MUTE);
      if (ioctl(settings->video_fd,VIDIOCSAUDIO, &vau) < 0)
      {
         lavrec_msg(LAVREC_MSG_INFO, info,
            "Error setting tuner audio params: %s", strerror(errno));
         return 0;
      }
   }

   /* set state to paused... ugly, but we need it for the software thread */
   settings->state = LAVREC_STATE_PAUSED;

   /* set up some hardware/software-specific stuff */
   if (info->software_encoding)
   {
      if (!lavrec_software_init(info)) return 0;
   }
   else
   {
      if (!lavrec_hardware_init(info)) return 0;
   }   

   /* Try to get a reliable timestamp for Audio */
   if (info->audio_size && info->sync_correction > 1)
   {
      int n,res;

      lavrec_msg(LAVREC_MSG_INFO, info, "Getting audio ...");

      for(n=0;;n++)
      {
         if(n > NUM_AUDIO_TRIES)
         {
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Unable to get audio - exiting ....");
            return 0;
         }
         res = audio_read((unsigned char*)settings->AUDIO_buff,AUDIO_BUFFER_SIZE,0,
            &(settings->audio_t0),&(settings->astat));
         if (res < 0)
         {
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error reading audio: %s",audio_strerror());
            return 0;
         }
         if(res && settings->audio_t0.tv_sec ) break;
         usleep(20000);
      }
   }

   /* If we can increase process priority ... no need for R/T though...
    * This is mainly useful for running using "at" which otherwise drops the
    * priority which causes sporadic audio buffer over-runs
    */
   if( getpriority(PRIO_PROCESS, 0) > -5 )
      setpriority(PRIO_PROCESS, 0, -5 );

   /* Seconds per video frame: */
   settings->spvf = (info->video_norm==VIDEO_MODE_NTSC) ? 1001./30000. : 0.040;
   settings->sync_lim = settings->spvf*1.5;

   /* Seconds per audio sample: */
   if(info->audio_size)
      settings->spas = 1.0/info->audio_rate;
   else
      settings->spas = 0.;

   return 1;
}


/******************************************************
 * lavrec_wait_for_start()
 *   catch audio until we have to stop or record
 ******************************************************/

static void lavrec_wait_for_start(lavrec_t *info)
{
   int res;

   video_capture_setup *settings = (video_capture_setup *)info->settings;

   while(settings->state == LAVREC_STATE_PAUSED)
   {
      usleep(10000);

      /* Audio (if on) is allready running, empty buffer to avoid overflow */
      if (info->audio_size)
      {
         while( (res=audio_read((unsigned char*)settings->AUDIO_buff,AUDIO_BUFFER_SIZE,
            0,&settings->audio_t0,&settings->astat)) >0 ) /*noop*/;
         if(res==0) continue;
         if(res<0)
         {
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error reading audio: %s", audio_strerror());
            lavrec_change_state(info, LAVREC_STATE_STOP); /* stop */
            return;
         }
      }
   }
}


/******************************************************
 * lavrec_queue_buffer()
 *   queues a buffer (either MJPEG or YUV)
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_queue_buffer(lavrec_t *info, unsigned long *num)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   lavrec_msg(LAVREC_MSG_DEBUG, info,
      "Queueing frame %lu", *num);
   if (info->software_encoding)
   {
      settings->mm.frame = *num;
      pthread_mutex_lock(&(settings->queue_mutex));
      if (settings->is_queued[*num] < 0)
      {
         pthread_mutex_unlock(&(settings->queue_mutex));
         return 1;
      }
      pthread_mutex_unlock(&(settings->queue_mutex));

      if( info->software_encoding == 2 )
      {
          if( set_format_part2( settings->video_fd, *num ) )
              return 0;
      }
      else
      {
          if (ioctl(settings->video_fd, VIDIOCMCAPTURE, &(settings->mm)) < 0)
             return 0;
      }

      pthread_mutex_lock(&(settings->queue_mutex));
      settings->queue_left++;
      settings->is_queued[*num] = 1;
      settings->buffers_queued++;
      pthread_cond_broadcast(&(settings->queue_wait));
      pthread_mutex_unlock(&(settings->queue_mutex));
   }
   else
   {
      if (ioctl(settings->video_fd, MJPIOC_QBUF_CAPT, num) < 0)
         return 0;
   }

   return 1;
}


/******************************************************
 * lavrec_software_sync_thread ()
 *   software syncing to get correct timestamps
 ******************************************************/

static void *lavrec_software_sync_thread(void* arg)
{
   lavrec_t *info = (lavrec_t *) arg;
   video_capture_setup *settings = (video_capture_setup *)info->settings;
   int frame = 0; /* framenum to sync on */
#if 1
   unsigned long qframe, i;
#endif

   /* Allow easy shutting down by other processes... */
   /* PTHREAD_CANCEL_ASYNCHRONOUS is evil
      pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
      pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
   */
   /* FIXME: is the right?  Or can we just stop.
    * Don't allow cancellation.  We need to shutdown in an orderly
    * fashion (by noticing that settings->state has changed, to make
    * sure we dequeue all queued buffers.
    */
   pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, NULL );

   while (1)
   {
      /* evil hack for BTTV-0.8 - we need to queue frames here */
      /* this cycle is non-onbligatory - we just queue frames as they become available,
       * below, we'll wait for queues if we don't have enough of them */
      for (i=0;i<settings->softreq.frames;i++)
      {
         qframe = settings->buffers_queued % settings->softreq.frames;
         if (settings->buffer_valid[qframe] == -2)
         {
            if (!lavrec_queue_buffer(info, &qframe))
            {
               pthread_mutex_lock(&(settings->software_sync_mutex));
               settings->software_sync_ready[qframe] = -1;
               pthread_cond_broadcast(&(settings->software_sync_wait[qframe]));
               pthread_mutex_unlock(&(settings->software_sync_mutex));
               lavrec_msg(LAVREC_MSG_ERROR, info,
                  "Error re-queueing a buffer (%lu): %s", qframe, strerror(errno));
               lavrec_change_state(info, LAVREC_STATE_STOP);
               pthread_exit(0);
            }
            settings->buffer_valid[qframe] = -1;
         }
         else
            break;
      }

      pthread_mutex_lock(&(settings->encoding_mutex));
      while (settings->queue_left < MIN_QUEUES_NEEDED)
      {
	 if (settings->is_queued[frame] <= 0 ||
             settings->please_stop_syncing)
            break; /* sync on all remaining frames */
#if 0
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Software sync thread: sleeping for new queues (%d)", frame);
         pthread_cond_wait(&(settings->queue_wait),
            &(settings->queue_mutex));
#else
         /* sleep for new buffers to be completed encoding. After that,
          * requeue them so we have more than MIN_QUEUES_NEEDED buffers
          * free */
         qframe = settings->buffers_queued % settings->softreq.frames;
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Software sync thread: sleeping for new queues (%lu) to become available", qframe);
         while (settings->buffer_valid[qframe] != -2)
         {
            pthread_cond_wait(&(settings->buffer_completion[qframe]),
               &(settings->encoding_mutex));
            if (settings->please_stop_syncing) {
               pthread_mutex_unlock(&(settings->encoding_mutex));
               pthread_exit(0);
            }
         }
         if (!lavrec_queue_buffer(info, &qframe))
         {
            pthread_mutex_unlock(&(settings->encoding_mutex));
            pthread_mutex_lock(&(settings->software_sync_mutex));
            settings->software_sync_ready[qframe] = -1;
            pthread_cond_broadcast(&(settings->software_sync_wait[qframe]));
            pthread_mutex_unlock(&(settings->software_sync_mutex));
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error re-queueing a buffer (%lu): %s", qframe, strerror(errno));
            lavrec_change_state(info, LAVREC_STATE_STOP);
            pthread_exit(0);
         }
         settings->buffer_valid[qframe] = -1;
#endif
      }

      if (!settings->queue_left)
      {
	 lavrec_msg(LAVREC_MSG_DEBUG, info,
		    "Software sync thread stopped");
	 pthread_mutex_unlock(&settings->encoding_mutex);
	 pthread_exit(NULL);
      }
      pthread_mutex_unlock(&settings->encoding_mutex);
      
retry:
      if (ioctl(settings->video_fd, VIDIOCSYNC, &frame) < 0)
      {
         if( errno==EINTR && info->software_encoding )
		goto retry; /* BTTV sync got interrupted */
         pthread_mutex_lock(&(settings->software_sync_mutex));
         settings->software_sync_ready[frame] = -1;
         pthread_cond_broadcast(&(settings->software_sync_wait[frame]));
         pthread_mutex_unlock(&(settings->software_sync_mutex));
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error syncing on a buffer: %s", strerror(errno));
         lavrec_change_state(info, LAVREC_STATE_STOP);
         pthread_exit(0);
      }
      else
      {
         pthread_mutex_lock(&(settings->software_sync_mutex));
         gettimeofday(&(settings->software_sync_timestamp[frame]), NULL);
         settings->software_sync_ready[frame] = 1;
         pthread_cond_broadcast(&(settings->software_sync_wait[frame]));
         pthread_mutex_unlock(&(settings->software_sync_mutex));
      }

      pthread_mutex_lock(&(settings->queue_mutex));
      settings->queue_left--;
      settings->is_queued[frame] = 0;
      pthread_mutex_unlock(&(settings->queue_mutex));

      frame = (frame+1)%settings->softreq.frames;
   }
   return NULL;
}


/******************************************************
 * lavrec_sync_buffer()
 *   sync on a buffer (either MJPIOC_SYNC or VIDIOCSYNC)
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_sync_buffer(lavrec_t *info, struct mjpeg_sync *bsync)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if (info->software_encoding)
   {
      bsync->frame = (bsync->frame+1)%settings->softreq.frames;
      bsync->seq++;
      pthread_mutex_lock(&(settings->software_sync_mutex));
      while (settings->software_sync_ready[bsync->frame] == 0)
      {
         lavrec_msg(LAVREC_MSG_DEBUG, info,
            "Software sync client: sleeping for new frames (waiting for frame %ld)", 
            bsync->frame);
         pthread_cond_wait(&(settings->software_sync_wait[bsync->frame]),
            &(settings->software_sync_mutex));
      }
      pthread_mutex_unlock(&(settings->software_sync_mutex));
      if (settings->software_sync_ready[bsync->frame] < 0)
      {
         return 0;
      }
      memcpy(&(bsync->timestamp), &(settings->software_sync_timestamp[bsync->frame]),
         sizeof(struct timeval));
      settings->software_sync_ready[bsync->frame] = 0;
   }
   else
   {
      if (ioctl(settings->video_fd, MJPIOC_SYNC, bsync) < 0)
      {
         return 0;
      }
   }
   lavrec_msg(LAVREC_MSG_DEBUG, info,
      "Syncing on frame %ld", bsync->frame);

   return 1;
}


/******************************************************
 * lavrec_handle_audio()
 *   handle audio and output stats
 *
 * return value: 1 on success, 0 on error
 ******************************************************/

static int lavrec_handle_audio(lavrec_t *info, struct timeval *timestamp)
{
   int x;
   int nerr = 0;
   video_capture_setup *settings = (video_capture_setup *)info->settings;
   video_capture_stats *stats = settings->stats;

   while (info->audio_size)
   {
      /* Only try to read a audio sample if video is ahead - else we might
       * get into difficulties when writing the last samples
       */
      if (settings->output_status < 3 && 
         stats->num_frames * settings->spvf <
         (stats->num_asamps + settings->audio_buffer_size /
         settings->audio_bps) * settings->spas)
         break;

      x = audio_read((unsigned char*)settings->AUDIO_buff, sizeof(settings->AUDIO_buff),
         0, &(settings->audio_tmstmp), &(settings->astat));

      if (x == 0) break;
      if (x < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error reading audio: %s", audio_strerror());
         if (info->files)
            lavrec_close_files_on_error(info);
         nerr++;
         break;
      }

      if (!(settings->astat))
      {
         stats->num_aerr++;
         stats->stats_changed = 1;
      }

      /* Adjust for difference at start */
      if (settings->audio_offset >= x)
      {
         settings->audio_offset -= x;
         continue;
      }
      x -= settings->audio_offset;

      /* Got an audio sample, write it out */
      if (audio_captured(info, settings->AUDIO_buff+settings->audio_offset,
         x/settings->audio_bps) != 1)
      {
         nerr++;
         break; /* Done or error occured */
      }
      settings->audio_offset = 0;

      /* calculate time differences beetween audio and video
       * tdiff1 is the difference according to the number of frames/samples written
       * tdiff2 is the difference according to the timestamps
       * (only if audio timestamp is not zero)
       */
      if(settings->audio_tmstmp.tv_sec)
      {
         stats->tdiff1 = stats->num_frames * settings->spvf - stats->num_asamps * settings->spas;
         stats->tdiff2 = (timestamp->tv_sec - settings->audio_tmstmp.tv_sec)
            + (timestamp->tv_usec - settings->audio_tmstmp.tv_usec) * 1.e-6;
      }
   }

   /* output_statistics */
   if (info->output_statistics) info->output_statistics(stats);
   stats->stats_changed = 0;

   stats->prev_sync = stats->cur_sync;

   if (nerr) return 0;
   return 1;
}


/******************************************************
 * lavrec_record()
 *   record and process video and audio
 ******************************************************/

static void lavrec_record(lavrec_t *info)
{
   unsigned long frame_cnt;
   int x, write_frame, nerr, nfout;
   video_capture_stats stats;
   unsigned int first_lost;
   double time;
   struct timeval first_time;

   video_capture_setup *settings = (video_capture_setup *)info->settings;
   settings->stats = &stats;
   settings->queue_left = 0;

   /* basically, this could be done on init, but we need to
    * reset some variables when going from pause to play and
    * the other way around, so we need to restart it when we
    * enter he playing state
    */
   if (info->software_encoding)
   {
      for (x=0;x<MJPEG_MAX_BUF;x++)
      {
         settings->is_queued[x] = 0;
         settings->buffer_valid[x] = -1; /* 0 means to just omit the frame, -1 means "in progress",
						-2 is an evil hack for BTTV-0.8 */
         settings->buffer_completed[x] = 1; /* 1 means compression and writing completed,
                                               0 means in progress */
      }
      settings->buffers_queued = 0;

      if( !(settings->encoders = malloc(info->num_encoders * sizeof(encoder_info_t))) )
      {
	 lavrec_msg (LAVREC_MSG_ERROR, info,
	    "Malloc error, you\'re probably out of memory");
	 return;
      }
      for (x=0; x < info->num_encoders; ++x )
      {
         settings->encoders[x].info = info;
         settings->encoders[x].encoder_id = x;
         settings->encoders[x].num_encoders = info->num_encoders;

         if ( pthread_create( &(settings->encoders[x].thread), NULL,
            lavrec_encoding_thread, (void *) &settings->encoders[x] ) )
         {
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Failed to create software encoding thread");
            lavrec_change_state(info, LAVREC_STATE_STOP);
            return;
         }
      }
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Created %d software JPEG-encoding process(es)\n",
         info->num_encoders);
      settings->audio_offset = 0;
   }

   /* Queue all buffers, this also starts streaming capture */
   for (frame_cnt=0; 
		frame_cnt<(info->software_encoding?settings->softreq.frames:settings->breq.count);
		frame_cnt++)
   {
      if (!lavrec_queue_buffer(info, &frame_cnt))
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error queuing buffers: %s", strerror(errno));
         lavrec_change_state(info, LAVREC_STATE_STOP);
         return;
      }
   }

   /* if we're doing software-encoding, start up the software-sync thread */
   if (info->software_encoding)
   {
      pthread_mutex_init(&(settings->software_sync_mutex), NULL);
      for (x=0;x<MJPEG_MAX_BUF;x++)
      {
         settings->software_sync_ready[x] = 0;
         pthread_cond_init(&(settings->software_sync_wait[x]), NULL);
      }
      settings->please_stop_syncing = 0;
      
      if ( pthread_create( &(settings->software_sync_thread), NULL,
         lavrec_software_sync_thread, (void *) info ) )
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Failed to create software sync thread");
         lavrec_change_state(info, LAVREC_STATE_STOP);
      }
   }

   /* reset the counter(s) */
   nerr = 0;
   write_frame = 1;
   first_lost = 0;
   stats.stats_changed = 0;
   stats.num_syncs = 0;
   stats.num_lost = 0;
   stats.num_frames = 0;
   stats.num_asamps = 0;
   stats.num_ins = 0;
   stats.num_del = 0;
   stats.num_aerr = 0;
   stats.tdiff1 = 0.;
   stats.tdiff2 = 0.;
   if (info->software_encoding);
      settings->bsync.frame = -1;
   gettimeofday( &(stats.prev_sync), NULL );

   /* The video capture loop */
   while (settings->state == LAVREC_STATE_RECORDING)
   {
      /* sync on a frame */
      if (!lavrec_sync_buffer(info, &(settings->bsync)))
      {
         if (info->files)
            lavrec_close_files_on_error(info);
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error syncing on a buffer: %s", strerror(errno));
         nerr++;
      }
      stats.num_syncs++;

      gettimeofday( &(stats.cur_sync), NULL );
      if(stats.num_syncs==1)
      {
         first_time = settings->bsync.timestamp;
         first_lost = settings->bsync.seq;
         if(info->audio_size && info->sync_correction > 1)
         {
            /* Get time difference beetween audio and video in bytes */
            settings->audio_offset  = ((first_time.tv_usec-settings->audio_t0.tv_usec)*1.e-6 +
               first_time.tv_sec-settings->audio_t0.tv_sec - settings->spvf)*info->audio_rate;
            settings->audio_offset *= settings->audio_bps;   /* convert to bytes */
         }
         else
            settings->audio_offset = 0;
      }

      time = settings->bsync.timestamp.tv_sec - first_time.tv_sec
         + 1.e-6*(settings->bsync.timestamp.tv_usec - first_time.tv_usec)
         + settings->spvf; /* for first frame */


      /* Should we write a frame? */
      if(info->single_frame)
      {
	 lavrec_change_state_if(info, LAVREC_STATE_PAUSED, LAVREC_STATE_RECORDING);
         write_frame = 1;
         nfout = 1; /* always output frame only once */
      }
      else if(info->time_lapse > 1)
      {

         write_frame = (stats.num_syncs % info->time_lapse) == 0;
         nfout = 1; /* always output frame only once */

      }
      else /* normal capture */
      {

         nfout = 1;
         frame_cnt = settings->bsync.seq - stats.num_syncs - first_lost + 1; /* total lost frames */
         if (info->sync_correction > 0) 
            nfout +=  frame_cnt - stats.num_lost; /* lost since last sync */
         stats.stats_changed = (stats.num_lost != frame_cnt);
         stats.num_lost = frame_cnt;

         /* Check if we have to insert/delete frames to stay in sync */
         if (info->sync_correction > 1)
         {
            if( stats.tdiff1 - stats.tdiff2 < -settings->sync_lim)
            {
               nfout++;
               stats.num_ins++;
               stats.stats_changed = 1;
               stats.tdiff1 += settings->spvf;
            }
            if( stats.tdiff1 - stats.tdiff2 > settings->sync_lim)
            {
               nfout--;
               stats.num_del++;
               stats.stats_changed = 1;
               stats.tdiff1 -= settings->spvf;
            }
         }
      }

      /* write it out */
      if (info->software_encoding)
      {
         pthread_mutex_lock(&(settings->encoding_mutex));
         settings->buffer_valid[settings->bsync.frame] = write_frame?nfout:0;
         settings->buffer_completed[settings->bsync.frame] = 0;
         pthread_cond_broadcast(&(settings->buffer_filled[settings->bsync.frame]));
         pthread_mutex_unlock(&(settings->encoding_mutex));
      }
      else if(write_frame && nfout > 0)
      {
         if (video_captured(info,
	    settings->MJPG_buff+settings->bsync.frame*settings->breq.size,
            settings->bsync.length, nfout) != 1)
            nerr++; /* Done or error occured */

         /* Re-queue the buffer */
         if (!lavrec_queue_buffer(info, &(settings->bsync.frame)))
         {
            if (info->files)
               lavrec_close_files_on_error(info);
            lavrec_msg(LAVREC_MSG_ERROR, info,
               "Error re-queuing buffer: %s", strerror(errno));
            nerr++;
         }

         if (!lavrec_handle_audio(info, &(settings->bsync.timestamp)))
            nerr++;
      }

      /* if (nerr++) we need to stop and quit */
      if (nerr) lavrec_change_state(info, LAVREC_STATE_STOP);
   }

   if (info->software_encoding)
   {
      pthread_mutex_lock(&settings->encoding_mutex);
      settings->please_stop_syncing = 1; /* Ask the software sync thread to stop */
      for (x=0;x<settings->softreq.frames;x++)
        pthread_cond_broadcast(&settings->buffer_completion[x]);
      pthread_mutex_unlock(&settings->encoding_mutex);
      
      for (x = 0; x < info->num_encoders; x++)
      {
	 lavrec_msg(LAVREC_MSG_DEBUG, info,
		    "Joining encoding thread %d", x);
	 pthread_cancel( settings->encoders[x].thread );
	 pthread_join( settings->encoders[x].thread, NULL );
      }
      free(settings->encoders);
      
      lavrec_msg(LAVREC_MSG_DEBUG, info,
		 "Joining software sync thread");
      pthread_join(settings->software_sync_thread, NULL);

      for (x=0;x<MJPEG_MAX_BUF;x++)
      {
         pthread_cond_destroy(&(settings->software_sync_wait[x]));
      }
      pthread_mutex_destroy(&(settings->software_sync_mutex));
   }
   else
   {
      /* cancel all queued buffers (now this is much nicer!) */
      x = -1;
      if (ioctl(settings->video_fd, MJPIOC_QBUF_CAPT, &x) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error resetting buffer-queue: %s", strerror(errno));
      }
   }
}


/******************************************************
 * lavrec_recording_cycle()
 *   the main cycle for recording video
 ******************************************************/

static void lavrec_recording_cycle(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   while (1)
   {
      if (settings->state == LAVREC_STATE_PAUSED)
         lavrec_wait_for_start(info);
      else if (settings->state == LAVREC_STATE_RECORDING)
         lavrec_record(info);
      else
         break;
   }
}


/******************************************************
 * lavrec_capture_thread()
 *   the video/audio capture thread
 ******************************************************/

static void *lavrec_capture_thread(void *arg)
{
   lavrec_t *info = (lavrec_t*)arg;
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   lavrec_recording_cycle(info);

   /* shutdown video/audio and close */
   if (info->audio_size)
      audio_shutdown();

   /* certainty for all :-) */
   if (settings->video_file)
   {
      lav_close(settings->video_file);
      settings->video_file = NULL;
   }
   if (settings->video_file_old)
   {
      lav_close(settings->video_file_old);
      settings->video_file_old = NULL;
   }

   /* reset mixer */
   if (info->audio_size)
      lavrec_set_mixer(info, 0);

   /* Re-mute tuner audio if this is a tuner */
   if (settings->has_audio) {
      struct video_audio vau;
         
      lavrec_msg(LAVREC_MSG_INFO, info,
         "Re-muting tuner audio...");
      vau.flags |= VIDEO_AUDIO_MUTE;
      if (ioctl(settings->video_fd,VIDIOCSAUDIO,&vau) < 0)
      {
         lavrec_msg(LAVREC_MSG_ERROR, info,
            "Error resetting tuner audio params: %s", strerror(errno));
      }
   }

   /* and at last, we need to get rid of the video device */
   close(settings->video_fd);

   /* just to be sure */
   if (settings->state != LAVREC_STATE_STOP)
      lavrec_change_state(info, LAVREC_STATE_STOP);

   pthread_exit(NULL);
   return NULL;
}


/******************************************************
 * lavrec_malloc()
 *   malloc a pointer and set default options
 *
 * return value: a pointer to lavrec_t or NULL
 ******************************************************/

lavrec_t *lavrec_malloc(void)
{
   lavrec_t *info;
   video_capture_setup * settings;

   info = (lavrec_t *)malloc(sizeof(lavrec_t));
   if (!info)
   {
      lavrec_msg (LAVREC_MSG_ERROR, NULL,
         "Malloc error, you\'re probably out of memory");
      return NULL;
   }

   /* let's set some default values now */
   info->video_format = '\0';
   info->video_norm = 3;
   info->video_src = -1;
   info->software_encoding = 0;
   info->num_encoders = 0; /* this should be set to the number of processors */
   info->horizontal_decimation = 4;
   info->vertical_decimation = 4;
   info->geometry = (rect *)malloc(sizeof(rect));
   if (!(info->geometry))
   {
      lavrec_msg (LAVREC_MSG_ERROR, NULL,
         "Malloc error, you\'re probably out of memory");
      return NULL;
   }
   info->geometry->x = VALUE_NOT_FILLED;
   info->geometry->y = VALUE_NOT_FILLED;
   info->geometry->w = 0;
   info->geometry->h = 0;
   info->quality = 50;
   info->record_time = -1;
   info->tuner_frequency = 0;
   info->video_dev = "/dev/video";

   info->audio_size = 16;
   info->audio_rate = 44100;
   info->stereo = 0;
   info->audio_level = -1;
   info->mute = 0;
   info->audio_src = 'l';
   info->use_read = 0;
   info->audio_dev = "/dev/dsp";
   info->mixer_dev = "/dev/mixer";

   info->single_frame = 0;
   info->time_lapse = 1;
   info->sync_correction = 2;
   info->MJPG_numbufs = 64;
   info->MJPG_bufsize = 256;

   info->files = NULL;
   info->num_files = 0;
   info->flush_count = 60;
   info->output_statistics = NULL;
   info->audio_captured = NULL;
   info->video_captured = NULL;
   info->msg_callback = NULL;
   info->state_changed = NULL;
   info->max_file_size_mb = -1; /*(0x4000000>>20);*/ /* Safety first ;-) */
   info->settings = (void *)malloc(sizeof(video_capture_setup));
   if (!(info->settings))
   {
      lavrec_msg (LAVREC_MSG_ERROR, NULL,
         "Malloc error, you\'re probably out of memory");
      return NULL;
   }

   settings = (video_capture_setup*)(info->settings);
   
   pthread_mutex_init(&settings->state_mutex, 0);
   settings->state = LAVREC_STATE_STOP;
   settings->output_status = 0;
   settings->video_file = NULL;
   settings->video_file_old = NULL;
   
   return info;
}


/******************************************************
 * lavrec_main()
 *   the whole video-capture cycle
 *
 * Basic setup:
 *   * this function initializes the devices,
 *       sets up the whole thing and then forks
 *       the main task and returns control to the
 *       main app. It can then start recording by
 *       calling lavrec_start():
 *
 *   1) setup/initialize/open devices (state: STOP)
 *   2) wait for lavrec_start() (state: PAUSE)
 *   3) record (state: RECORD)
 *   4) stop/deinitialize/close (state: STOP)
 *
 *   * it should be possible to switch from RECORD
 *       to PAUSE and the other way around. When
 *       STOP, we stop and close the devices, so
 *       then you need to re-call this function.
 *
 * return value: 1 on succes, 0 on error
 ******************************************************/

int lavrec_main(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   int ret;
   struct sched_param schedparam;

   /* Flush the Linux File buffers to disk */
   sync();

   /* start with initing */
   if (!lavrec_init(info))
      return 0;

   /* Now we're ready to go move to Real-time scheduling... */
   schedparam.sched_priority = 1;
   if(setpriority(PRIO_PROCESS, 0, -15)) { /* Give myself maximum priority */ 
      lavrec_msg(LAVREC_MSG_WARNING, info,
         "Unable to set negative priority for main thread");
   }
   if( (ret = pthread_setschedparam( pthread_self(), SCHED_FIFO, &schedparam ) ) ) {
      lavrec_msg(LAVREC_MSG_WARNING, info,
         "Pthread Real-time scheduling for main thread could not be enabled"); 
   }

   /* now, set state to pause and catch audio until started */
   /* lavrec_change_state(info, LAVREC_STATE_PAUSED); */
   settings->state = LAVREC_STATE_PAUSED;

   /* fork ourselves to return control to the main app */
   if( pthread_create( &(settings->capture_thread), NULL,
      lavrec_capture_thread, (void*)info) )
   {
      lavrec_msg(LAVREC_MSG_ERROR, info,
         "Failed to create thread");
      return 0;
   }

   return 1;
}


/******************************************************
 * lavrec_start()
 *   start recording (only call when ready!)
 *
 * return value: 1 on succes, 0 on error
 ******************************************************/

int lavrec_start(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if (!lavrec_change_state_if(info, LAVREC_STATE_RECORDING, LAVREC_STATE_PAUSED))
   {
      lavrec_msg(LAVREC_MSG_WARNING, info,
         "Not ready for capture (state = %d)!", settings->state);
      return 0;
   }

   return 1;
}


/******************************************************
 * lavrec_pause()
 *   pause recording (you can call play to continue)
 *
 * return value: 1 on succes, 0 on error
 ******************************************************/

int lavrec_pause(lavrec_t *info)
{
   if (!lavrec_change_state_if(info, LAVREC_STATE_PAUSED, LAVREC_STATE_RECORDING))
   {
      lavrec_msg(LAVREC_MSG_WARNING, info,
		 "Not recording!");
      return 0;
   }
   return 1;
}


/******************************************************
 * lavrec_stop()
 *   stop recording
 *
 * return value: 1 on succes, 0 on error
 ******************************************************/

int lavrec_stop(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   int okay = lavrec_change_state_if(info, LAVREC_STATE_STOP, LAVREC_STATE_RECORDING)
      || lavrec_change_state_if(info, LAVREC_STATE_STOP, LAVREC_STATE_PAUSED);

   if (!okay)
   {
      lavrec_msg(LAVREC_MSG_DEBUG, info,
		 "We weren't even initialized!");
      lavrec_change_state(info, LAVREC_STATE_STOP);
      return 0;
   }

   lavrec_change_state(info, LAVREC_STATE_STOP);

   pthread_join( settings->capture_thread, NULL );

   return 1;
}


/******************************************************
 * lavrec_free()
 *   free() the struct
 *
 * return value: 1 on succes, 0 on error
 ******************************************************/

int lavrec_free(lavrec_t *info)
{
   video_capture_setup *settings = (video_capture_setup *)info->settings;

   if (settings->state != LAVREC_STATE_STOP)
   {
      lavrec_msg(LAVREC_MSG_WARNING, info,
         "We're not stopped yet, use lavrec_stop() first!");
      return 0;
   }

   pthread_mutex_destroy(&settings->state_mutex);
   free(settings);
   free(info->geometry);
   free(info);
   return 1;
}


/******************************************************
 * lavrec_busy()
 *   Wait until capturing is finished
 ******************************************************/

void lavrec_busy(lavrec_t *info)
{
   pthread_join( ((video_capture_setup*)(info->settings))->capture_thread, NULL);
}
