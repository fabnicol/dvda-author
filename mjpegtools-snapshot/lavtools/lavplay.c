/*
 * lavplay - Linux Audio Video PLAYback
 *      
 * Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
 * Extended by:   Gernot Ziegler  <gz@lysator.liu.se>
 *                Ronald Bultje   <rbultje@ronald.bitfreak.net>
 *              & many others
 *
 * Plays back MJPEG AVI or Quicktime files using the
 * hardware of the zoran card or using SDL (software playback)
 *
 * Usage: lavplay [options] filename [filename ...]
 * where options are as follows:
 *
 *   -H/--H-offset num --- Horizontal offset
 *
 *   -V/--V-offset num --- Vertical offset
 *      You may use that only for quarter resolution videos
 *      unless you remove the following 4 lines of code in the BUZ driver:
 *
 *      if(params->img_x     <0) err0++;
 *      if(params->img_y     <0) err0++;
 *
 *      and
 *
 *      if(params->img_x + params->img_width  > 720) err0++;
 *      if(params->img_y + params->img_height > tvnorms[params->norm].Ha/2) err0++;
 *
 *      These parameters are usefull for videos captured from other sources
 *      which do not appear centered when played with the BUZ
 *
 *   -a/--audio num --- When play audio, 0:never, or sum of
 *      1:forward
 *      2:reverse
 *      4:fast
 *      8:pause
 *      (default 7:forward,reverse,fast)
 *
 *   -s/--skip num --- skip num seconds before playing
 *
 *   -c/--synchronization [01] --- Sync correction off/on (default on)
 *
 *   -n/--mjpeg-buffers num --- Number of MJPEG buffers
 *               normally no need to change the default
 *
 *   -q/--no-quit --- Do NOT quit at end of video
 *
 *   -g/--gui-mode --- GUI-mode, used by xlav and LVS to control video position
 *
 *   -x/--exchange-fields --- Exchange fields of an interlaced video
 *
 *   -z/--zoom --- "Zoom to fit flag"
 *      If this flag is not set, the video is always displayed in origininal
 *      resolution for interlaced input and in double height and width
 *      for not interlaced input. The aspect ratio of the input file is maintained.
 *
 *      If this flag is set, the program tries to zoom the video in order
 *      to fill the screen as much as possible. The aspect ratio is NOT
 *      maintained, ie. the zoom factors for horizontal and vertical directions
 *      may be different. This might not always yield the results you want!
 *
 *   -p/--playback [SHC] --- playback method
 *      'S': software playback (using SDL)
 *      'H': hardware playback directly on the monitor
 *      'C': hardware playback to the output of the card
 *      For 'C', you need xawtv to see what you're playing
 *
 *   --s-x-offset and --s-y-offset <num>
 *     Video window offset from top left corner when using onscreen video
 *     playback (--playback=H). Default: centered
 *
 * Following the options, you may give a optional +p/+n parameter (for forcing
 * the use of PAL or NTSC) and the several AVI files, Quicktime files
 * or Edit Lists arbitrarily mixed (as long as all files have the same
 * paramters like width, height and so on).
 *
 * The format of an edit list file is as follows:
 *    line 1: "LAV Edit List"
 *    line 2: "PAL" or "NTSC"
 *    line 3: Number of AVI/Quicktime files comprising the edit sequence
 *    next lines: Filenames of AVI/Quicktime files
 *    and then for every sequence
 *    a line consisting of 3 numbers: file-number start-frame end-frame
 *
 * If you are a real command line hardliner, you may try to entering the following
 * commands during playing (normally used by xlav/LVS, just type the command and
 * press enter):
 *    p<num>    Set playback speed, num may also be 0 (=pause) and negative (=reverse)
 *    s<num>    Skip to frame <num>
 *    s+<num>   <num> frames forward
 *    s-<num>   <num> frames backward
 *    +         1 frame forward (makes only sense when paused)
 *    -         1 frame backward (makes only sense when paused)
 *    q         quit
 *    em        Move scene (arg1->arg2) to position (arg3)
 *    eu/eo     Cut (u) or Copy (o) scene (arg1->arg2) into memory
 *    ep        Paste selection into current position of video
 *    ed        Delete scene (arg1->arg2) from video
 *    ea        Add movie (arg1, be sure that it's mov/avi!!)
 *                frame arg2-arg3 (if arg2=-1, whole movie) to position arg4
 *    es        Set a lowest/highest possible frame (max/min = trimming)
 *    om        Open movie (arg1) frame arg2-arg3 (arg2=-1 means whole
 *                movie). Watch out, this movie should have the same params
 *                as the current open movie or weird things may happen!!!
 *    wa        Save current movie into edit list (arg1)
 *    ws        Save current selection into memory into edit list (arg1)
 *
 **** Environment variables ***
 *
 * Recognized environment variables
 *    LAV_VIDEO_DEV: Name of video device (default: "/dev/video")
 *    LAV_AUDIO_DEV: Name of audio device (default: "/dev/dsp")
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "liblavplay.h"
#include "mjpeg_logging.h"
#include <mpegconsts.h>
#include <mpegtimecode.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SDL      /* Seems that this is needed so MAC OS X is also happy */
#include <SDL.h>
#endif

