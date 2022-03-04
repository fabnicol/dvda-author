/*
 * lavrec - Linux Audio Video RECord
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
 * Usage: lavrec [options] filename [filename ...]
 * where options are as follows:
 *
 *   -f/--format [aAqm] --- Output file format:
 *      'a': AVI (default)
 *      'A': AVI with fields exchanged
 *      'j': JPEG image(s)
 *      'q': quicktime (if compiled with quicktime support)
 *      Hint: If your AVI video looks strange, try 'A' instead 'a'
 *      and vice versa.
 *		 
 *   -i/--input [pPnNsStTa] --- Input Source:
 *   -i/--input input[:norm]
 *      'p': PAL       Composite Input or first Bt8x8 input
 *      'P': PAL       SVHS-Input or second Bt8x8 input
 *      't': PAL       TV tuner input or third Bt8x8 input
 *      'n': NTSC      Composite Input or first Bt8x8 input
 *      'N': NTSC      SVHS-Input or second Bt8x8 input
 *      'T': NTSC      TV tuner input or third Bt8x8 input
 *      's': SECAM     Composite Input or first Bt8x8 input
 *      'S': SECAM     SVHS-Input or second Bt8x8 input
 *      'f': SECAM     TV tuner input or third Bt8x8 input
 *      'a': Autosense (default)
 *      input - a number, 1-10
 *      norm  - pal, ntsc or secam
 *
 *   -d/--decimation num --- Frame recording decimation:
 *      must be either 1, 2 or 4 for identical decimation
 *      in horizontal and vertical direction (mostly used) or a
 *      two digit letter with the first digit specifying horizontal
 *      decimation and the second digit specifying vertical decimation
 *      (more exotic usages). Not supported for BTTV.
 *
 *   -g/--geometry WxH+X+Y --- An X-style geometry string (capturing area):
 *      Even if a decimation > 1 is used, these values are always
 *      coordinates in the undecimated frame.  
 *                For DC10: 768/640x{576 or 480}.
 *                For others: 720x{576 or 480}.
 *      Also, unlike in X-Window, negative values for X and Y
 *      really mean negative offsets (if this feature is enabled
 *      in the driver) which lets you fine tune the position of the
 *      image caught.
 *      The horizontal resolution of the DECIMATED frame must
 *      allways be a multiple of 16, the vertical resolution
 *      of the DECIMATED frame must be a multiple of 16 for decimation 1
 *      and a multiple of 8 for decimations 2 and 4
 *
 *      If not offset (X and Y values) is given, the capture area
 *      is centered in the frame.
 *
 *      For BTTV-capuring (--software-encoding), X and Y are omitted
 *      and W and H are the video-capture-size (so decimation is omitted)
 *      too). BTTV does not support subframe-capture.
 *
 *   -q/--quality num --- quality:
 *      must be between 0 and 100, default is 50
 *
 *   -t/--time num -- capturing time:
 *      Time to capture in seconds, default is unlimited
 *      (use ^C to stop capture!)
 *
 *   -S/--single-frame --- enables single-frame capturing mode
 *
 *   -T/--time-lapse num --- time-lapse mode:
 *      Time lapse factor: Video will be played back <num> times
 *      faster, audio is switched off.
 *      This means that only every <num>th frame is recorded.
 *      If num==1 it is silently ignored.
 *
 *   -w/--wait --- Wait for user confirmation to start
 *
 *   -B/--batch --- Batch mode recording.  Minimal error logging, no
 *      interaction, works when output is redirected to files.
 *      Intended for use when doing unattended script-controlled
 *      recording.
 *
 *   --software-encoding --- encode frames in software-mode:
 *      Mainly intended to make it possible to use lavrec with BTTV
 *      cards too. Should work for V4L-capture with zoran cards too
 *      but that's only for testing - you really don't want to use
 *      this option for zoran-devices since they have hardware-encoding
 *      possibilities
 *
 *   --max-file-size --- The maximum size (in MB) per video file
 *      Intended for those that would like to record video in MJPEG
 *      format and need a specific filesize limit, for example to be
 *      able to burn the video files to CD (650 MB).
 *
 *   --max-file-frames --- The maximum number of frames per video file
 *      Intended for those that would like to record video in MJPEG
 *      format and need a specific number of frames per file, for
 *      example to be able to perform easy file and frame number
 *      arithmetics when cutting video manually.
 *
 *   --file-flush  --- How often (in frames) the current output 
 *       file (if any) should be flushed to disk.  (default:60).
 *       Set to 0 if your chosen file-system and system tuning
 *       ensures you don't get lengthy periods of disk activity
 *       during which lavrec writing is held up.
 *
 **** Audio settings ***
 *
 *   -a/--audio-bitsize num --- audio bitsize:
 *      Audio size in bits, must be 0 (no audio), 8 or 16 (default)
 *
 *   -r/--audio_bitrate num --- audio-bitrate:
 *      Audio rate (in Hz), must be a permitted sampling rate for
 *      your soundcard. Default is 22050.
 *
 *   -s/--stereo --- enable stereo (disabled by default)
 *
 *   -l/--audio-volume num --- audio recording level/volume:
 *      Audio level to use for recording, must be between 0 and 100
 *      or -1 (for not touching the mixer settings at all), default
 *      is 100.
 *
 *   -m/--mute --- mute output during recording:
 *      Mute audio output during recording (default is to let it enabled).
 *      This is particularly usefull if you are recording with a
 *      microphone to avoid feedback.
 *
 *   -R/--audio-source [lmc] --- Audio recording source:
 *      'l': line-in
 *      'm': microphone
 *      'c': cdrom
 *      '1': line1
 *      '2': line2
 *      '3': line3
 *
 **** Audio/Video synchronization ***
 *
 *   -c/--synchronization num --- Level of corrections for synchronization:
 *      0: Neither try to replicate lost frames nor any sync correction
 *      1: Replicate frames for lost frames, but no sync correction
 *      2. lost frames replication + sync correction
 *
 **** Special capture settings ***
 *
 *   -n/--mjpeg-buffers num --- Number of MJPEG capture buffers (default 64)
 *
 *   -b/--mjpeg-buffer-size num --- Size of MJPEG buffers in KB (default 256)
 *
 **** Environment variables ***
 *
 * Recognized environment variables:
 *    LAV_VIDEO_DEV: Name of video device (default: "/dev/video")
 *    LAV_AUDIO_DEV: Name of audio device (default: "/dev/dsp")
 *    LAV_MIXER_DEV: Name of mixer device (default: "/dev/mixer")
 *
 * To overcome the AVI (and linux-2.2 ext2fs) 2 GB filesize limit, you can:
 *   - give multiple filenames on the command-line
 *   - give one filename which contains a '%' sign. This name is then
 *     interpreted as the format argument to sprintf() to form multiple
 *     file names
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
#include "liblavrec.h"
#include "mjpeg_logging.h"
#include <mpegtimecode.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/fsuid.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

static lavrec_t *info;
static int num_frames, num_lost, num_ins, num_del, num_aerr;
static int show_stats = 0;
static int state;
static sem_t state_changed;
static int verbose = 0;
static int wait_for_start = 0;
static int batch_mode = 0;
static char input_source;
static pthread_t signal_thread;

static void Usage(char *progname)
{
	fprintf(stderr, "lavtools version " VERSION ": lavrec\n");
	fprintf(stderr, "Usage: %s [options] <filename> [<filename> ...]\n", progname);
	fprintf(stderr, "where options are:\n");
	fprintf(stderr, "  -f/--format [aAj"
#ifdef HAVE_LIBQUICKTIME
           "q"
#else
           ""
#endif
           "]         Format AVI/Quicktime\n");
	fprintf(stderr, "  -i/--input [pPnNsStTfa]     Input Source or 1-10:norm see manpage\n");
	fprintf(stderr, "  -d/--decimation num         Decimation (either 1,2,4 or two digit number)\n");
	fprintf(stderr, "  -g/--geometry WxH+X+Y       X-style geometry string (part of 768/720x576/480)\n");
	fprintf(stderr, "  -q/--quality num            Quality [%%]\n");
	fprintf(stderr, "  -t/--time num               Capture time (default: unlimited - Ctrl-C to stop\n");
	fprintf(stderr, "  -S/--single-frame           Single frame capture mode\n");
	fprintf(stderr, "  -T/--time-lapse num         Time lapse, capture only every <num>th frame\n");
	fprintf(stderr, "  -w/--wait                   Wait for user confirmation to start\n");
	fprintf(stderr, "  -B/--batch                  Batch mode for recording non-interactively\n");
	fprintf(stderr, "  -a/--audio-bitsize num      Audio size, 0 for no audio, 8 or 16\n");
	fprintf(stderr, "  -r/--audio-bitrate num      Audio rate [Hz]\n");
	fprintf(stderr, "  -s/--stereo                 Stereo (default: mono)\n");
	fprintf(stderr, "  -l/--audio-volume num       Recording level [%%], -1 for mixers not touched\n");
	fprintf(stderr, "  -m/--mute                   Mute audio output during recording\n");
	fprintf(stderr, "  -R/--audio-source [lmc123]  Set recording source: (l)ine, (m)icro, (c)d,\n");
	fprintf(stderr, "                              line(1), line(2), line(3\n");
	fprintf(stderr, "  -c/--synchronization [012]  Level of corrections for synchronization\n");
	fprintf(stderr, "  -n/--mjpeg-buffers num      Number of MJPEG buffers (default: 64)\n");
	fprintf(stderr, "  -b/--mjpeg-buffer-size num  Size of MJPEG buffers [Kb] (default: 256)\n");
	fprintf(stderr, "  -C/--channel LIST:CHAN      When using a TV tuner, channel list/number\n");
	fprintf(stderr, "  -F/--frequency KHz          When using a TV tuner, frequency in KHz\n");
	fprintf(stderr, "  -U/--use-read               Use read instead of mmap for recording\n");
	fprintf(stderr, "  --software-encoding         Use software JPEG-encoding (for BTTV-capture)\n");
	fprintf(stderr, "  --software-encoding-yuvp    Use software JPEG-encoding (for EM28XX-capture)\n");
	fprintf(stderr, "  --num-procs num             Number of encoding processes (default: 1)\n");
	fprintf(stderr, "  --max-file-size num         Maximum size per file (in MB)\n");
	fprintf(stderr, "  --max-file-frames num       Maximum number of frames per file\n");
	fprintf(stderr, "  --file-flush num            Flush capture file to disk every num frames\n");

	fprintf(stderr, "  -v/--verbose [012]          verbose level (default: 0)\n");
	fprintf(stderr, "Environment variables recognized:\n");
	fprintf(stderr, "   LAV_VIDEO_DEV, LAV_AUDIO_DEV, LAV_MIXER_DEV\n");
	exit(1);
}


/* RJ: The following stuff thanks to Philipp Zabel: */

