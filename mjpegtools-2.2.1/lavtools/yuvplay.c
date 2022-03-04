/*
 *  yuvplay - play YUV data using SDL
 *
 *  Copyright (C) 2000, Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "yuv4mpeg.h"
#include "mjpeg_logging.h"
#include <mpegconsts.h>
#include <mpegtimecode.h>
#include <SDL.h>
#include <sys/time.h>


/* SDL variables */
SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

static int got_sigint = 0;

static void usage (void) {
  fprintf(stderr, "Usage: lavpipe/lav2yuv... | yuvplay [options]\n"
	  "  -s : display size, width x height\n"
	  "  -t : set window title\n"
	  "  -f : frame rate (overrides rate in stream header)\n"
          "  -c : don't sync on frames - plays at stream speed\n"
	  "  -v : verbosity {0, 1, 2} [default: 1]\n"
	  );
}

static void sigint_handler (int signal) {
   mjpeg_warn("Caught SIGINT, exiting...");
   got_sigint = 1;
}

static long get_time_diff(struct timeval time_now) {
   struct timeval time_now2;
   gettimeofday(&time_now2,0);
   return time_now2.tv_sec*1.e6 - time_now.tv_sec*1.e6 + time_now2.tv_usec - time_now.tv_usec;
}

static char *print_status(int frame, double framerate) {
   MPEG_timecode_t tc;
   static char temp[256];

   mpeg_timecode(&tc, frame,
		 mpeg_framerate_code(mpeg_conform_framerate(framerate)),
		 framerate);
   sprintf(temp, "%d:%2.2d:%2.2d.%2.2d", tc.h, tc.m, tc.s, tc.f);
   return temp;
}