#define LAVPLAY_VSTR "lavplay" VERSION  /* Expected version info */

static lavplay_t *info;
static int verbose = 0;
static int skip_seconds = 0;
static int current_frame = 0;
static int gui_mode = 0;
static int exit_flag = 0;

static void Usage(char *progname)
{
   fprintf(stderr, "Usage: %s [options] <filename> [<filename> ...]\n", progname);
   fprintf(stderr, "where options are:\n");
   fprintf(stderr, "  -H/--H-offset num          Horizontal offset (for hardware playback)\n");
   fprintf(stderr, "  -V/--V-offset num          Vertical offset (for hardware playback)\n");
   fprintf(stderr, "  -s/--skip num              skip num seconds before playing\n");
   fprintf(stderr, "  -c/--synchronization [01]  Sync correction off/on (default on)\n");
   fprintf(stderr, "  -n/--mjpeg-buffers num     Number of MJPEG buffers\n");
   fprintf(stderr, "  -q/--no-quit               Do NOT quit at end of video\n");
   fprintf(stderr, "  -g/--gui-mode              GUI-mode (used by xlav and LVS)\n");
   fprintf(stderr, "  -x/--exchange-fields       Exchange fields of an interlaced video\n");
   fprintf(stderr, "  -z/--zoom                  Zoom video to fill screen as much as possible\n");
   fprintf(stderr, "  -Z/--full-screen           Switch to fullscreen\n");
   fprintf(stderr, "  -P/--preserve-pathnames    Do not 'canonicalise' pathnames in editlists\n" );
   fprintf(stderr, "  -p/--playback ["
#ifdef HAVE_SDL
      "S"
#else
      " "
#endif
#ifdef HAVE_V4L
      "HC"
#else
      "  "
#endif
      "]        playback: "
#ifdef HAVE_SDL
      "(S)oftware, "
#endif
#ifdef HAVE_V4L
      "(H)ardware (screen) or (C)ard"
#endif
      "\n");
   fprintf(stderr, "  -a/--audio num             When play audio, 0:never, or sum of\n");
   fprintf(stderr, "                             1:forward, 2:reverse, 4:fast, 8:pause (default: 7)\n");
   fprintf(stderr, "  -U/--use-write             Use write instead of mmap for audio playback\n");
   fprintf(stderr, "  -F/--flicker               Disable flicker reduction\n");
   fprintf(stderr, "  -S/--size NxN              width X height for SDL (S) or video (H) window\n");
   fprintf(stderr, "  --s-x-offset num           Video Window X offset from topleft corner\n");
   fprintf(stderr, "  --s-y-offset num           Video Window Y offset from topleft corner\n");
   fprintf(stderr, "  --display :x.x             The X-display to use (default: \':0.0\')\n");
   fprintf(stderr, "  -v/--verbose [012]         verbosity\n");
   exit(1);
}


static void SigHandler(int sig_num)
{
  lavplay_stop(info);
}

static void input(int type, char *message)
{
  switch (type)
  {
    case LAVPLAY_MSG_ERROR:
      mjpeg_error("%s", message);
      break;
    case LAVPLAY_MSG_WARNING:
      mjpeg_warn("%s", message);
      break;
    case LAVPLAY_MSG_INFO:
      mjpeg_info("%s", message);
      break;
    case LAVPLAY_MSG_DEBUG:
      mjpeg_debug("%s", message);
      break;
  }
}

