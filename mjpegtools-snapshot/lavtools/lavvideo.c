/*
lavvideo (V1.0)
===============

Small tool for showing a realtime video window on the screen,
but (simplification) _ignoring_ all window managing.

Usage: lavvideo [options]
Have a look at the options below. Nothing is guaranteed to be working.

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

Copyright by Gernot Ziegler.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DEBUG(x) x
#define VERBOSE(x) x

/* Here you can enter your own framebuffer address, 
 * NULL means autodetect by V4L driver 
 */
#define FRAMEBUFFERADDRESS NULL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/types.h>
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

#include <videodev_mjpeg.h>
#include <frequencies.h>
#include <termios.h>

#define DEVNAME "/dev/video0"

static const char *device   = DEVNAME;

int v4ldev;
int file;
int res;
int frame;
int chlist = -1;
struct video_window vw;
struct video_channel vc;
struct video_mbuf vb;
struct video_mmap vm;
struct video_buffer vv;
char * buff;

static int norm = VIDEO_MODE_PAL;
static int wait = -1;
static int input = 0;
static int xres = 800;
static int pixbits = 32;
static int width = 400;
static int height = 300;
static int xoffset = 5;
static int yoffset = 5;
static int verbose = 0;
static int debug = 0;

void usage(char *prog);
int doIt(void);
void tune_to_channel(int channel);

void tune_to_channel(int channel)
{
  unsigned long outfreq;

  /* if we are a tuner set the channel */
  if (vc.flags & VIDEO_VC_TUNER)
  {
	if (chlist >= 0)
	{
		outfreq = (chanlists[chlist].list)[channel-1].freq*16/1000;
		fprintf(stdout, "Setting channel to %d frequency %d\n", channel, (chanlists[chlist].list)[channel-1].freq);
  		/* Tune to channel */
		if (ioctl(v4ldev, VIDIOCSFREQ, &outfreq) < 0)
		{
			perror("VIDIOCSFREQ failed.");
			exit(1);
		}
	}
	else
		fprintf(stderr, "No channel list selected!\n");
  }
  else
	  fprintf(stderr, "Not a tunable device!\n");
}

void usage(char *prog)
{
    char *h;

    if (NULL != (h = (char *)strrchr(prog,'/')))
	prog = h+1;
    
    fprintf(stderr,
	    "%s shows an overlay video window on the screen.\n"
	    "\n"
	    "usage: %s [ options ]\n"
	    "\n"
	    "options: [defaults]\n"
	    "  -h          help	       \n"
	    "  -v          verbose operation	       \n"
	    "  -d          debug information	       \n"
            "  -c device   specify device              [%s]\n"
            "  -t seconds  the time the video window shows [infinite]\n"
            "  -s size     specify size                [%dx%d]\n"
            "  -o offset   displacement on screen      [%dx%d]\n"
            "  -x xres     screen x resolution          [%d]\n"
            "  -b pixbits  Bits per pixel (15,16,24,32) [%d]\n"
	    "  -n tvnorm   set pal/ntsc/secam          [pal]\n"
	    "  -i input    set input source (int)      [%d]\n"
	    "  -C chanlist channel list			\n"
	    "\n"
	    "NOTE: lavvideo makes your V4L card write _directly_ into the frame buffer, "
            "ignoring\n _everything_ that lies under it. It is far from being as "
	    "comfortable as \ne.g. xawtv is, and just serving as a simple test and "
            "demo program."
	    "\n"
	    "examples:\n"
	    "  %s  | shows a video window with the default config.\n"
	    "  %s -c /dev/v4l0 -n ntsc | utilizes the device v4l0 for V4L I/O\n"
            "  communication, and expects the signal being NTSC standard.\n"
	    "\n"
	    "--\n"
	    "(c) 2000 Gernot Ziegler<gz@lysator.liu.se>\n",
	    prog, prog, device, width, height, xoffset, yoffset, xres, pixbits, 
	    input, prog, prog);
}