int main(int argc, char *argv[])
{
   int verbosity = 1;
   double time_between_frames = 0.0;
   double frame_rate = 0.0;
   struct timeval time_now;
   int n, frame;
   unsigned char *yuv[3];
   int in_fd = 0;
   int screenwidth=0, screenheight=0;
   y4m_stream_info_t streaminfo;
   y4m_frame_info_t frameinfo;
   int frame_width;
   int frame_height;
   int wait_for_sync = 1;
   char *window_title = NULL;

   while ((n = getopt(argc, argv, "hs:t:f:cv:")) != EOF) {
      switch (n) {
         case 'c':
            wait_for_sync = 0;
            break;
         case 's':
            if (sscanf(optarg, "%dx%d", &screenwidth, &screenheight) != 2) {
               mjpeg_error_exit1( "-s option needs two arguments: -s 10x10");
               exit(1);
            }
            break;
	  case 't':
	    window_title = optarg;
	    break;
	  case 'f':
		  frame_rate = atof(optarg);
		  if( frame_rate <= 0.0 || frame_rate > 200.0 )
			  mjpeg_error_exit1( "-f option needs argument > 0.0 and < 200.0");
		  break;
          case 'v':
	    verbosity = atoi(optarg);
	    if ((verbosity < 0) || (verbosity > 2))
	      mjpeg_error_exit1("-v needs argument from {0, 1, 2} (not %d)",
				verbosity);
	    break;
	  case 'h':
	  case '?':
            usage();
            exit(1);
            break;
         default:
            usage();
            exit(1);
      }
   }

   mjpeg_default_handler_verbosity(verbosity);

   y4m_accept_extensions(1);
   y4m_init_stream_info(&streaminfo);
   y4m_init_frame_info(&frameinfo);
   if ((n = y4m_read_stream_header(in_fd, &streaminfo)) != Y4M_OK) {
      mjpeg_error("Couldn't read YUV4MPEG2 header: %s!",
         y4m_strerr(n));
      exit (1);
   }

   switch (y4m_si_get_chroma(&streaminfo)) {
   case Y4M_CHROMA_420JPEG:
   case Y4M_CHROMA_420MPEG2:
   case Y4M_CHROMA_420PALDV:
     break;
   default:
     mjpeg_error_exit1("Cannot handle non-4:2:0 streams yet!");
   }

   frame_width = y4m_si_get_width(&streaminfo);
   frame_height = y4m_si_get_height(&streaminfo);

   if ((screenwidth <= 0) || (screenheight <= 0)) {
     /* no user supplied screen size, so let's use the stream info */
     y4m_ratio_t aspect = y4m_si_get_sampleaspect(&streaminfo);
       
     if (!(Y4M_RATIO_EQL(aspect, y4m_sar_UNKNOWN))) {
       /* if pixel aspect ratio present, use it */
#if 1
       /* scale width, but maintain height (line count) */
       screenheight = frame_height;
       screenwidth = frame_width * aspect.n / aspect.d;
#else
       if ((frame_width * aspect.d) < (frame_height * aspect.n)) {
	 screenwidth = frame_width;
	 screenheight = frame_width * aspect.d / aspect.n;
       } else {
	 screenheight = frame_height;
	 screenwidth = frame_height * aspect.n / aspect.d;
       }
#endif
     } else {
       /* unknown aspect ratio -- assume square pixels */
       screenwidth = frame_width;
       screenheight = frame_height;
     }
   }

   /* Initialize the SDL library */
   if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
      mjpeg_error("Couldn't initialize SDL: %s", SDL_GetError());
      exit(1);
   }

   /* set window title */
   SDL_WM_SetCaption(window_title, NULL);

   /* yuv params */
   yuv[0] = malloc(frame_width * frame_height * sizeof(unsigned char));
   yuv[1] = malloc(frame_width * frame_height / 4 * sizeof(unsigned char));
   yuv[2] = malloc(frame_width * frame_height / 4 * sizeof(unsigned char));

   screen = SDL_SetVideoMode(screenwidth, screenheight, 0, SDL_SWSURFACE);
   if ( screen == NULL ) {
      mjpeg_error("SDL: Couldn't set %dx%d: %s",
		  screenwidth, screenheight, SDL_GetError());
      exit(1);
   }
   else {
      mjpeg_debug("SDL: Set %dx%d @ %d bpp",
		  screenwidth, screenheight, screen->format->BitsPerPixel);
   }

   /* since IYUV ordering is not supported by Xv accel on maddog's system
    *  (Matrox G400 --- although, the alias I420 is, but this is not
    *  recognized by SDL), we use YV12 instead, which is identical,
    *  except for ordering of Cb and Cr planes...
    * we swap those when we copy the data to the display buffer...
    */
   yuv_overlay = SDL_CreateYUVOverlay(frame_width, frame_height,
				      SDL_YV12_OVERLAY,
				      screen);
   if ( yuv_overlay == NULL ) {
      mjpeg_error("SDL: Couldn't create SDL_yuv_overlay: %s",
		      SDL_GetError());
      exit(1);
   }
   if ( yuv_overlay->hw_overlay ) 
     mjpeg_debug("SDL: Using hardware overlay.");

   rect.x = 0;
   rect.y = 0;
   rect.w = screenwidth;
   rect.h = screenheight;

   SDL_DisplayYUVOverlay(yuv_overlay, &rect);

   signal (SIGINT, sigint_handler);

   frame = 0;
   if ( frame_rate == 0.0 ) 
   {
	   /* frame rate has not been set from command-line... */
	   if (Y4M_RATIO_EQL(y4m_fps_UNKNOWN, y4m_si_get_framerate(&streaminfo))) {
	     mjpeg_info("Frame-rate undefined in stream... assuming 25Hz!" );
	     frame_rate = 25.0;
	   } else {
	     frame_rate = Y4M_RATIO_DBL(y4m_si_get_framerate(&streaminfo));
	   }
   }
   time_between_frames = 1.e6 / frame_rate;

   gettimeofday(&time_now,0);

   while ((n = y4m_read_frame(in_fd, &streaminfo, &frameinfo, yuv)) == Y4M_OK && (!got_sigint)) {

      /* Lock SDL_yuv_overlay */
      if ( SDL_MUSTLOCK(screen) ) {
         if ( SDL_LockSurface(screen) < 0 ) break;
      }
      if (SDL_LockYUVOverlay(yuv_overlay) < 0) break;

      /* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
      memcpy(yuv_overlay->pixels[0], yuv[0], frame_width * frame_height);
      memcpy(yuv_overlay->pixels[1], yuv[2], frame_width * frame_height / 4);
      memcpy(yuv_overlay->pixels[2], yuv[1], frame_width * frame_height / 4);

      /* Unlock SDL_yuv_overlay */
      if ( SDL_MUSTLOCK(screen) ) {
         SDL_UnlockSurface(screen);
      }
      SDL_UnlockYUVOverlay(yuv_overlay);

      /* Show, baby, show! */
      SDL_DisplayYUVOverlay(yuv_overlay, &rect);
      mjpeg_info("Playing frame %4.4d - %s",
		 frame, print_status(frame, frame_rate));

      if (wait_for_sync)
         while(get_time_diff(time_now) < time_between_frames) {
            usleep(1000);
         }
      frame++;

      gettimeofday(&time_now,0);
   }

   if ((n != Y4M_OK) && (n != Y4M_ERR_EOF))
      mjpeg_error("Couldn't read frame: %s", y4m_strerr(n));

   for (n=0; n<3; n++) {
      free(yuv[n]);
   }

   mjpeg_info("Played %4.4d frames (%s)",
	      frame, print_status(frame, frame_rate));

   SDL_FreeYUVOverlay(yuv_overlay);
   SDL_Quit();

   y4m_fini_frame_info(&frameinfo);
   y4m_fini_stream_info(&streaminfo);
   return 0;
}