static void stats(video_playback_stats *stats)
{
  if (exit_flag) return;

  current_frame = stats->frame;

  if (gui_mode)
  {
    printf("@%g/%d/%ld/%d\n",info->editlist->video_fps,stats->frame,
      info->editlist->video_frames,stats->play_speed);
  }
  else
  {
    MPEG_timecode_t tc;
    mpeg_timecode(&tc, stats->frame, mpeg_framerate_code
		  (mpeg_conform_framerate(info->editlist->video_fps)),
		  info->editlist->video_fps);
    printf("%d:%2.2d:%2.2d.%2.2d (%6.6d/%6.6ld) - Speed: %c%d, Norm: %s, Diff: %f\r",
      tc.h, tc.m, tc.s, tc.f, stats->frame, info->editlist->video_frames,
      stats->play_speed>0?'+':(stats->play_speed<0?'-':' '), abs(stats->play_speed),
      stats->norm==1?"NTSC":"PAL", stats->tdiff);
  }
  fflush(stdout);
}

static void state_changed(int state)
{
   if (state == LAVPLAY_STATE_STOP)
      exit_flag = 1;
}

static void process_input(char *buffer)
{
   int arg1, arg2, arg3;
   char arg[256];
   char *movie[1];

   switch (buffer[0])
   {
      case 'q':
         lavplay_stop(info);
         break;
      case '+':
         lavplay_increase_frame(info, 1);
         break;
      case '-':
         lavplay_increase_frame(info, -1);
         break;
      case 'a':
         lavplay_toggle_audio(info, atoi(buffer+1));
         break;
      case 'p':
         lavplay_set_speed(info, atoi(buffer+1));
         break;
      case 's':
         switch (buffer[1])
         {
            case '-':
               lavplay_increase_frame(info, -atoi(buffer+2));
               break;
            case '+':
               lavplay_increase_frame(info, atoi(buffer+2));
               break;
            default:
               lavplay_set_frame(info, atoi(buffer+1));
               break;
         }
         break;
      case 'e':
         switch (buffer[1])
         {
            case 'o':
               sscanf(buffer+2, "%d %d", &arg1, &arg2);
               lavplay_edit_copy(info, arg1, arg2);
               break;
            case 'u':
               sscanf(buffer+2, "%d %d", &arg1, &arg2);
               lavplay_edit_cut(info, arg1, arg2);
               break;
            case 'p':
               lavplay_edit_paste(info, current_frame);
               break;
            case 'm':
               sscanf(buffer+2, "%d %d %d", &arg1, &arg2, &arg3);
               lavplay_edit_move(info, arg1, arg2, arg3);
               break;
            case 'a':
               sscanf(buffer+2, "%s %d %d %d", arg, &arg1, &arg2, &arg3);
               lavplay_edit_addmovie(info, arg, arg1, arg2, arg3);
               break;
            case 'd':
               sscanf(buffer+2, "%d %d", &arg1, &arg2);
               lavplay_edit_delete(info, arg1, arg2);
               break;
            case 's':
               sscanf(buffer+2, "%d %d", &arg1, &arg2);
               lavplay_edit_set_playable(info, arg1, arg2);
               break;
         }
         break;
      case 'o':
         movie[0] = buffer+2;
         lavplay_open(info, movie, 1);
         break;
      case 'w':
         switch (buffer[1])
         {
            case 'a':
               sscanf(buffer+3, "%s", arg);
               lavplay_save_all(info, arg);
               break;
            case 's':
               sscanf(buffer+3, "%d %d %s", &arg1, &arg2, arg);
               lavplay_save_selection(info, arg, arg1, arg2);
               break;
         }
         break;
   }
}