/* pH5 - the following was stolen from glut (that stole the code from X):   */

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue         0x0000
#define XValue          0x0001
#define YValue          0x0002
#define WidthValue      0x0004
#define HeightValue     0x0008
#define AllValues       0x000F
#define XNegative       0x0010
#define YNegative       0x0020

/* the following function was stolen from the X sources as indicated. */

/* Copyright 	Massachusetts Institute of Technology  1985, 1986, 1987 */
/* $XConsortium: XParseGeom.c,v 11.18 91/02/21 17:23:05 rws Exp $ */

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged. 
 */

static int
ReadInteger(char *string, char **NextString)
{
    register int Result = 0;
    int Sign = 1;
    
    if (*string == '+')
		string++;
    else if (*string == '-')
    {
		string++;
		Sign = -1;
    }
    for (; (*string >= '0') && (*string <= '9'); string++)
    {
		Result = (Result * 10) + (*string - '0');
    }
    *NextString = string;
    if (Sign >= 0)
		return (Result);
    else
		return (-Result);
}

static int XParseGeometry(char *string, int *x, int *y, unsigned int *width, unsigned int *height)
{
	int mask = NoValue;
	register char *strind;
	unsigned int tempWidth, tempHeight;
	int tempX, tempY;
	char *nextCharacter;

	if ( (string == NULL) || (*string == '\0')) return(mask);
	if (*string == '=')
		string++;  /* ignore possible '=' at beg of geometry spec */

	strind = (char *)string;
	tempWidth = 0;
	if (*strind != '+' && *strind != '-' && *strind != 'x') {
		tempWidth = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter) 
		    return (0);
		strind = nextCharacter;
		mask |= WidthValue;
	}

	tempHeight = 0;
	if (*strind == 'x' || *strind == 'X') {	
		strind++;
		tempHeight = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter)
		    return (0);
		strind = nextCharacter;
		mask |= HeightValue;
	}

	tempX = tempY = 0;
	if ((*strind == '+') || (*strind == '-')) {
		if (*strind == '-') {
  			strind++;
			tempX = -ReadInteger(strind, &nextCharacter);
			if (strind == nextCharacter)
			    return (0);
			strind = nextCharacter;
			mask |= XNegative;

		}
		else
		{	strind++;
		tempX = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter)
			return(0);
		strind = nextCharacter;
		}
		mask |= XValue;
		if ((*strind == '+') || (*strind == '-')) {
			if (*strind == '-') {
				strind++;
				tempY = -ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
					return(0);
				strind = nextCharacter;
				mask |= YNegative;

			}
			else
			{
				strind++;
				tempY = ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
					return(0);
				strind = nextCharacter;
			}
			mask |= YValue;
		}
	}
	
	/* If strind isn't at the end of the string the it's an invalid
	   geometry specification. */

	if (*strind != '\0') return (0);

	if (mask & XValue)
	    *x = tempX;
 	if (mask & YValue)
	    *y = tempY;
	if (mask & WidthValue)
		*width = tempWidth;
	if (mask & HeightValue)
		*height = tempHeight;
	return (mask);
}

