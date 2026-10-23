/*
    testrec - test RECording for lavtools.  Used to hunt down driver bugs.

    Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
    Extended by: Gernot Ziegler <gz@lysator.liu.se>
    Patched '2000 by Wolfgang Scherr <scherr@net4you.net>
    Works fine with Miro DC10(plus), too.


    Capture motion video from the IOMEGA Buz to an AVI or Quicktime
    file together with capturing audio from the soundcard.

    Usage: testrec [options]

    where options are as follows:


   -a num        Audio size in bits, must be 0 (no audio), 8 or 16 (default)

   -r num        Audio rate, must be a permitted sampling rate for your soundcard
                 default is 22050.

   -s            enable stereo (disabled by default)

   -l num        Audio level to use for recording, must be beetwen 0 and 100
                 or -1 (for not touching the mixer settings at all), default is 100

                 Only if mixer is used (l != -1):

   -m            Mute audio output during recording (default is to let it enabled).
                 This is particularly usefull if you are recording with a
                 microphone to avoid feedback.

   -R [lmc]      Recording source:
                 l: line
                 m: microphone
                 c: cdrom

   *** Environment variables ***

   LAV_VIDEO_DEV     Name of video device (default: "/dev/video")
   LAV_AUDIO_DEV     Name of audio device (default: "/dev/dsp")
   LAV_MIXER_DEV     Name of mixer device (default: "/dev/mixer")


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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/vfs.h>

#include <signal.h>
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
#include <linux/soundcard.h>

/* These are explicit prototypes for the compiler, to prepare separation of audiolib.c */
void audio_shutdown(void);
int audio_init(int a_read, int a_stereo, int a_size, int a_rate);
long audio_get_buffer_size(void);
int audio_read(char *buf, int size, int swap, struct timeval *tmstmp, int *status);

char *audio_strerror(void);

void system_error(char *str1,char *str2);
void set_mixer(int flag);
void CleanUpAudio(void);
void SigHandler(int sig_num);
void Usage(char *progname);


/* Set the default options here */


static int audio_size    = 16;          /* size of an audio sample: 8 or 16 bits,
                                           0 for no audio */
static int audio_rate    = 44100;       /* sampling rate for audio */
static int stereo        = 0;           /* 0: capture mono, 1: capture stereo */
static int audio_lev     = 100;         /* Level of Audio input,
										   0..100: Recording level
                                           -1:  don't change mixer settings */
static int audio_mute    = 0;           /* Flag for muting audio output */
static int audio_recsrc  = 'l';         /* Audio recording source */

static int verbose	= 2;
/* On some systems MAP_FAILED seems to be missing */

#ifndef MAP_FAILED
#define MAP_FAILED ( (caddr_t) -1 )
#endif


#define NUM_AUDIO_TRIES 500 /* makes 10 seconds with 20 ms pause beetween tries */

static int audio_bps; /* audio bytes per sample */


static char infostring[4096];

#define LAVREC_INTERNAL 0
#define LAVREC_DEBUG    1
#define LAVREC_INFO     2
#define LAVREC_WARNING  3
#define LAVREC_ERROR    4
#define LAVREC_PROGRESS 5

static int need_newline=0;

static void lavrec_msg(int type, const char *str1, const char *str2)
{
	const char *ctype;

	switch(type) {

	case LAVREC_DEBUG:
		if (verbose < 3)
			return;
		break;

	case LAVREC_INFO:
		if (verbose < 1)
			return;
		break;

	case LAVREC_WARNING:
		if (verbose < 2)
			return;
		break;

	case LAVREC_PROGRESS:
		if (verbose == 0)
			return;
		break;
	}

	if(type==LAVREC_PROGRESS)
	{
		printf("%s   \r",str1);
		fflush(stdout);
		need_newline=1;
	}
	else
	{
		switch(type)
		{
		case LAVREC_INTERNAL: ctype = "Internal Error"; break;
		case LAVREC_DEBUG:    ctype = "Debug";          break;
		case LAVREC_INFO:     ctype = "Info";           break;
		case LAVREC_WARNING:  ctype = "Warning";        break;
		case LAVREC_ERROR:    ctype = "Error";          break;
		default:              ctype = "Unkown";
		}
		if(need_newline) printf("\n");
		printf("%s: %s\n",ctype,str1);
		if(str2[0]) printf("%s: %s\n",ctype,str2);
		need_newline=0;
	}
}

/* system_error: report an error from a system call */

void system_error(char *str1,char *str2)
{
	sprintf(infostring,"Error %s (in %s)",str1,str2);
	lavrec_msg(LAVREC_ERROR,infostring,strerror(errno));
	exit(1);
}

static int mixer_set = 0;
static int mixer_volume_saved = 0;
static int mixer_recsrc_saved = 0;
static int mixer_inplev_saved = 0;

/*
   Set the sound mixer:
   flag = 1 : set for recording from the line input
   flag = 0 : restore previously saved values
*/

