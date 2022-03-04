/*
 *  yuv2lav - write YUV4MPEG data stream from stdin to mjpeg file
 *
 *  Copyright (C) 2000, pHilipp Zabel <pzabel@gmx.de>
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include "mjpeg_logging.h"

#include "yuv4mpeg.h"
#include "jpegutils.h"
#include "lav_io.h"

#define FOURCC(a,b,c,d) ( (d<<24) | ((c&0xff)<<16) | ((b&0xff)<<8) | (a&0xff) )

#define FOURCC_RIFF     FOURCC ('R', 'I', 'F', 'F')
#define FOURCC_WAVE     FOURCC ('W', 'A', 'V', 'E')
#define FOURCC_FMT      FOURCC ('f', 'm', 't', ' ')
#define FOURCC_DATA     FOURCC ('d', 'a', 't', 'a')


static int   param_quality = 80;
static char  param_format = 'x'; /* a format we don't know */
static char *param_output = 0;
static char *param_inputwav = NULL;
static int   param_bufsize = 256*1024; /* 256 kBytes */
static int   param_interlace = -1;
static int   param_maxfilesize = 0;

static int got_sigint = 0;

static int verbose = 1;

static void usage(void) 
{
  fprintf(stderr,
	  "Usage:  yuv2lav [params] -o <filename>\n"
	  "where possible params are:\n"
	  "   -v num      Verbosity [0..2] (default 1)\n"
	  "   -f [aA"
#ifdef HAVE_LIBQUICKTIME
                   "q"
#else
                   " "
#endif
                    " "
                     "]   output format (AVI"
#ifdef HAVE_LIBQUICKTIME
                                           "/Quicktime"
#endif
                                                            ") [%c]\n"
	  "   -I num      force output interlacing 0:no 1:top 2:bottom field first\n"
	  "   -q num      JPEG encoding quality [%d%%]\n"
	  "   -b num      size of MJPEG buffer [%d kB]\n"
          "   -m num      maximum size per file [%d MB]\n"
	  "   -w file     WAVE file - audio data to be added to output file\n"
	  "   -o file     output mjpeg file (REQUIRED!)\n",
	  param_format, param_quality, param_bufsize/1024, param_maxfilesize);
}

static void sigint_handler (int signal) {
 
   mjpeg_debug( "Caught SIGINT, exiting...");
   got_sigint = 1;
   
}