/* ===========================================================================
   Parse the X-style geometry string
*/

static int parse_geometry (char *geom, int *x, int *y, unsigned int *width, unsigned int *height)
{
	return XParseGeometry (geom, x, y, width, height);
}


static void input(int type, char *message)
{
  switch (type)
  {
    case LAVREC_MSG_ERROR:
      mjpeg_error("%s", message);
      break;
    case LAVREC_MSG_WARNING:
      mjpeg_warn("%s", message);
      break;
    case LAVREC_MSG_INFO:
      mjpeg_info("%s", message);
      break;
    case LAVREC_MSG_DEBUG:
      mjpeg_debug("%s", message);
      break;
  }
}

static void *
user_input_thread (void * arg)
{
   fd_set rfds;
   struct timeval timeout;
   char input_buffer[256];

   if (state == LAVREC_STATE_PAUSED)
   {
      printf("Press enter to start recording>");
      fflush(stdout);
   }

   while (state != LAVREC_STATE_STOP)
   {
      pthread_testcancel();

      /* wait for input */
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      if (select(1, &rfds, 0, 0, &timeout) == 0)
        continue; /* input timeout */

      read(0, input_buffer, sizeof(input_buffer));

      switch (state)
      {
         case LAVREC_STATE_PAUSED:
            lavrec_start(info);
            printf("Press enter to pause recording\n");
            break;
         case LAVREC_STATE_RECORDING:
            lavrec_pause(info);
            printf("Press enter to start recording>");
            fflush(stdout);
            break;
      }
   }

   return 0;
}

