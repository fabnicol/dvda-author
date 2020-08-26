/*
    lavtrans - takes file arguments like lavplay
               (AVI/Quicktime files and Edit Lists)
               and produces a new file or a bunch of
               JPEG images from that.

    Mostly intended to flaten the content of Edit Lists

    Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
    Additions by Gernot Ziegler <gz@lysator.liu.se>

    Usage: lavtrans [options] filename [filename ...] <-i num>

    where options are as follows:

    
    -o name    Write output to file "name".
               Output must fit into 1 file (2 GB limit!!!)
               unless single images are produced.

    -f [amqiw]  Format of output
               a: AVI (no selection of bottom/top frame first possible)
               q: Quicktime (if the input is interlaced, then all input files
                             must be Quicktime, too)
               i: single images, "name" in the -o option must be a vaild
                  format string for sprintf!
               w: WAV file (of sound only)

    Both options are mandatory!

    -i num     Grab a single frame (num) from the input to the outputfile
               (you need -f i for this!)

    Filename options like in lavplay (optional +p/+n followed by
    an arbitrary number of AVI / Quicktime files or edit lists).



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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "lav_io.h"
#include "editlist.h"
#include "mjpeg_logging.h"

static EditList el;

static char *outfile=0;
static char format=0;

static int process_image_frame = -1; /* All, >-1 means not all */

#define FOURCC(a,b,c,d) ((d<<24) | ((c&0xff)<<16) | ((b&0xff)<<8) | (a&0xff))

#define FOURCC_RIFF     FOURCC ('R', 'I', 'F', 'F')
#define FOURCC_WAVE     FOURCC ('W', 'A', 'V', 'E')
#define FOURCC_FMT      FOURCC ('f', 'm', 't', ' ')
#define FOURCC_DATA     FOURCC ('d', 'a', 't', 'a')

static struct
{
   uint32_t rifftag;
   uint32_t rifflen;
   uint32_t wavetag;
   uint32_t fmt_tag;
   uint32_t fmt_len;
   uint16_t wFormatTag;
   uint16_t nChannels;
   uint32_t nSamplesPerSec;
   uint32_t nAvgBytesPerSec;
   uint16_t nBlockAlign;
   uint16_t wBitsPerSample;
   uint32_t datatag;
   uint32_t datalen;
} wave_hdr;

static uint8_t abuff[16384];
int big_endian;
int	verbose = 1;

/* Prototype definitions */
void Usage(char *str);
void system_error(const char *str1, const char *str2);
void initwav_hdr(void);

void initwav_hdr()
{
wave_hdr.rifflen = 0; /* to be filled later */
wave_hdr.datalen = 0; /* to be filled later */
wave_hdr.wFormatTag = 1;  /* PCM */

     wave_hdr.rifftag = reorder_32(FOURCC_RIFF, big_endian);
     wave_hdr.wavetag = reorder_32(FOURCC_WAVE, big_endian);
     wave_hdr.fmt_tag = reorder_32(FOURCC_FMT, big_endian);
     wave_hdr.fmt_len = reorder_32(16, big_endian);
     wave_hdr.datatag = reorder_32(FOURCC_DATA, big_endian);
     swab(&wave_hdr.wFormatTag, &wave_hdr.wFormatTag ,2);
}

void Usage(char *str)
{
   fprintf(stderr,"Usage: %s -o <outputfile> -f [aqiw] filenames [<-i num>]\n",str);
   fprintf(stderr,"          -f a    output AVI file\n");
   fprintf(stderr,"          -f q    output Quicktime file\n");
   fprintf(stderr,"          -f i    output single JPEG images, "
                  "-o option mut be a valid format string\n");
   fprintf(stderr,"          -f w    output WAV file (sound only!)\n");
   fprintf(stderr,"          -i num  convert single frame to JPEG\n");
   exit(1);
}

void system_error(const char *str1, const char *str2)
{
   mjpeg_error_exit1("%s: %s: %s",str1, str2, strerror(errno));
}