static int set_option(const char *name, char *value)
{
   /* return 1 means error, return 0 means okay */
   int nerr = 0;
   if (strcmp(name, "verbose")==0 || strcmp(name, "v")==0)
   {
      verbose = atoi(optarg);
   }
   else if (strcmp(name, "audio")==0 || strcmp(name, "a")==0)
   {
      info->audio = atoi(optarg);
   }
   else if (strcmp(name, "use-write")==0 || strcmp(name, "U")==0)
   {
      info->use_write = 1;
   }
   else if (strcmp(name, "H-offset")==0 || strcmp(name, "H")==0)
   {
      info->horizontal_offset = atoi(optarg);
   }
   else if (strcmp(name, "V-offset")==0 || strcmp(name, "V")==0)
   {
      info->vertical_offset = atoi(optarg);
   }
   else if (strcmp(name, "skip")==0 || strcmp(name, "s")==0)
   {
      skip_seconds = atoi(optarg);
      if (skip_seconds<0) skip_seconds = 0;
   }
   else if (strcmp(name, "synchronization")==0 || strcmp(name, "c")==0)
   {
      info->sync_correction = atoi(optarg);
   }
   else if (strcmp(name, "mjpeg-buffers")==0 || strcmp(name, "n")==0)
   {
      info->MJPG_numbufs = atoi(optarg);
      if (info->MJPG_numbufs<4) info->MJPG_numbufs = 4;
      if (info->MJPG_numbufs>256) info->MJPG_numbufs = 256;
   }
   else if (strcmp(name, "no-quit")==0 || strcmp(name, "q")==0)
   {
      info->continuous = 1;
   }
   else if (strcmp(name, "exchange-fields")==0 || strcmp(name, "x")==0)
   {
      info->exchange_fields = 1;
   }
   else if (strcmp(name, "zoom")==0 || strcmp(name, "z")==0)
   {
      info->zoom_to_fit = 1;
   }
   else if (strcmp(name, "gui-mode")==0 || strcmp(name, "g")==0)
   {
      gui_mode = 1;
   }
   else if (strcmp(name, "full-screen")==0 || strcmp(name, "Z")==0)
   {
      info->soft_full_screen = 1;
   }
   else if( strcmp( name, "preserve-pathnames") == 0 || strcmp(name, "P")==0)
   {
	   info->preserve_pathnames = 1;
   }
   else if (strcmp(name, "playback")==0 || strcmp(name, "p")==0)
   {
      switch (value[0])
      {
#ifdef HAVE_V4L
         case 'H':
         case 'C':
#endif
#ifdef HAVE_SDL
         case 'S':
#endif
            info->playback_mode = value[0];
            break;
         default:
            mjpeg_error("Unknown playback mode: \'%c\'", value[0]);
            nerr++;
            break;
      }
   }
   else if (strcmp(name, "size")==0 || strcmp(name, "S")==0)
   {
      if (sscanf(value, "%dx%d", &info->sdl_width, &info->sdl_height)!=2)
      {
         mjpeg_error( "--size parameter requires NxN argument");
         nerr++;
      }
   }
   else if (strcmp(name, "flicker")==0 || strcmp(name, "F")==0)
   {
      info->flicker_reduction = 0;
   }
   else if (strcmp(name, "display")==0)
   {
      info->display = optarg;
   }
   else if (strcmp(name, "s-x-offset")==0)
   {
      info->vw_x_offset = atoi(optarg);
   }
   else if (strcmp(name, "s-y-offset")==0)
   {
      info->vw_y_offset = atoi(optarg);
   }
   else nerr++; /* unknown option - error */

   return nerr;
}