static void statechanged(int new)
{
  show_stats = (new == LAVREC_STATE_RECORDING);
  state = new;
  sem_post(&state_changed);
}

#if 0
static void batch_stats(video_capture_stats *stats)
{
	MPEG_timecode_t tc;
	static int first_stats = 1;
	static video_capture_stats prev_stats;
	return;
	if( first_stats )
	{
		first_stats = 0;
		prev_stats = *stats;
		return;
	}

	
	if( stats->num_lost == prev_stats.num_lost &&
        stats->num_ins == prev_stats.num_ins &&
		stats->num_del == prev_stats.num_del &&
		stats->num_aerr == prev_stats.num_aerr )
	{
		return;
	}

	prev_stats = *stats;
	mpeg_timecode(&tc, stats->num_frames, ((info->video_norm!=1)? 3: 4),
				  ((info->video_norm!=1)? 25.: 30000./1001.));
	fprintf(stdout, "%2d.%2.2d.%2.2d:%2.2d int: %05ld lst:%4d ins:%3d del:%3d "
		   "ae:%3d td1=%.3f td2=%.3f\n",
		   tc.h, tc.m, tc.s, tc.f,
		   (stats->cur_sync.tv_usec - stats->prev_sync.tv_usec)/1000, stats->num_lost,
        stats->num_ins, stats->num_del, stats->num_aerr, stats->tdiff1, stats->tdiff2);
	fflush(stdout);

}
#endif

static void output_stats(video_capture_stats *stats)
{
  num_frames = stats->num_frames;
  num_lost = stats->num_lost;
  num_ins = stats->num_ins;
  num_del = stats->num_del;
  num_aerr = stats->num_aerr;

  if (show_stats > 0 && !batch_mode)
  {
    MPEG_timecode_t tc;

    mpeg_timecode(&tc, stats->num_frames, ((info->video_norm!=1)? 3: 4),
				  ((info->video_norm!=1)? 25.: 30000./1001.));
    if( stats->prev_sync.tv_usec > stats->cur_sync.tv_usec )
      stats->prev_sync.tv_usec -= 1000000;
    if (info->single_frame)
      printf("%06d frames captured, press enter for more>", stats->num_frames);
    else {
      printf(
        "%d.%2.2d.%2.2d:%2.2d int:%03ld lst:%3d ins:%3d del:%3d "
        "ae:%3d td1=%.3f td2=%.3f\r",
        tc.h, tc.m, tc.s, tc.f,
        (stats->cur_sync.tv_usec - stats->prev_sync.tv_usec)/1000, stats->num_lost,
        stats->num_ins, stats->num_del, stats->num_aerr, stats->tdiff1, stats->tdiff2);
      if(verbose) printf("\n"); // Keep lines from overlapping
    }
     fflush(stdout);
  }
}

