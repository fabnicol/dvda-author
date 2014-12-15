/*
 * liblavplay - a librarified Linux Audio Video PLAYback
 *
 * Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
 * Extended by:   Gernot Ziegler  <gz@lysator.liu.se>
 *                Ronald Bultje   <rbultje@ronald.bitfreak.net>
 *              & many others
 *
 * A library for playing back MJPEG video via software MJPEG
 * decompression (using SDL) or via hardware MJPEG video
 * devices such as the Pinnacle/Miro DC10(+), Iomega Buz,
 * the Linux Media Labs LML33, the Matrox Marvel G200,
 * Matrox Marvel G400 and the Rainbow Runner G-series.
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

#include <lav_io.h>
#include <editlist.h>

enum {
   LAVPLAY_MSG_ERROR   = 0,
   LAVPLAY_MSG_WARNING = 1,
   LAVPLAY_MSG_INFO    = 2,
   LAVPLAY_MSG_DEBUG   = 3
};

enum {
   LAVPLAY_STATE_STOP    = 0,   /* uninitialized state */
   LAVPLAY_STATE_PAUSED  = 1,   /* also known as: speed = 0 */
   LAVPLAY_STATE_PLAYING = 2    /* speed != 0 */
};


typedef struct {
   int stats_changed;           /* has anything bad happened?                        */

   unsigned int frame;          /* current frame which is being played back          */
   unsigned int num_corrs_a;    /* Number of corrections because video ahead audio   */
   unsigned int num_corrs_b;    /* Number of corrections because video behind audio  */
   unsigned int num_aerr;       /* Number of audio buffers in error                  */
   unsigned int num_asamps;
   unsigned int nsync;          /* Number of syncs                                   */
   unsigned int nqueue;         /* Number of frames queued                           */
   int play_speed;              /* current playback speed                            */
   int audio;                   /* whether audio is currently turned on              */
   int norm;                    /* [0-2] playback norm: 0 = PAL, 1 = NTSC, 2 = SECAM */

   double tdiff;                /* video/audio time difference (sync debug purposes) */
} video_playback_stats;

typedef struct {
   char playback_mode;          /* [HSC] H = hardware/on-screen, C = hardware/on-card, S = software (SDL) */
   int  horizontal_offset;      /* Horizontal offset of the video when using hardware playback */
   int  vertical_offset;        /* Vertical offset of the video when using hardware playback */
   int  exchange_fields;        /* [0-1] whether to exchange the fields (for interlaced video) */
   int  zoom_to_fit;            /* [0-1] zooms video to fit the screen as good as possible */
   int  flicker_reduction;      /* [0-1] whether to use flicker reduction */
   int  sdl_width;              /* width of the SDL playback window in case of software playback */
   int  sdl_height;             /* height of the SDL playback window in case of software playback */
   int  soft_full_screen;       /* [0-1] set software-driven full-screen/screen-output, 1 = yes, 0 = no */
   int  vw_x_offset;            /* onscreen hardware playback video window X offset */
   int  vw_y_offset;            /* onscreen hardware playback video window Y offset */
   const char *video_dev;       /* the video device */
   const char *display;         /* the X-display (only important for -H) */

   int  audio;                  /* When play audio, 0:never, or sum of 1:forward, 2:reverse, 4:fast, 8:pause */
   int  use_write;              /* whether to use "write" (1) or mmap (0) for audio playback */
   const char *audio_dev;       /* the audio device */

   int  continuous;             /* [0-1] 0 = quit when the video has been played, 1 = continue cycle */
   int  sync_correction;        /* [0-1] Whether to enable sync correction, 0 = no, 1 = yes */
   int  sync_skip_frames;       /* [0-1] If video is behind audio: 1 = skip video, 0 = insert audio */
   int  sync_ins_frames;        /* [0-1] If video is ahead of audio: 1 = insert video, 0 = skip audio */
   int  MJPG_numbufs;           /* Number of MJPEG-buffers */

	int preserve_pathnames;		/* [0-1] Don't canonicalise pathnames
								 * when creating edit lists */
   EditList *editlist;          /* the main editlist */

   void (*output_statistics)(video_playback_stats *stats);     /* speaks for itself */
   void (*msg_callback)(int type, char* message);              /* callback for error/info/warn messages */
   void (*state_changed)(int new_state);                       /* changed state */
   void (*get_video_frame)(uint8_t *buffer, int *len, long num);  /* functions for manually submitting video/audio */
   void (*get_audio_sample)(uint8_t *buff, int *samps, long num); /* functions for manually submitting video/audio */

   void *settings;              /* private info - don't touch :-) (type UNKNOWN) */
} lavplay_t;


/* malloc the pointer and set default options */
lavplay_t *lavplay_malloc(void);

/* the whole video-playback cycle */
int lavplay_main(lavplay_t *info);

/* stop playing back (which also deinitializes everything) */
int lavplay_stop(lavplay_t *info);

/* free info and quit if necessary */
int lavplay_free(lavplay_t *info);

/* Wait until playback is finished */
void lavplay_busy(lavplay_t *info);


/*** Methods for searching through the video stream ***/

/* go to a specific frame, if framenum<0, then it's 'total_frames - framenum' */
int lavplay_set_frame(lavplay_t *info, long framenum);

/* increase (numframes>0) or decrease (numframes<0) a number of frames */
int lavplay_increase_frame(lavplay_t *info, long numframes);

/* set the playback speed, if speed<0, then we're playing backwards */
int lavplay_set_speed(lavplay_t *info, int speed);


/*** Methods for simple video editing (cut/paste) ***/

/* cut a number of frames into a buffer */
int lavplay_edit_cut(lavplay_t *info, long start, long end);

/* copy a number of frames into a buffer */
int lavplay_edit_copy(lavplay_t *info, long start, long end);

/* paste frames from the buffer into a certain position */
int lavplay_edit_paste(lavplay_t *info, long destination);

/* add a number of frames from a new movie to a certain position in the current movie */
int lavplay_edit_addmovie(lavplay_t *info, char *movie, long start, long end, long destination);

/* move a number of frames to a different position */
int lavplay_edit_move(lavplay_t *info, long start, long end, long destination);

/* delete a number of frames from the current movie */
int lavplay_edit_delete(lavplay_t *info, long start, long end);

/* set the part of the movie that will actually be played, start<0 means whole movie */
int lavplay_edit_set_playable(lavplay_t *info, long start, long end);


/*** Control sound during video playback */

/* mutes or unmutes audio (1 = on, 0 = off) */
int lavplay_toggle_audio(lavplay_t *info, int audio);


/*** Methods for saving the currently played movie to editlists or open new movies */

/* save a certain range of frames to an editlist */
int lavplay_save_selection(lavplay_t *info, char *filename, long start, long end);

/* save the whole current movie to an editlist */
int lavplay_save_all(lavplay_t *info, char *filename);

/* open a new (series of) movie */
int lavplay_open(lavplay_t *info, char **files, int num_files);