void set_mixer(int flag)
{
	int fd, recsrc, level, status, numerr;
	int sound_mixer_read_input;
	int sound_mixer_write_input;
	int sound_mask_input;
	const char *mixer_dev_name;

	/* Avoid restoring anything when nothing was set */

	if (flag==0 && mixer_set==0) return;

	mixer_dev_name = getenv("LAV_MIXER_DEV");
	if(!mixer_dev_name) mixer_dev_name = "/dev/mixer";
	fd = open(mixer_dev_name, O_RDONLY);
	if (fd == -1)
	{
		sprintf(infostring,"Unable to open sound mixer %s", mixer_dev_name);
		lavrec_msg(LAVREC_WARNING, infostring,
				   "Try setting the sound mixer with another tool!!!");
		return;
	}

	mixer_set = 1;

	switch(audio_recsrc)
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
	default :
		sound_mixer_read_input  = SOUND_MIXER_READ_LINE;
		sound_mixer_write_input = SOUND_MIXER_WRITE_LINE;
		sound_mask_input        = SOUND_MASK_LINE;
		break;
	}

	if(flag)
	{
		/* Save the values we are going to change */

		numerr = 0;
		status = ioctl(fd, SOUND_MIXER_READ_VOLUME, &mixer_volume_saved);
		if (status == -1) numerr++;
		status = ioctl(fd, SOUND_MIXER_READ_RECSRC, &mixer_recsrc_saved);
		if (status == -1) numerr++;
		status = ioctl(fd, sound_mixer_read_input , &mixer_inplev_saved);
		if (status == -1) numerr++;
		if (numerr) 
		{
			lavrec_msg(LAVREC_WARNING,
					   "Unable to save sound mixer settings",
					   "Restore your favorite setting with another tool after capture");
			mixer_set = 0; /* Avoid restoring the wrong values */
		}

		/* Set the recording source to the line input, 
		   the level of the line input to audio_lev,
		   the output volume to zero (to avoid audio feedback
		   when using a camera build in microphone */

		numerr = 0;

		recsrc = sound_mask_input;
		status = ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &recsrc);
		if (status == -1) numerr++;

		level = 256*audio_lev + audio_lev; /* left and right channel */
		status = ioctl(fd, sound_mixer_write_input, &level);
		if (status == -1) numerr++;

		if(audio_mute)
		{
			level = 0;
			status = ioctl(fd, SOUND_MIXER_WRITE_VOLUME, &level);
			if (status == -1) numerr++;
		}

		if (numerr) 
		{
			lavrec_msg(LAVREC_WARNING,
					   "Unable to set the sound mixer correctly",
					   "Audio capture might not be successfull (try another mixer tool!)");
		}
	}
	else
	{
		/* Restore previously saved settings */

		numerr = 0;
		status = ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &mixer_recsrc_saved);
		if (status == -1) numerr++;
		status = ioctl(fd, sound_mixer_write_input,  &mixer_inplev_saved);
		if (status == -1) numerr++;
		if(audio_mute)
		{
			status = ioctl(fd, SOUND_MIXER_WRITE_VOLUME, &mixer_volume_saved);
			if (status == -1) numerr++;
		}

		if (numerr) 
		{
			lavrec_msg(LAVREC_WARNING,
					   "Unable to restore sound mixer settings",
					   "Restore your favorite setting with another tool");
		}
	}

	close(fd);

}

/* Shut down the audio task and reset the mixers.
   CleanUpAudio is only called indirectly through atexit()
*/

void CleanUpAudio(void)
{
	if(audio_size) audio_shutdown();
	set_mixer(0);
}

/* The signal handler only sets a flag which makes the main program
   to exit the video recording loop.
   This way we avoid race conditions when ^C is pressed during
   writing to the AVI File
*/

static int VideoExitFlag = 0;

void SigHandler(int sig_num)
{
	VideoExitFlag = 1;
}

void Usage(char *progname)
{
	fprintf(stderr, "lavtools version " VERSION ": lavrec\n");
	fprintf(stderr, "Usage: %s [options]", progname);
	fprintf(stderr, "where options are:\n");
	fprintf(stderr, "   -a num     Audio size, 0 for no audio, 8 or 16\n");
	fprintf(stderr, "   -r num     Audio rate [Hz]\n");
	fprintf(stderr, "   -s         Stereo\n");
	fprintf(stderr, "   -l num     Recording level [%%], -1 for mixers not touched\n");
	fprintf(stderr, "   -m         Mute audio output during recording\n");
	fprintf(stderr, "   -R [lmc]   Set recording source: (l)ine, (m)icro, (c)d\n");
	fprintf(stderr, "Environment variables recognized:\n");
	fprintf(stderr, "   LAV_VIDEO_DEV, LAV_AUDIO_DEV, LAV_MIXER_DEV\n");
	exit(1);
}

static double spas;   /* seconds per audio sample */