static void check_command_line_options(int argc, char *argv[])
{
   int nerr,n,option_index=0;
   char option[2];

#ifdef HAVE_GETOPT_LONG
   /* getopt_long options */
   static struct option long_options[]={
      {"verbose"         ,1,0,0},   /* -v/--verbose         */
      {"h-offset"        ,1,0,0},   /* -H/--H-offset        */
      {"v-offset"        ,1,0,0},   /* -V/--V-offset        */
      {"skip"            ,1,0,0},   /* -s/--skip            */
      {"synchronization" ,1,0,0},   /* -c/--synchronization */
      {"mjpeg-buffers"   ,1,0,0},   /* -n/--mjpeg-buffers   */
      {"no-quit"         ,0,0,0},   /* -q/--no-quit         */
      {"exchange-fields" ,0,0,0},   /* -x/--exchange-fields */
      {"zoom"            ,0,0,0},   /* -z/--zoom            */
      {"full-screen"     ,0,0,0},   /* -Z/--full-screen     */
      {"preserve-pathnames" ,0,0,0},   /* -P/--preserve-pathnames    */	  
      {"playback"        ,1,0,0},   /* -p/--playback [SHC]  */
      {"audio"           ,1,0,0},   /* -a/--audio num       */
      {"use-write"       ,0,0,0},   /* -U/--use-write       */
      {"gui-mode"        ,1,0,0},   /* -g/--gui-mode        */
      {"size"            ,1,0,0},   /* -S/--size            */
      {"flicker"         ,0,0,0},   /* -F/--flicker         */
      {"display"         ,1,0,0},   /* --display            */
      {"s-x-offset"      ,1,0,0},   /* --s-x-offset         */
      {"s-y-offset"      ,1,0,0},   /* --s-y-offset         */
      {0,0,0,0}
   };
#endif

   if(argc < 2) Usage(argv[0]);

/* Get options */
   nerr = 0;
#ifdef HAVE_GETOPT_LONG
   while( (n=getopt_long(argc,argv,"S:a:v:H:V:s:c:n:t:qUZp:xrzgPF",
      long_options, &option_index)) != EOF)
#else
   while( (n=getopt(argc,argv,"S:a:v:H:V:s:c:n:t:qUZp:xrzgPF")) != EOF)
#endif
   {
      switch(n)
      {
#ifdef HAVE_GETOPT_LONG
         /* getopt_long values */
         case 0:
            nerr += set_option(long_options[option_index].name,
               optarg);
            break;
#endif

         /* These are the old getopt-values (non-long) */
         default:
            sprintf(option, "%c", n);
            nerr += set_option(option, optarg);
            break;
      }
   }
   if(optind>=argc) nerr++;
   if(nerr) Usage(argv[0]);

   if (getenv("LAV_VIDEO_DEV")) info->video_dev = getenv("LAV_VIDEO_DEV");
   else {
       struct stat vstat;
       if(stat("/dev/video", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
           info->video_dev = "/dev/video";
       else if(stat("/dev/video0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
           info->video_dev = "/dev/video0";
       else if(stat("/dev/v4l/video0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
           info->video_dev = "/dev/v4l/video0";
       else if(stat("/dev/v4l0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
           info->video_dev = "/dev/v4l0";
       else if(stat("/dev/v4l", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
           info->video_dev = "/dev/v4l";
   }
   if (getenv("LAV_AUDIO_DEV")) info->audio_dev = getenv("LAV_AUDIO_DEV");
   else {
       struct stat astat;
       if(stat("/dev/dsp", &astat) == 0 && S_ISCHR(astat.st_mode)) 
           info->audio_dev = "/dev/dsp";
       else if(stat("/dev/sound/dsp", &astat) == 0 && S_ISCHR(astat.st_mode)) 
           info->audio_dev = "/dev/sound/dsp";
       else if(stat("/dev/audio", &astat) == 0 && S_ISCHR(astat.st_mode)) 
           info->audio_dev = "/dev/audio";
   }

   mjpeg_default_handler_verbosity(verbose);

   /* Get and open input files */
   lavplay_open(info, argv + optind, argc - optind);
}

int main(int argc, char **argv)
{
   char buffer[256];

   /* Output Version information - Used by xlav to check for
    * consistency. 
    */
   printf( LAVPLAY_VSTR "\n" );
   fflush(stdout);
   printf( "lavtools version " VERSION "\n" );
   fcntl(0, F_SETFL, O_NONBLOCK);
   signal(SIGINT,SigHandler);
   info = lavplay_malloc();
   if (!info) return 1;
   check_command_line_options(argc, argv);
   info->output_statistics = stats;
   info->msg_callback = input;
   info->state_changed = state_changed;
   if (!lavplay_main(info)) return 1;
   if (!lavplay_set_frame(info, skip_seconds * info->editlist->video_norm=='p'?25:30)) return 1;
   if (!lavplay_set_speed(info, 1)) return 1;

   /* okay, now wait until we get command-line input and ROCK! */
   while (!exit_flag)
   {
      if (read(0, buffer, 255) > 0)
         process_input(buffer);
      else
         usleep(10000);
   }

   lavplay_busy(info); /* wait for all the nice goodies to shut down */
   lavplay_free(info);

   fprintf(stderr, "\n");

   return 0;
}