static int set_option(const char *name, char *value)
{
	/* return 1 means error, return 0 means okay */
	int nerr = 0;
	int i = 0;
	char *norm_i = NULL;

	if (strcmp(name, "format")==0 || strcmp(name, "f")==0)
	{
		info->video_format = value[0];
		if(value[0]!='a' && value[0]!='A' && value[0]!='j'
#ifdef HAVE_LIBQUICKTIME
                   && value[0]!='q'
#endif
                   )
		{
#ifdef	HAVE_LIBQUICKTIME
		mjpeg_error("Format (-f/--format) must be j, a, A or q");
#else
		mjpeg_error("Format (-f/--format) must be j, a or A");
#endif
		nerr++;
		}
	}
	else if (strcmp(name, "input")==0 || strcmp(name, "i")==0)
	{
		switch(value[0])
		{
			case 'p': info->video_norm = 0 /* PAL */; info->video_src = 0 /* composite */; break;
			case 'P': info->video_norm = 0 /* PAL */; info->video_src = 1 /* S-Video */; break;
			case 'n': info->video_norm = 1 /* NTSC */; info->video_src = 0 /* composite */; break;
			case 'N': info->video_norm = 1 /* NTSC */; info->video_src = 1 /* S-Video */; break;
			case 's': info->video_norm = 2 /* SECAM */; info->video_src = 0 /* composite */; break;
			case 'S': info->video_norm = 2 /* SECAM */; info->video_src = 1 /* S-Video */; break;
			case 't': info->video_norm = 0 /* PAL */; info->video_src = 2 /* TV-tuner */; break;
			case 'T': info->video_norm = 1 /* NTSC */; info->video_src = 2 /* TV-tuner */; break;
			case 'f': info->video_norm = 2 /* SECAM */; info->video_src = 2 /* TV-tuner */; break;
			case 'a':  info->video_norm = 3 /* auto */; info->video_src = -1 /* auto */; break;
			default:
				i = atoi(value);
				if (i>0 && i<11)
					{
					info->video_norm = 3 /* auto */; info->video_src = i - 1 /* input */;
					norm_i = strstr(value, ":");
					if(norm_i != NULL && strlen(norm_i) > 1)
						{
							if (strcasecmp(norm_i + 1, "pal") == 0)
								{
									info->video_norm = 0;
								}
							else if (strcasecmp(norm_i + 1, "ntsc") == 0)
								{
									info->video_norm = 1;
								}
							else if (strcasecmp(norm_i + 1, "secam") == 0)
								{
									info->video_norm = 0;
								}
							else
								{
									mjpeg_error("unknown norm: '%s'", norm_i + 1);
									nerr++;
									break;
								}
						}
					} 
				else 
					{
						mjpeg_error("numeric input must be between 1 and 10");
						nerr++;
					}
			break;

		}
		input_source = value[0];
	}
	else if (strcmp(name, "decimation")==0 || strcmp(name, "d")==0)
	{
		int i = atoi(value);
		if(i<10)
		{
			info->horizontal_decimation = info->vertical_decimation = i;
		}
		else
		{
			info->horizontal_decimation = i/10;
			info->vertical_decimation = i%10;
		}
		if( (info->horizontal_decimation != 1 && info->horizontal_decimation != 2 &&
			info->horizontal_decimation != 4) || (info->vertical_decimation != 1 &&
			info->vertical_decimation != 2 && info->vertical_decimation != 4) )
		{
			mjpeg_error("decimation (-d/--decimation) = %d invalid",i);
			mjpeg_error("   must be one of 1,2,4,11,12,14,21,22,24,41,42,44");
			nerr++;
		}
	}
	else if (strcmp(name, "verbose")==0 || strcmp(name, "v")==0)
	{
		verbose = atoi(value);
	}
	else if (strcmp(name, "quality")==0 || strcmp(name, "q")==0)
	{
		info->quality = atoi(value);
		if(info->quality<0 || info->quality>100)
		{
			mjpeg_error("quality (-q/--quality) = %d invalid (must be 0 ... 100)",info->quality);
			nerr++;
		}
	}
	else if (strcmp(name, "geometry")==0 || strcmp(name, "g")==0)
	{
		parse_geometry (value,
			&info->geometry->x, &info->geometry->y, &info->geometry->w, &info->geometry->h);
	}
	else if (strcmp(name, "time")==0 || strcmp(name, "t")==0)
	{
		info->record_time = atoi(value);
		if(info->record_time<=0)
		{
			mjpeg_error("record_time (-t/--time) = %d invalid",info->record_time);
			nerr++;
		}
	}
	else if (strcmp(name, "batch")==0 || strcmp(name, "B")==0)
	{
		batch_mode = 1;
	}
	else if (strcmp(name, "single-frame")==0 || strcmp(name, "S")==0)
	{
		info->single_frame = 1;
	}
	else if (strcmp(name, "use-read")==0 || strcmp(name, "U")==0)
	{
		info->use_read = 1;
	}
	else if (strcmp(name, "time-lapse")==0 || strcmp(name, "T")==0)
	{
		info->time_lapse = atoi(value);
		if(info->time_lapse<=1) info->time_lapse = 1;
	}
	else if (strcmp(name, "wait")==0 || strcmp(name, "w")==0)
	{
		wait_for_start = 1;
	}
	else if (strcmp(name, "audio-bitsize")==0 || strcmp(name, "a")==0)
	{
		info->audio_size = atoi(value);
		if(info->audio_size != 0 && info->audio_size != 8 && info->audio_size != 16)
		{
			mjpeg_error("audio_size (-a/--audio-bitsize) ="
				" %d invalid (must be 0, 8 or 16)",
				info->audio_size);
			nerr++;
		}
	}
	else if (strcmp(name, "audio-bitrate")==0 || strcmp(name, "r")==0)
	{
		info->audio_rate = atoi(value);
		if(info->audio_rate<=0)
		{
			mjpeg_error("audio_rate (-r/--audio-bitrate) = %d invalid",info->audio_rate);
			nerr++;
		}
	}
	else if (strcmp(name, "stereo")==0 || strcmp(name, "s")==0)
	{
		info->stereo = 1;
	}
	else if (strcmp(name, "audio-volume")==0 || strcmp(name, "l")==0)
	{
		info->audio_level = atoi(value);
		if(info->audio_level<-1 || info->audio_level>100)
		{
			mjpeg_error("recording level (-l/--audio-volume)"
				" = %d invalid (must be 0 ... 100 or -1)",
				info->audio_level);
			nerr++;
		}
	}
	else if (strcmp(name, "mute")==0 || strcmp(name, "m")==0)
	{
		info->mute = 1;
	}
	else if (strcmp(name, "audio-source")==0 || strcmp(name, "R")==0)
	{
		info->audio_src = value[0];
                if(info->audio_src!='l' && info->audio_src!='m' &&
                        info->audio_src!='c' && info->audio_src!='1'&&
                        info->audio_src!='2'&& info->audio_src!='3')
		{
			mjpeg_error("Recording source (-R/--audio-source)"
				" must be l,m,c,1,2, or 3\n");
			nerr++;
		}
	}
	else if (strcmp(name, "synchronization")==0 || strcmp(name, "c")==0)
	{
		info->sync_correction = atoi(value);
		if(info->sync_correction<0 || info->sync_correction>2)
		{
			mjpeg_error("Synchronization (-c/--synchronization) ="
				" %d invalid (must be 0, 1 or 2)",
				info->sync_correction);
			nerr++;
		}
	}
	else if (strcmp(name, "mjpeg-buffers")==0 || strcmp(name, "n")==0)
	{
		info->MJPG_numbufs = atoi(value);
	}
	else if (strcmp(name, "mjpeg-buffer-size")==0 || strcmp(name, "b")==0)
	{
		info->MJPG_bufsize = atoi(value);
	}
	else if (strcmp(name, "frequency")==0 || strcmp(name,"F")==0)
	{
		info->tuner_frequency = atoi(value);
	}
	else if (strcmp(name, "channel")==0 || strcmp(name,"C")==0)
	{
		int colin=0; int chlist=0; int chan=0;
		
		/* First look for the colon mark */
		while ( value[colin] && value[colin]!=':') colin++;
		if (value[colin]==0) /* didn't find the colin */
		{
			mjpeg_error("malformed channel parameter (-C/--channel) - example: -C europe-west:SE20");
			nerr++;
		}

		if (nerr==0) /* Now search for the given channel list in frequencies.c */
			while (strncmp(chanlists[chlist].name,value,colin)!=0)
			{
			  chlist++;
			  /* madmac debug: printf("Searching for channel list: chlist = %d, value=%s\n", chlist, value); */
			  if (chanlists[chlist].name==NULL)
			    {
			      mjpeg_error("bad frequency map");
			      nerr++;
			      break; /* bail out */
			    }
			}

		if (nerr==0) /* channel list - get the channel spec */
		  {
		    /*printf("channel list count is %d\n", chanlists[chlist].count); */
		    while (strcmp((chanlists[chlist].list)[chan].name,  &(value[colin+1]))!=0)
		      {
			chan++;
			
			if (chan > (chanlists[chlist].count))
			  {
			    mjpeg_error("bad channel spec (see e g xawtv, channel editor, for channel spec)");
			    nerr++; 
			    break;
			  }
			/*printf("Searching for channel: chan: %d %s\n", chan, (chanlists[chlist].list)[chan].name);*/
		      }
		  }

		if (nerr==0) 
			while (strcmp((chanlists[chlist].list)[chan].name,  &(value[colin+1]))!=0)
			{
				if ((chanlists[chlist].list)[chan++].name==NULL)
				{
					mjpeg_error("bad channel spec");
					nerr++;
				}
			}
		if (nerr==0)
		  {
		   printf("Ok, found channel %s with frequency %d\n", 
			   (chanlists[chlist].list)[chan].name, 
			   (chanlists[chlist].list)[chan].freq); 

		    info->tuner_frequency=(chanlists[chlist].list)[chan].freq;
		  }
	}
	else if (strcmp(name, "software-encoding")==0)
	{
		info->software_encoding = 1;
		/* set the number of enoding processes to the number of processors */
		if (info->num_encoders == 0)
			info->num_encoders = sysconf(_SC_NPROCESSORS_ONLN);
	}
	else if( strcmp(name, "software-encoding-yuvp")==0)
	{
		info->software_encoding = 2;
		/* set the number of enoding processes to the number of processors */
		if (info->num_encoders == 0)
			info->num_encoders = sysconf(_SC_NPROCESSORS_ONLN);
	}
	else if (strcmp(name, "num-procs")==0)
	{
		info->num_encoders = atoi(value);
	}
	else if (strcmp(name, "max-file-size")==0)
	{
		info->max_file_size_mb = atoi(optarg);
	}
	else if (strcmp(name, "max-file-frames")==0)
	{
		info->max_file_frames = atoi(optarg);
	}
	else if (strcmp(name, "file-flush")==0)
	{
		info->flush_count = atoi(optarg);
		if( info->flush_count < 0 )
			info->flush_count = 0;
			
	}
	else nerr++; /* unknown option - error */

	return nerr;
}