int main(int argc, char ** argv)
{
	int res;
	struct timeval audio_t0;
	unsigned long audio_buffer_size;
	char AUDIO_buff[8192];
	int n, nerr;
	int astat;
	int num_asamps, num_aerr;
	int nb;
	struct timeval audio_tmstmp;

	/* check usage */
	if (argc < 2)  Usage(argv[0]);

	/* Get options */

	nerr = 0;
	while( (n=getopt(argc,argv,"a:r:sl:mR:v:")) != EOF)
	{
		switch(n) {

		case 'a':
            audio_size = atoi(optarg);
            if(audio_size != 0 && audio_size != 8 && audio_size != 16)
            {
				fprintf(stderr,"audio_size = %d invalid (must be 0, 8 or 16)\n",
						audio_size);
				nerr++;
            }
            break;

		case 'r':
            audio_rate = atoi(optarg);
            if(audio_rate<=0)
            {
				fprintf(stderr,"audio_rate = %d invalid\n",audio_rate);
				nerr++;
            }
            break;

		case 's':
            stereo = 1;
            break;

		case 'l':
            audio_lev = atoi(optarg);
            if(audio_lev<-1 || audio_lev>100)
            {
				fprintf(stderr,"recording level = %d invalid (must be 0 ... 100 or -1)\n",
						audio_lev);
				nerr++;
            }
            break;

		case 'm':
            audio_mute = 1;
            break;

		case 'R':
            audio_recsrc = optarg[0];
            if(audio_recsrc!='l' && audio_recsrc!='m' && audio_recsrc!='c')
            {
				fprintf(stderr,"Recording source (-R param) must be l,m or c\n");
				nerr++;
            }
            break;


		case 'v':
			verbose = atoi(optarg);
			break;

		default:
            nerr++;
			fprintf(stderr, "Unrecognised option!\n" );
            break;
		}
	}

	if(nerr) Usage(argv[0]);

	if(audio_size)
	{
		printf("\nAudio parameters:\n\n");
		printf("Audio sample size:           %d bit\n",audio_size);
		printf("Audio sampling rate:         %d Hz\n",audio_rate);
		printf("Audio is %s\n",stereo ? "STEREO" : "MONO");
		if(audio_lev!=-1)
		{
			printf("Audio input recording level: %d %%\n",audio_lev);
			printf("%s audio output during recording\n",
				   audio_mute?"Mute":"Don\'t mute");
			printf("Recording source: %c\n",audio_recsrc);
		}
		else
			printf("Audio input recording level: Use mixer setting\n");
	}
	else
		printf("\nAudio disabled\n\n");
	printf("\n");

	/* set the sound mixer */

	if(audio_size && audio_lev>=0) set_mixer(1);

	/* Initialize the audio system if audio is wanted.
	   This involves a fork of the audio task and is done before
	   the video device and the output file is opened */

	audio_bps = 0;
	if (audio_size)
	{
		res = audio_init(1,stereo,audio_size,audio_rate);
		if(res)
		{
			set_mixer(0);
			lavrec_msg(LAVREC_ERROR,"Error initializing Audio",audio_strerror());
			exit(1);
		}
		audio_bps = audio_size/8;
		if(stereo) audio_bps *= 2;
		audio_buffer_size = audio_get_buffer_size();
	}

	/* The audio system needs a exit processing (audio_shutdown()),
	   the mixers also should be reset at exit. */

	atexit(CleanUpAudio);

	/* Try to get a reliable timestamp for Audio */

	if (audio_size)
	{
		lavrec_msg(LAVREC_INFO, "Getting audio ... ", "");

		for(n=0;;n++)
		{
			if(n>NUM_AUDIO_TRIES)
			{
				lavrec_msg(LAVREC_ERROR,"Unable to get audio - exiting ....","");
				exit(1);
			}
			res = audio_read(AUDIO_buff,sizeof(AUDIO_buff),0,&audio_t0,&astat);
			if(res<0)
			{
				lavrec_msg(LAVREC_ERROR,"Error reading audio",audio_strerror());
				exit(1);
			}
			if(res && audio_t0.tv_sec ) break;
			usleep(20000);
		}
	}
         


	/* The test loop */

	num_asamps = 0; /* Number of audio samples written to file */
	num_aerr   = 0; /* Number of audio buffers in error        */

	/* Seconds per audio sample: */
	if(audio_size)
		spas = 1.0/audio_rate;
	else
		spas = 0.;


	while (1)
	{
		usleep(20000); /* 50Hz as if frame sync */
		/* sync on a frame */
		/* Care about audio */

		if (!audio_size) continue;

		res = 0;

		while(1)
		{

			/* Only try to read a audio sample if video is ahead - else we might
			   get into difficulties when writing the last samples */

			nb=audio_read(AUDIO_buff,sizeof(AUDIO_buff),0,&audio_tmstmp,&astat);
			if(nb==0) break;

			if(nb<0)
			{
				lavrec_msg(LAVREC_ERROR,"Error reading audio",audio_strerror());
				res = -1;
				break;
			}

			if(!astat) 
			{
				num_aerr++;
			}



			if(audio_tmstmp.tv_sec)
			{
			}
		}
		if (res) break;
	}

	/* Audio and mixer exit processing is done with atexit() */

	if(res>=0) {
		lavrec_msg(LAVREC_INFO,"Clean exit ...","");
	} else
		lavrec_msg(LAVREC_INFO,"Error exit ...","");
	exit(0);
}
