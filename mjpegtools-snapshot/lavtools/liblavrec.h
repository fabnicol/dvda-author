/*
 * liblavrec - a librarified Linux Audio Video RECord
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

#include <sys/time.h>
#include <mjpeg_types.h>
#include <frequencies.h>

#define AUDIO_BUFFER_SIZE 8192

enum {
   LAVREC_MSG_ERROR   = 0,
   LAVREC_MSG_WARNING = 1,
   LAVREC_MSG_INFO    = 2,
   LAVREC_MSG_DEBUG   = 3
};

enum {
   LAVREC_STATE_STOP      = 0,
   LAVREC_STATE_PAUSED    = 1,
   LAVREC_STATE_RECORDING = 2
};

typedef struct {
   int stats_changed;           /* has anything bad happened? */

   unsigned int num_frames;     /* Number of video frames captured until now  */
   unsigned int num_syncs;      /* Number of MJPIOC_SYNC ioctls               */
   unsigned int num_lost;       /* Number of frames lost                      */
   unsigned int num_asamps;     /* Number of frames written to file           */
   unsigned int num_ins;        /* Number of frames inserted for sync         */
   unsigned int num_del;        /* Number of frames deleted for sync          */
   unsigned int num_aerr;       /* Number of audio buffers in error           */

   int current_output_file;     /* the number of the current file             */
   char output_filename[1024];  /* name of current recording file             */

   double tdiff1;               /* Time difference (sync debug purposes only) */
   double tdiff2;               /* Time difference (sync debug purposes only) */

   struct timeval prev_sync;
   struct timeval cur_sync;
} video_capture_stats;


typedef struct {
   int x;                       /* x-positions */
   int y;                       /* y-position  */
   unsigned int w;              /* width       */
   unsigned int h;              /* height      */
} rect;


typedef struct {
   char video_format;           /* [aAqm] a/A = AVI, j = JPEG, q = Quicktime */
   int  video_norm;             /* [0-3] 0 = PAL, 1 = NTSC, 2 = SECAM, 3 = auto */
   int  video_src;              /* [0-3] 0 = Composite, 1 = S-video, 2 = TV-tuner, 3 = auto */
   int  software_encoding;      /* [0-1] 0 = hardware MJPEG encoding (zoran), 1 = software MJPEG encoding */
   unsigned int num_encoders; /* Number of software JPEG compressor threads */

   int  record_time;            /* time to record (in seconds). Default: -1 (unlimited) */
   int  horizontal_decimation;  /* [1,2,4] horizontal decimation */
   int  vertical_decimation;    /* [1,2,4] vertical decimation */
   rect *geometry;              /* X geometry string: what to capture */
   int  quality;                /* [0-100] video capture quality */
   int  tuner_frequency;        /* when using a TV-tuner, the tuner frequency can be set */
   const char *video_dev;       /* /dev-entry for the video device */

   int  audio_size;             /* [0,8,16] audio sample size, 0 means no audio */
   int  audio_rate;             /* Audio rate supported by the soundcard (e.g. 11025, 22050, 44100) */
   int  stereo;                 /* [0,1] 0 = mono, 1 = stereo */
   int  audio_level;            /* [-1,0-100] audio volume (0-100) or -1 to use mixer defaults */
   int  mute;                   /* [0,1] 0 = don't mute, 1 = mute (e.g. for a microphone) */
   char audio_src;              /* [lmc] l = Line-in, m = Microphone, c = CD-ROM */
   int  use_read;               /* whether to use 'read' (1) or mmap (0) for audio capture */
   const char *audio_dev;       /* /dev-entry for the audio device */
   const char *mixer_dev;       /* /dev-entry for the mixer device */

   int  single_frame;           /* [0,1] lavrec_main() captures one frame and returns */
   int  time_lapse;             /* [>=1] one out of each 'n' frames is captured (>=1) */
   int  sync_correction;        /* [0-2] 0 = none, 1 = replicate frames, 2 = 1 + sync correction */
   int  MJPG_numbufs;           /* Number of MJPEG-buffers */
   int  MJPG_bufsize;           /* buffer size (in kB) per MJPEG-buffer */

   char **files;                /* the files where to capture the video to */
   int  num_files;              /* number of files in the files[]-array */
   int max_file_size_mb;
   int max_file_frames;         /* maximum number of frames per file */
   int flush_count;             /* How often (in frames) to flush data to disk */
   void (*output_statistics)(video_capture_stats *stats);      /* speaks for itself */
   void (*audio_captured)(uint8_t *audio, long sampes);           /* callback when audio has been grabbed */
   void (*video_captured)(uint8_t *video, long size, long count); /* callback when a frame has been grabbed */
   void (*msg_callback)(int type, char* message);              /* callback for error/info/warn messages */
   void (*state_changed)(int new_state);                       /* changed state */

   void *settings;              /* private info - don't touch :-) (type video_capture_setup) */
} lavrec_t;



/* malloc the pointer and set default options */
lavrec_t *lavrec_malloc(void);

/* the whole video-capture cycle */
int lavrec_main(lavrec_t *info);

/* start recording (only call when ready!) */
int lavrec_start(lavrec_t *info);

/* pause recording (you can call play to continue) */
int lavrec_pause(lavrec_t *info);

/* stop recording (which also deinitializes everything) */
int lavrec_stop(lavrec_t *info);

/* free info and quit if necessary */
int lavrec_free(lavrec_t *info);

/* wait until capturing is finished */
void lavrec_busy(lavrec_t *info);