int main(int argc, char ** argv)
{
   lav_file_t *outfd = NULL;
   FILE *wavfd = NULL;
   FILE *imgfd = NULL;
   uint8_t *vbuff;
   char *dotptr;
   char imgfname[4096];
   long audio_bytes_out = 0;
   int res, n, nv = 0, na = 0, nframe;
   int forcestereo = 0;

   while ((n=getopt(argc,argv,"o:f:i:v:")) != EOF)
   {
      switch(n) {
		  
	  case 'o':
		  outfile = optarg;
		  break;
		  
	  case 'f':
		  format = optarg[0];
		  break;
		  
	  case 'i':
		  process_image_frame = atoi(optarg);
		  break;
      case 'v':
		  verbose = atoi (optarg);
		  if (verbose < 0 || verbose >2)
		  {
			  Usage (argv[0]);
			  exit (1);
		  }
		  break;		  
		  
         case '?':
			 Usage(argv[0]);
            exit(1);
      }
   }

   (void)mjpeg_default_handler_verbosity(verbose);

   big_endian = lav_detect_endian();
   if (big_endian < 0)
      exit(1);

   if (outfile==0) Usage(argv[0]);
   if (optind>=argc) Usage(argv[0]);
   if (outfile != 0 && format == 0) {
      if ((dotptr = strrchr(outfile, '.'))) {
#ifdef HAVE_LIBQUICKTIME
            if (!strcasecmp(dotptr+1, "mov"))
               format = 'q';
#endif
            if (!strcasecmp(dotptr+1, "avi")) format = 'a';
            if (!strcasecmp(dotptr+1, "wav")) format = 'w';
         }
      if (format == '\0') format = 'a';
   }
   if (format!='a' && format!='q' && format!='i' && format!='w' && format!='A' && format!='W') Usage(argv[0]);

   if (process_image_frame != -1 && format!='i')
   {
      mjpeg_error_exit1("If you specify \'-i <num>\', you must use jpg (\'-f i\') as output:"
						"   lavtrans -o image.jpg -f i movie.avi -i <frame_num>");
   }

   /* Get and open input files */

   read_video_files(argv + optind, argc - optind, &el,0);


   if (format == 'a' && el.video_inter == Y4M_ILACE_BOTTOM_FIRST) format = 'A';

   if ((format == 'q') && el.video_inter == Y4M_ILACE_BOTTOM_FIRST)
   {
      mjpeg_error_exit1("Output is Quicktime - wrong interlacing order");
   }

   if (format == 'q' || format == 'a' || format == 'A')
   {
      outfd = lav_open_output_file(outfile,format,
                                   el.video_width,el.video_height,el.video_inter, 
                                   el.video_fps /* was:video_norm=='n' ? 30000.0/1001.0 : 25.0*/ ,
                                   el.audio_bits,el.audio_chans,el.audio_rate);
      if (!outfd)
      {
		  mjpeg_error_exit1("Opening output file %s: %s",
					  outfile,lav_strerror());
      }
   }

   if (format == 'w' || format == 'W')
   {
      if (!el.has_audio)
      {
         mjpeg_error_exit1("WAV output requested but no audio present");
      }
      if (format == 'W')
      {
          if (el.audio_bits == 16 && el.audio_chans == 1)
		forcestereo = 1;
          else
          {
			  mjpeg_error_exit1("STEREOFIED WAV output request but non mono 16-bit audio present");
          }
      }
      
      initwav_hdr();
      wave_hdr.nChannels       = forcestereo ? 2 : el.audio_chans;
      wave_hdr.nBlockAlign     = el.audio_bps;
      wave_hdr.wBitsPerSample  = el.audio_bits;

      wave_hdr.nSamplesPerSec  = reorder_32(el.audio_rate, big_endian);
      wave_hdr.nAvgBytesPerSec = reorder_32(el.audio_rate*el.audio_bps, big_endian);
      if (big_endian)
	 {
         swab(&wave_hdr.nChannels, &wave_hdr.nChannels, 2);
         swab(&wave_hdr.nBlockAlign, &wave_hdr.nBlockAlign, 2);
         swab(&wave_hdr.wBitsPerSample, &wave_hdr.wBitsPerSample, 2);
	 }

      wavfd = fopen(outfile,"wb");
      if (wavfd==0) system_error("opening WAV file","fopen");
      res = fwrite(&wave_hdr,sizeof(wave_hdr),1,wavfd);
      if (res!=1) system_error("writing WAV file","fwrite");
   }

   vbuff = (uint8_t*) malloc(el.max_frame_size);
   if (vbuff==0) { mjpeg_error_exit1("malloc failed");  }

   /* Quick hack by Ronald to enable one-frame picture grabbing */
   if (process_image_frame != -1)
   {
      nv = el_get_video_frame(vbuff, process_image_frame, &el);
      sprintf(imgfname,outfile);
      imgfd = fopen(imgfname,"wb");
      if (imgfd==0) system_error("opening image file","fopen");
      res = fwrite(vbuff,nv,1,imgfd);
      if (res!=1) system_error("writing image","fwrite");
      fclose(imgfd);
      mjpeg_info("Frame %d grabbed", process_image_frame);
      exit(1);
   }


   for(nframe=0;nframe<el.video_frames;nframe++)
   {
      if (format!='w') nv = el_get_video_frame(vbuff, nframe, &el);
      if (el.has_audio && format!='i')
         na = el_get_audio_data(abuff, nframe, &el, 0);

      switch(format)
      {
         case 'a':
         case 'A':
         case 'q':
            res = lav_write_frame(outfd,vbuff,nv,1);
            if (el.has_audio && res==0)
               res = lav_write_audio(outfd,abuff,na/el.audio_bps);
            if (res)
            {
               mjpeg_error_exit1("Error writing output: %s",lav_strerror());
            }
            break;

         case 'i':
            sprintf(imgfname,outfile,nframe);
            imgfd = fopen(imgfname,"wb");
            if (imgfd==0) system_error("opening image file","fopen");
            res = fwrite(vbuff,nv,1,imgfd);
            if (res!=1) system_error("writing image","fwrite");
            fclose(imgfd);
            break;

         case 'w':
         case 'W':
	    if (forcestereo)
	    {
		/* Double up the samples */
	        int i;
                short *sbuff = (short *)abuff;
                for (i = na-1; i >= 0; --i)
                  {
		  sbuff[i+i+1]=sbuff[i+i]=sbuff[i];
                  }
		na = na + na;
                res = fwrite(abuff,na,1,wavfd);
 	    }
            else
            {
              res = fwrite(abuff,na,1,wavfd);
            }
            if (res!=1) system_error("writing WAV file","fwrite");
            audio_bytes_out += na;
            break;
      }
   }

   if (format == 'q' || format == 'a' || format == 'A')
   {
      res = lav_close(outfd);
      if (res)
      {
         mjpeg_error_exit1("Closing output file: %s",lav_strerror());
      }
   }

   if (format == 'w' || format == 'W')
   {
     wave_hdr.rifflen = reorder_32(sizeof(wave_hdr) -8+ audio_bytes_out, big_endian);
      wave_hdr.datalen = reorder_32(audio_bytes_out, big_endian);
      res = fseek(wavfd,0,SEEK_SET);
      if (res) system_error("writing WAV file","fseek");
      res = fwrite(&wave_hdr,sizeof(wave_hdr),1,wavfd);
      if (res!=1) system_error("writing WAV file","fwrite");
      fclose(wavfd);
   }
   return 0;
}