static void check_command_line_options(int argc, char *argv[])
{
	int n, nerr, option_index = 0;
	char option[2];
        char* dotptr;

#ifdef HAVE_GETOPT_LONG
	/* getopt_long options */
	static struct option long_options[]={
		{"format"           ,1,0,0},   /* -f/--format            */
		{"input"            ,1,0,0},   /* -i/--input             */
		{"decimation"       ,1,0,0},   /* -d/--decimation        */
		{"verbose"          ,1,0,0},   /* -v/--verbose           */
		{"quality"          ,1,0,0},   /* -q/--quality           */
		{"geometry"         ,1,0,0},   /* -g/--geometry          */
		{"time"             ,1,0,0},   /* -t/--time              */
		{"single-frame"     ,1,0,0},   /* -S/--single-frame      */
		{"time-lapse"       ,1,0,0},   /* -T/--time-lapse        */
		{"wait"             ,0,0,0},   /* -w/--wait              */
		{"batch"            ,0,0,0},   /* -b/--batch             */
		{"audio-bitsize"    ,1,0,0},   /* -a/--audio-size        */
		{"audio-bitrate"    ,1,0,0},   /* -r/--audio-rate        */
		{"stereo"           ,0,0,0},   /* -s/--stereo            */
		{"audio-volume"     ,1,0,0},   /* -l/--audio-volume      */
		{"mute"             ,0,0,0},   /* -m/--mute              */
		{"audio-source"     ,1,0,0},   /* -R/--audio-source      */
		{"synchronization"  ,1,0,0},   /* -c/--synchronization   */
		{"mjpeg-buffers"    ,1,0,0},   /* -n/--mjpeg_buffers     */
		{"mjpeg-buffer-size",1,0,0},   /* -b/--mjpeg-buffer-size */
		{"channel"          ,1,0,0},   /* -C/--channel           */
		{"use-read"         ,0,0,0},   /* -U/--use-read          */
		{"software-encoding",0,0,0},   /* --software-encoding    */
		{"software-encoding-yuvp",0,0,0}, /* --software-encoding-yuvp */
		{"num-procs"        ,1,0,0},   /* --num-procs            */
		{"max-file-size"    ,1,0,0},   /* --max-file-size        */
		{"max-file-frames"  ,1,0,0},   /* --max-file-frames      */
		{"file-flush"       ,1,0,0},   /* --file-flush           */
		{"frequency"        ,1,0,0},   /* --frequency/-F         */
		{0,0,0,0}
	};
#endif

	/* check usage */
	if (argc < 2)  Usage(argv[0]);

	/* Get options */
	nerr = 0;
#ifdef HAVE_GETOPT_LONG
	while( (n=getopt_long(argc,argv,"v:f:i:d:g:q:t:ST:wa:r:sl:mUBR:c:n:b:C:F:",
		long_options, &option_index)) != EOF)
#else
	while( (n=getopt(argc,argv,"v:f:i:d:g:q:t:ST:wa:r:sl:mUBR:c:n:b:C:F:")) != EOF)
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
	if (getenv("LAV_VIDEO_DEV")) info->video_dev = getenv("LAV_VIDEO_DEV");
        else {
            struct stat vstat;
            if((stat("/dev/video", &vstat) == 0) && S_ISCHR(vstat.st_mode)) 
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
	if (getenv("LAV_MIXER_DEV")) info->mixer_dev = getenv("LAV_MIXER_DEV");
        else {
            struct stat mstat;
            if(stat("/dev/mixer", &mstat) == 0 && S_ISCHR(mstat.st_mode)) 
                info->mixer_dev = "/dev/mixer";
            else if(stat("/dev/sound/mixer", &mstat) == 0 && S_ISCHR(mstat.st_mode)) 
                info->mixer_dev = "/dev/sound/mixer";
        }

	if(optind>=argc) nerr++;
	if(nerr) Usage(argv[0]);

	mjpeg_default_handler_verbosity(verbose);

	/* disable audio for jpeg */
	if (info->video_format == 'j') info->audio_size = 0;

	info->num_files = argc - optind;
	info->files = argv + optind;
	/* If the first filename contains a '%', the user wants file patterns */
	if(strstr(argv[optind],"%")) info->num_files = 0;
        /* Try to guess the file type by looking at the extension. */
        if(info->video_format == '\0') {
            if((dotptr = strrchr(argv[optind], '.'))) {
#ifdef HAVE_LIBQUICKTIME
                if (!strcasecmp(dotptr+1, "mov"))
                    info->video_format = 'q';
#endif
                if (!strcasecmp(dotptr+1, "avi")) info->video_format = 'a';
            }
            if(info->video_format == '\0') info->video_format = 'a';
        }
}


/* Simply prints recording parameters */
static void lavrec_print_properties(void)
{
	const char *source;
	char *tmsg;
	mjpeg_info("Recording parameters:");
	if (info->video_format=='q')
	   tmsg = "Quicktime";
	else if (info->video_format=='j')
	   tmsg = "JPEG";
	else
	   tmsg = "AVI";
	mjpeg_info("Output format:      %s", tmsg);

	switch(input_source)
	{
		case 'p': source = info->software_encoding?"BT8x8 1st input PAL\n":"Composite PAL\n"; break;
		case 'P': source = info->software_encoding?"BT8x8 2nd input PAL\n":"S-Video PAL\n"; break;
		case 'n': source = info->software_encoding?"BT8x8 1st input NTSC\n":"Composite NTSC\n"; break;
	        case 'N': source = info->software_encoding?"BT8x8 2nd input PAL\n":"S-Video NTSC\n"; break;
		case 's': source = info->software_encoding?"BT8x8 1st input SECAM\n":"Composite SECAM\n"; break;
		case 'S': source = info->software_encoding?"BT8x8 2nd input PAL\n":"S-Video SECAM\n"; break;
		case 't': source = info->software_encoding?"BT8x8 3rd input PAL\n":"PAL TV-tuner\n"; break;
		case 'T': source = info->software_encoding?"BT8x8 3rd input PAL\n":"NTSC TV-tuner\n"; break;
		case 'f': source = info->software_encoding?"BT8x8 3rd input PAL\n":"SECAM TV-tuner\n"; break;
//sam Fix numeric source
		default:  source = "Auto detect\n";
	}
	mjpeg_info("Input Source:       %s", source);

	if(info->horizontal_decimation == info->vertical_decimation)
		mjpeg_info("Decimation:         %d",
			info->horizontal_decimation);
	else
		mjpeg_info("Decimation:         %d (hor) x %d (ver)",
			info->horizontal_decimation, info->vertical_decimation);
	
	mjpeg_info("Quality:            %d",info->quality);
	mjpeg_info("Recording time:     %d sec",info->record_time);
	if(info->time_lapse>1)
		mjpeg_info("Time lapse factor:  %d",info->time_lapse);
	
	mjpeg_info(" ");
	mjpeg_info("MJPEG buffer size:  %d KB",info->MJPG_bufsize);
	mjpeg_info("# of MJPEG buffers: %d",info->MJPG_numbufs);
	if(info->audio_size)
	{
		mjpeg_info("Audio parameters:");
		mjpeg_info("Audio sample size:           %d bit",info->audio_size);
		mjpeg_info("Audio sampling rate:         %d Hz",info->audio_rate);
		mjpeg_info("Audio is %s",info->stereo ? "STEREO" : "MONO");
		if(info->audio_level!=-1)
		{
			mjpeg_info("Audio input recording level: %d%%",
				info->audio_level);
			mjpeg_info("%s audio output during recording",
					   info->mute?"Mute":"Don\'t mute");
			mjpeg_info("Recording source: %c",info->audio_src);
		}
			else
				mjpeg_info("Audio input recording level: Use mixer setting");
		mjpeg_info("Level of correction for Audio/Video synchronization:");
		switch(info->sync_correction)
		{
		case 0: mjpeg_info("No lost frame compensation, No frame drop/insert");
			break;
		case 1: mjpeg_info("Lost frame compensation, No frame drop/insert"); 
			break;
		case 2: mjpeg_info("Lost frame compensation and frame drop/insert"); 
			break;
		}
	}
	else
		mjpeg_info("Audio disabled");
}

static void print_summary(void)
{
  MPEG_timecode_t tc;

  mpeg_timecode(&tc, num_frames, ((info->video_norm==1)?4:3),
    ((info->video_norm==1)?(30000/1001):25.));

  printf("Recording time  : %2d.%2.2d.%2.2d:%2.2d\n"
         "Lost frames     : %d\n"
         "A/V sync ins/del: %d/%d\n"
         "Audio errors    : %d\n",
    tc.h, tc.m, tc.s, tc.f,
    num_lost, num_ins, num_del, num_aerr);
}

static void *
signal_catcher_thread (void * arg)
{
   sigset_t * sigs = arg;
   int sig;

   sigwait(sigs, &sig);
   lavrec_stop(info);
   return 0;
}

static void
signal_forwarder (int sig)
{
   pthread_kill(signal_thread, sig);
}

static void
forward_signals (sigset_t * sigs)
{
   struct sigaction sa;
   int sig;
   
   sa.sa_handler = signal_forwarder;
   sigfillset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;

   for (sig = 1; sig < 32; sig++)
      if (sigismember(sigs, sig) > 0)
	 sigaction(sig, &sa, 0);

   pthread_sigmask(SIG_UNBLOCK, sigs, 0);
}

int main(int argc, char **argv)
{
  sigset_t sigmask;
  pthread_t input_thread;

  /* no root please (only during audio setup) */
  if (getuid() != geteuid())
  {
    if (setfsuid(getuid()) < 0)
    {
      mjpeg_error("Failed to set filesystem user ID: %s",
        strerror(errno));
      return 0;
    }
  }

  info = lavrec_malloc();
  info->state_changed = statechanged;
  info->msg_callback = input;
  check_command_line_options(argc, argv);
  lavrec_print_properties();

/*  if( batch_mode ) */
/*	  info->output_statistics = batch_stats; */
/*  else */
	  info->output_statistics = output_stats;

  sem_init(&state_changed, 0, 0);
  state = LAVREC_STATE_PAUSED;

  /* Block all signals */	  
  sigfillset(&sigmask);
  pthread_sigmask(SIG_SETMASK, &sigmask, 0);

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGHUP);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);

  /* Start signal catching thread */
  pthread_create(&signal_thread, NULL, signal_catcher_thread, &sigmask);
  /* Catch and forward selected signals to the signal catcher */
  forward_signals(&sigmask);

  if ((lavrec_main(info)) != 1)
		{
			mjpeg_error_exit1("Something went wrong while setting up the card");
	  		print_summary();
			lavrec_free(info);
		}

  if (!wait_for_start)
     lavrec_start(info);
  if (!batch_mode)
     pthread_create(&input_thread, NULL, user_input_thread, NULL);

  lavrec_busy(info);

  pthread_cancel(signal_thread);
  pthread_join(signal_thread, 0);
  if (!batch_mode)
  {
     pthread_cancel(input_thread);
     pthread_join(input_thread, 0);
  }
  
  sem_destroy(&state_changed);
  fprintf(stderr, "\n");
  print_summary();
  lavrec_free(info);

  return 0;
}