int main(int argc, char *argv[])
{

   int frame;
   int fd_in;
   int wav_fd = 0;
   long data[64];
   int fmtlen;
   double fps;
   long audio_samps = 0;
   long audio_rate = 0;
   int  audio_chans = 0;
   int  audio_bits = 0;
   int  audio_bps = 1;
   double absize = 0.0;
   long asize = 0;
   uint8_t *abuff = NULL;
   long na_out = 0;
   int n;
   lav_file_t *output = 0;
   int filenum = 0;
   unsigned long filesize_cur = 0;
	char *dotptr;
   
   uint8_t *jpeg;
   int   jpegsize = 0;
   unsigned char *yuv[3];

   y4m_frame_info_t frameinfo;
   y4m_stream_info_t streaminfo;


   while ((n = getopt(argc, argv, "v:f:I:q:b:m:o:w:")) != -1) {
      switch (n) {
      case 'v':
         verbose = atoi(optarg);
         if (verbose < 0 || verbose > 2) {
            mjpeg_error( "-v option requires arg 0, 1, or 2");
			usage();
         }
         break;

      case 'f':
         switch (param_format = optarg[0]) {
         case 'a':
         case 'A':
#ifdef HAVE_LIBQUICKTIME
         case 'q':
#endif
            /* do interlace setting here? */
            continue;
         default:
            mjpeg_error( "-f parameter must be one of [aA"
#ifdef HAVE_LIBQUICKTIME
                                                        "q"
#endif
                                                          "]");
            usage ();
            exit (1);
         }
         break;
      case 'I':
	 param_interlace = atoi(optarg);
         if (2 < param_interlace) {
            mjpeg_error("-I parameter must be one of 0,1,2");
            exit (1);
         }
	 break;
      case 'q':
         param_quality = atoi (optarg);
         if ((param_quality<24)||(param_quality>100)) {
            mjpeg_error( "quality parameter (%d) out of range [24..100]!", param_quality);
            exit (1);
         }
         break;
      case 'b':
         param_bufsize = atoi (optarg) * 1024;
         /* constraints? */
         break;
      case 'm':
         param_maxfilesize = atoi(optarg);
         break;
      case 'o':
         param_output = optarg;
         break;
      case 'w':
         param_inputwav = optarg;
         break;
      default:
         usage();
         exit(1);
      }
   }

   if (!param_output) {
      mjpeg_error( "yuv2lav needs an output filename");
      usage ();
      exit (1);
   }
   if (param_interlace == 2 && param_format == 'q') {
      mjpeg_error("cannot use -I 2 with -f %c", param_format);
      usage ();
      exit (1);
   }

/*
 * If NO limit was explicitly specified (to limit files to 2GB or less) then
 * allow unlimited (well, ok - 4TB ;)) size.  ODML extensions will handle the
 * AVI files and Quicktime has had 64bit filesizes for a long time
*/
   if (param_maxfilesize <= 0)
      param_maxfilesize = MAX_MBYTES_PER_FILE_64;

   (void)mjpeg_default_handler_verbosity(verbose);   
   fd_in = 0;                   /* stdin */

   y4m_accept_extensions(1);

   y4m_init_stream_info(&streaminfo);
   y4m_init_frame_info(&frameinfo);
   if (y4m_read_stream_header(fd_in, &streaminfo) != Y4M_OK) {
      mjpeg_error( "Couldn't read YUV4MPEG header!");
      exit (1);
   }

   switch (y4m_si_get_interlace(&streaminfo)) {
   case Y4M_ILACE_NONE:
   case Y4M_ILACE_TOP_FIRST:
   case Y4M_ILACE_BOTTOM_FIRST:
     break;
   default:
     mjpeg_error_exit1("unsupported input interlace");
   }

   switch (y4m_si_get_chroma(&streaminfo)) {
   case Y4M_CHROMA_420JPEG:
   case Y4M_CHROMA_422:
     break;
   case Y4M_CHROMA_420MPEG2:
   case Y4M_CHROMA_420PALDV:
     mjpeg_warn("unsupported input chroma, assume '420jpeg'");
     y4m_si_set_chroma(&streaminfo, Y4M_CHROMA_420JPEG);
     break;
   default:
     mjpeg_error_exit1("unsupported input chroma");
   }

   if (param_interlace < 0) {
	   param_interlace = y4m_si_get_interlace(&streaminfo);
   }

   /* If no desired format option was given, we try to detect the format with */
   /* the last 4 char of the filename */
   dotptr = strrchr(param_output, '.');
   if (dotptr && (param_format == 'x'))
   {
#ifdef HAVE_LIBQUICKTIME 
	   if (!strcasecmp(dotptr+1, "mov"))
		   param_format = 'q';
#endif
	   if (!strcasecmp(dotptr+1, "avi"))
		   param_format = 'a';
   }

   /* Telling the people which format we really use */
   if (param_format == 'a')
	   mjpeg_info("creating AVI output format");
   else if (param_format == 'q')
	   mjpeg_info("creating Quicktime output format");
   else 
	   mjpeg_error_exit1("No format specified add the -f option");

   if (param_interlace == Y4M_ILACE_TOP_FIRST  && param_format == 'A')
	   param_format = 'a';
   else if (param_interlace == Y4M_ILACE_BOTTOM_FIRST && param_format == 'a')
	   param_format = 'A';
   fps = Y4M_RATIO_DBL(y4m_si_get_framerate(&streaminfo));

   /* Open WAV file */

   if (param_inputwav != NULL)
   {
	   wav_fd = open(param_inputwav,O_RDONLY);
	   if(wav_fd<0) { mjpeg_error_exit1("Open WAV file: %s", strerror(errno));}

	   n = read(wav_fd,(char*)data,20);
	   if(n!=20) { mjpeg_error_exit1("Read WAV file: %s", strerror(errno)); }

	   if(data[0] != FOURCC_RIFF || data[2] != FOURCC_WAVE ||
			   data[3] != FOURCC_FMT  || data[4] > sizeof(data) )
	   {
		   mjpeg_error_exit1("Error in WAV header");
	   }

	   fmtlen = data[4];

	   n = read(wav_fd,(char*)data,fmtlen);
	   if(n!=fmtlen) { perror("read WAV header"); exit(1); }
   
      if( (data[0]&0xffff) != 1)
      {
         mjpeg_error_exit1("WAV file is not in PCM format");
      }
   
      audio_chans = (data[0]>>16) & 0xffff;
      audio_rate  = data[1];
      audio_bits  = (data[3]>>16) & 0xffff;
      audio_bps   = (audio_chans*audio_bits+7)/8;
   
      if(audio_bps==0) audio_bps = 1; /* safety first */
   
      n = read(wav_fd,(char*)data,8);
      if(n!=8) { mjpeg_error_exit1("Read WAV header: %s", strerror(errno)); }
   
      if(data[0] != FOURCC_DATA)
      {
         mjpeg_error_exit1("Error in WAV header");
      }
      audio_samps = data[1]/audio_bps;
      absize = audio_rate/fps;
      abuff = (uint8_t*) malloc(((int)ceil(absize)+10)*audio_bps);
      if(abuff==0)
      {
         mjpeg_error_exit1("Out of Memory - malloc failed");
      }
      na_out = 0;

      /* Debug Output */
   
      mjpeg_debug("File: %s",param_inputwav);
      mjpeg_debug("   audio samps: %8ld",audio_samps);
      mjpeg_debug("   audio chans: %8d",audio_chans);
      mjpeg_debug("   audio bits:  %8d",audio_bits);
      mjpeg_debug("   audio rate:  %8ld",audio_rate);
      mjpeg_debug(" ");
      mjpeg_debug("Length of audio:  %15.3f sec",(double)audio_samps/(double)audio_rate);
      mjpeg_debug(" ");
   }

   if (strstr(param_output,"%"))
   {
      char buff[256];
      sprintf(buff, param_output, filenum++);
      output = lav_open_output_file (buff, param_format,
                                  y4m_si_get_width(&streaminfo),
				  y4m_si_get_height(&streaminfo),
				  param_interlace,
                                  fps,
                                  (param_inputwav == NULL)
				     ? 0 : audio_bits,
                                  (param_inputwav == NULL)
				     ? 0 : audio_chans,
                                  (param_inputwav == NULL)
				     ? 0 : audio_rate);
      if (!output) {
         mjpeg_error( "Error opening output file %s: %s", buff, lav_strerror ());
         exit(1);
      }
   }
   else
   {
      output = lav_open_output_file (param_output, param_format,
                                  y4m_si_get_width(&streaminfo),
				  y4m_si_get_height(&streaminfo),
				  param_interlace,
                                  fps,
                                  (param_inputwav == NULL)
				     ? 0 : audio_bits,
                                  (param_inputwav == NULL)
				     ? 0 : audio_chans,
                                  (param_inputwav == NULL)
				     ? 0 : audio_rate);
      if (!output) {
         mjpeg_error( "Error opening output file %s: %s", param_output, lav_strerror ());
         exit(1);
      }
   }

   yuv[0] = malloc(y4m_si_get_width(&streaminfo) *
		   y4m_si_get_height(&streaminfo) *
		   sizeof(unsigned char));
   mjpeg_info(" Plane 0 X: %i Y: %i, Chroma %i", y4m_si_get_width(&streaminfo), y4m_si_get_height(&streaminfo), y4m_si_get_chroma(&streaminfo)); 
   yuv[1] = malloc(y4m_si_get_width(&streaminfo) * 
		   y4m_si_get_height(&streaminfo) *
		   sizeof(unsigned char) /
		   ((y4m_si_get_chroma(&streaminfo) == Y4M_CHROMA_422)? 2: 4));
   yuv[2] = malloc(y4m_si_get_width(&streaminfo) * 
		   y4m_si_get_height(&streaminfo) *
		   sizeof(unsigned char) /
		   ((y4m_si_get_chroma(&streaminfo) == Y4M_CHROMA_422)? 2: 4));
   jpeg = (uint8_t*)malloc(param_bufsize);
   
   signal (SIGINT, sigint_handler);

   frame = 0;
   while (y4m_read_frame(fd_in, &streaminfo, &frameinfo, yuv)==Y4M_OK && (!got_sigint)) {

      fprintf (stdout, "frame %d\r", frame);
      fflush (stdout);
      jpegsize = encode_jpeg_raw (jpeg, param_bufsize, param_quality,
                                  param_interlace,
				  y4m_si_get_chroma(&streaminfo),
                                  y4m_si_get_width(&streaminfo),
				  y4m_si_get_height(&streaminfo),
                                  yuv[0], yuv[1], yuv[2]);
      if (jpegsize==-1) {
         mjpeg_error( "Couldn't compress YUV to JPEG");
         exit(1);
      }

      if ((filesize_cur + jpegsize + (int)absize*audio_bps)>>20 > param_maxfilesize)
      {
         /* max file size reached, open a new one if possible */
         if (strstr(param_output,"%"))
         {
            char buff[256];
            if (lav_close (output)) {
               mjpeg_error("Closing output file: %s",lav_strerror());
               exit(1);
            }
            sprintf(buff, param_output, filenum++);
            output = lav_open_output_file (buff, param_format,
                                        y4m_si_get_width(&streaminfo),
                                        y4m_si_get_height(&streaminfo),
                                        param_interlace,
                                        fps,
                                        (param_inputwav == NULL)
				           ? 0 : audio_bits,
                                        (param_inputwav == NULL)
				           ? 0 : audio_chans,
                                        (param_inputwav == NULL)
				           ? 0 : audio_rate);
            if (!output) {
               mjpeg_error( "Error opening output file %s: %s", buff, lav_strerror ());
               exit(1);
            }
            mjpeg_info("Maximum file size reached (%d MB), opened new file: %s",
               param_maxfilesize, buff);
            filesize_cur = 0;
         }
         else
         {
            mjpeg_warn("Maximum file size reached");
            break;
         }
      }

      if (lav_write_frame (output, jpeg, jpegsize, 1)) {
         mjpeg_error( "Writing output: %s", lav_strerror());
         exit(1);
      }
      filesize_cur += jpegsize;
      frame++;

      if (param_inputwav != NULL)
      {
         asize = ((int)ceil ((double)frame * absize)
            - (int)ceil ((double)(frame - 1) * absize)) * audio_bps;
         fflush (stdout);
         n = read(wav_fd,abuff,asize);
         if(n>0)
         {
            na_out += n/audio_bps;
            n = lav_write_audio(output,abuff,n/audio_bps);
            if(n<0)
            {
               mjpeg_error("Error writing audio: %s",lav_strerror());
               lav_close(output);
               exit(1);
            }
         }
      }
   }

   /* copy remaining audio */
   if (param_inputwav != NULL)
   {
      do
      {
         asize = (int)absize * audio_bps;
         n = read(wav_fd,abuff,asize);
         if(n>0)
         {
            na_out += n/audio_bps;
            n = lav_write_audio(output,abuff,n/audio_bps);
            if(n<0)
            {
               mjpeg_error("Error writing audio: %s",lav_strerror());
               lav_close(output);
               exit(1);
            }
         }
      }
      while(n>0);
   }

   if(na_out != audio_samps)
      mjpeg_warn("audio samples expected: %ld, written: %ld",
              audio_samps, na_out);
   if (lav_close (output)) {
      mjpeg_error("Closing output file: %s",lav_strerror());
      exit(1);
   }
      
   for (n=0; n<3; n++) {
      free(yuv[n]);
   }

   y4m_fini_frame_info(&frameinfo);
   y4m_fini_stream_info(&streaminfo);

   return 0;
}