int doIt(void)
{ 
  int turnon;

  /* V4l initialization */

  /* Figure out which device to use */
  struct stat vstat;
  if((stat("/dev/video", &vstat) == 0) && S_ISCHR(vstat.st_mode)) 
      device = "/dev/video";
  else if(stat("/dev/video0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
      device = "/dev/video0";
  else if(stat("/dev/v4l/video0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
      device = "/dev/v4l/video0";
  else if(stat("/dev/v4l0", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
      device = "/dev/v4l0";
  else if(stat("/dev/v4l", &vstat) == 0 && S_ISCHR(vstat.st_mode)) 
      device = "/dev/v4l";
  /* open device */
  if (verbose) printf("Opening device %s\n", device);
  v4ldev = open(device, O_RDWR);
  if (v4ldev < 0) { perror("Error opening video device."); exit(1); }

  /* select input */
  if (verbose) printf("Selecting input %d\n", input);
  vc.channel = input;
  vc.norm = norm;
  res = ioctl(v4ldev, VIDIOCSCHAN, &vc);
  if (res < 0) 
  {
    perror("VIDIOCSCHAN failed.");
    exit(1);
  }

  if (verbose) printf("Choosing window width [%dx%d] and offset [%dx%d]\n, "
		      "bitdepth %d", 
		      width, height, xoffset, yoffset, pixbits);
  vv.width = width;
  vv.height = height;
  if (pixbits == 15)
    vv.bytesperline = xres*16/8;
  else
    vv.bytesperline = xres*pixbits/8;
  vv.depth = pixbits;
  vv.base = NULL; 
  
  res = ioctl(v4ldev, VIDIOCSFBUF, &vv);
  if (res < 0) { perror("VIDIOCSFBUF failed."); exit(1); }

   /* set up window parameters */
  vw.x = xoffset; 
  vw.y = yoffset; 
  vw.width = width;
  vw.height = height;
  vw.chromakey = 0;
  vw.flags = 0;
  vw.clips = NULL;
  vw.clipcount = 0;
  
  res = ioctl(v4ldev, VIDIOCSWIN, &vw);
  if (res < 0) 
    { 
      perror("VIDIOCSWIN");
      printf("\n(Is your videocard listed in videodev.h in the v4l driver ?\n");  
      exit(1); 
    }
  
  if (verbose) printf("Turning on the video window !\n");
  turnon = 1;
  res = ioctl(v4ldev, VIDIOCCAPTURE, &turnon);
  if (res < 0) { perror("v4l: VIDIOCCAPTURE"); exit(1); }

  if (wait == -1) 
  {
	unsigned char input;
	struct timeval tv;
	fd_set rfds;
	int channel = 2;
  	struct termios modes, savemodes;

	if (verbose) printf("Changing keyboard mode.\n");

	/* save old keyboard mode and set new one */
	tcgetattr(0,&modes);
	savemodes = modes;
	modes.c_lflag &= ~ICANON;
	modes.c_lflag &= ~ECHO;
	tcsetattr(0,0,&modes);

	printf("Entering read loop, press q or [ESC] to quit.\n");
        while(read(0, &input, 1) > 0)
        {
		if (input == 'q')
			break;
		if (input == 0x1B) /* ESC */
		{
			/* Since up and down are escaped, we have to read again */
			FD_ZERO(&rfds);
			FD_SET(0, &rfds);
			tv.tv_sec= 0;
			tv.tv_usec = 500;
			if (select(1, &rfds, NULL, NULL, &tv))
			{
				if (read(0, &input, 1) < 0)
					break;
				if (read(0, &input, 1) < 0)
					break;
				if (input == 0x41)
  					tune_to_channel(++channel);
				else
				if (input == 0x42)
				{
					if (channel > 2)
  						tune_to_channel(--channel);
				}
			}
			else /* If the just hit escape, then exit */
				break;
		}
	}

	if (verbose) printf("Restoring keyboard mode.\n");

	/* restore keyboard mode */
	tcsetattr(0,0,&savemodes);
  }
  else
  {
  	if (verbose) printf("Alright, waiting %d seconds.", wait);
  	usleep(wait*1000000);
  }

  if (verbose) printf("Turning off the video window.\n");
  turnon = 0;
  res = ioctl(v4ldev, VIDIOCCAPTURE, &turnon);
  if (res < 0) { perror("v4l: VIDIOCCAPTURE"); exit(1); }

  return 1;
}

int main(int argc,char *argv[]) 
{ int c;

  /* parse options */
  for (;;) 
  {
    if (-1 == (c = getopt(argc, argv, "Svds:o:m:c:n:i:t:b:x:C:h")))
      break;

    switch (c) {
    case 'v':
      verbose = 1;
      break;
    case 'd':
      debug = 1;
      break;
    case 's':
      if (2 != sscanf(optarg,"%dx%d",&width,&height))
	{ width = 400; height = 300; }
      break;
    case 'o':
      if (2 != sscanf(optarg,"%dx%d",&xoffset,&yoffset))
	{ width = 5; height = 5; }
      break;
    case 'c':
      device = optarg;
      break;
    case 'n':
      if (0 == strcasecmp(optarg,"pal"))
	norm = VIDEO_MODE_PAL;
      else if (0 == strcasecmp(optarg,"ntsc"))
	norm = VIDEO_MODE_NTSC;
      else if (0 == strcasecmp(optarg,"secam"))
	norm = VIDEO_MODE_SECAM;
      else 
	{
	fprintf(stderr,"unknown tv norm %s\n",optarg);
	exit(1);
	}
      break;
    case 'i':
      input = atoi(optarg);
	    break;
    case 't':
      wait = atoi(optarg);
	    break;
    case 'b':
      pixbits = atoi(optarg);
	    break;
    case 'x':
      xres = atoi(optarg);
	    break;
    case 'C':
      chlist = 0;
      while (strcmp(chanlists[chlist].name, optarg)!=0)
      {
	      chlist++;
	      if (chanlists[chlist].name == NULL)
	      {
		      fprintf(stderr, "unable to find channel list for %s\n", optarg);
		      exit(1);
	      }
      }
      	    break;
    case 'h':
    default:
      usage(argv[0]);
      exit(1);
    }
  }
    
  doIt();

  return 0;
}




