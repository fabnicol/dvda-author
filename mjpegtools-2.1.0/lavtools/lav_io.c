/*
 *  Some routines for handling I/O from/to different video
 *  file formats (currently AVI, Quicktime)
 *
 *  These routines are isolated here in an extra file
 *  in order to be able to handle more formats in the future.
 *
 *  Copyright (C) 2000 Rainer Johanni <Rainer@Johanni.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "lav_io.h"

#ifdef HAVE_LIBDV
#include <libdv/dv.h>
#endif

extern int AVI_errno;

static char video_format=' ';
static int  internal_error=0;
int libdv_pal_yv12 = -1;
uint16_t reorder_16(uint16_t todo, int big_endian);

#define ERROR_JPEG      1
#define ERROR_MALLOC    2
#define ERROR_FORMAT    3
#define ERROR_NOAUDIO   4

static unsigned long jpeg_field_size     = 0;
static unsigned long jpeg_quant_offset   = 0;
static unsigned long jpeg_huffman_offset = 0;
static unsigned long jpeg_image_offset   = 0;
static unsigned long jpeg_scan_offset    = 0;
static unsigned long jpeg_data_offset    = 0;
static unsigned long jpeg_padded_len     = 0;
static unsigned long jpeg_app0_offset    = 0;
static unsigned long jpeg_app1_offset    = 0;

#define M_SOF0  0xC0
#define M_SOF1  0xC1
#define M_DHT   0xC4
#define M_SOI   0xD8		/* Start Of Image (beginning of datastream) */
#define M_EOI   0xD9		/* End Of Image (end of datastream) */
#define M_SOS   0xDA		/* Start Of Scan (begins compressed data) */
#define M_DQT   0xDB
#define M_APP0  0xE0
#define M_APP1  0xE1

#define QUICKTIME_MJPG_TAG 0x6d6a7067  /* 'mjpg' */

#ifdef HAVE_LIBDV
static int check_DV2_input(lav_file_t *lav_fd);
#endif

#define TMP_EXTENSION ".tmp"

#ifdef HAVE_LIBQUICKTIME
/*
   put_int4:
   Put a 4 byte integer value into a character array as big endian number
*/

static void put_int4(unsigned char *buf, int val)
{
	buf[0] = (val >> 24);
	buf[1] = (val >> 16);
	buf[2] = (val >> 8 );
	buf[3] = (val      );
}
#endif

/*
   get_int2:
   get a 2 byte integer value from a character array as big endian number
 */

static int get_int2(unsigned char *buff)
{
   return (buff[0]*256 + buff[1]);
}

/*
   scan_jpeg:
   Scan jpeg data for markers, needed for Quicktime MJPA format
   and partly for AVI files.
   Taken mostly from Adam Williams' quicktime library
 */

static int scan_jpeg(unsigned char * jpegdata, long jpeglen, int header_only)
{
   int  marker, length;
   long p;

   jpeg_field_size     = 0;
   jpeg_quant_offset   = 0;
   jpeg_huffman_offset = 0;
   jpeg_image_offset   = 0;
   jpeg_scan_offset    = 0;
   jpeg_data_offset    = 0;
   jpeg_padded_len     = 0;
   jpeg_app0_offset    = 0;
   jpeg_app1_offset    = 0;

   /* The initial marker must be SOI */

   if (jpegdata[0] != 0xFF || jpegdata[1] != M_SOI) return -1;

   /* p is the pointer within the jpeg data */

   p = 2;

   /* scan through the jpeg data */

   while(p<jpeglen)
   {
      /* get next marker */

      /* Find 0xFF byte; skip any non-FFs */
      while(jpegdata[p] != 0xFF)
      {
         p++;
         if(p>=jpeglen) return -1;
      }

      /* Get marker code byte, swallowing any duplicate FF bytes */
      while(jpegdata[p] == 0xFF)
      {
         p++;
         if(p>=jpeglen) return -1;
      }

      marker = jpegdata[p++];

      if(p<=jpeglen-2)
         length = get_int2(jpegdata+p);
      else
         length = 0;

      /* We found a marker - check it */

      if(marker == M_EOI) { jpeg_field_size = p; break; }

      switch(marker)
      {
         case M_SOF0:
         case M_SOF1:
            jpeg_image_offset = p-2;
            break;
         case M_DQT:
            if(jpeg_quant_offset==0) jpeg_quant_offset = p-2;
            break;
         case M_DHT:
            if(jpeg_huffman_offset==0) jpeg_huffman_offset = p-2;
            break;
         case M_SOS:
            jpeg_scan_offset = p-2;
            jpeg_data_offset = p+length;
            if(header_only) return 0; /* we are done with the headers */
            break;
         case M_APP0:
            if(jpeg_app0_offset==0) jpeg_app0_offset = p-2;
            break;
         case M_APP1:
            if(jpeg_app1_offset==0) jpeg_app1_offset = p-2;
            break;
      }

      /* The pseudo marker as well as the markers M_TEM (0x01)
         and M_RST0 ... M_RST7 (0xd0 ... 0xd7) have no paramters.
         M_SOI and M_EOI also have no parameters, but we should
         never come here in that case */

      if(marker == 0 || marker == 1 || (marker >= 0xd0 && marker <= 0xd7))
         continue;

      /* skip length bytes */

      if(p+length<=jpeglen)
         p += length;
      else
         return -1;
   }

   /* We are through parsing the jpeg data, we should have seen M_EOI */

   if(!jpeg_field_size) return -1;

   /* Check for trailing garbage until jpeglen is reached or a new
      M_SOI is seen */

   while(p<jpeglen)
   {
      if(p<jpeglen-1 && jpegdata[p]==0xFF && jpegdata[p+1]==M_SOI) break;
      p++;
   }

   jpeg_padded_len = p;
   return 0;
}

/* The query routines about the format */

int lav_query_APP_marker(char format)
{
   /* AVI needs the APP0 marker, Quicktime APP1 */

   switch(format)
   {
      case 'a': return 0;
      case 'A': return 0;
      case 'j': return 0;
      case 'q': return 1;
      default:  return 0;
   }
}

int lav_query_APP_length(char format)
{
   /* AVI: APP0 14 bytes, Quicktime APP1: 40 */

   switch(format)
   {
      case 'a': return 14;
      case 'A': return 14;
      case 'j': return 14;
      case 'q': return 40;
      default:  return 0;
   }
}

int lav_query_polarity(char format)
{
   /* Quicktime needs TOP_FIRST, for AVI we have the choice */

   switch(format)
   {
      case 'a': return Y4M_ILACE_TOP_FIRST;
      case 'A': return Y4M_ILACE_BOTTOM_FIRST;
      case 'j': return Y4M_ILACE_TOP_FIRST;
      case 'q': return Y4M_ILACE_TOP_FIRST;
      default:  return Y4M_ILACE_TOP_FIRST;
   }
}

lav_file_t *lav_open_output_file(char *filename, char format,
                    int width, int height, int interlaced, double fps,
                    int asize, int achans, long arate)
{
   lav_file_t *lav_fd = (lav_file_t*) malloc(sizeof(lav_file_t));
   char *tempfile;

   if (lav_fd == 0) { internal_error=ERROR_MALLOC; return 0; }

   lav_fd->avi_fd      = 0;
   lav_fd->qt_fd       = 0;
   lav_fd->format      = format;
   lav_fd->interlacing = interlaced ? lav_query_polarity(format) :
                                      Y4M_ILACE_NONE;
   lav_fd->has_audio   = (asize>0 && achans>0);
   lav_fd->bps         = (asize*achans+7)/8;
   lav_fd->chroma = Y4M_UNKNOWN;

   switch(format)
   {
      case 'a':
      case 'A':
         /* Open AVI output file */

         lav_fd->avi_fd = AVI_open_output_file(filename);
         if(!lav_fd->avi_fd) { free(lav_fd); return 0; }
         AVI_set_video(lav_fd->avi_fd, width, height, fps, "MJPG");
         if (asize) AVI_set_audio(lav_fd->avi_fd, achans, arate, asize, WAVE_FORMAT_PCM, 0);
         return lav_fd;

      case 'j':
        /* Open JPEG output file */
	tempfile = (char *)malloc(strlen(filename) + strlen(TMP_EXTENSION) + 1);
	if (tempfile == NULL)
	   {
	   internal_error=ERROR_MALLOC;
	   return(0);
	   }
        strcpy(tempfile, filename);
        strcat(tempfile, TMP_EXTENSION);
        lav_fd->jpeg_filename = strdup(filename);
        lav_fd->jpeg_fd = open(tempfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	free(tempfile);
        return lav_fd;

      case 'q':
#ifdef HAVE_LIBQUICKTIME
         /* open quicktime output file */

         /* since the documentation says that the file should be empty,
            we try to remove it first */

         remove(filename);

         lav_fd->qt_fd = quicktime_open(filename, 0, 1);
         if(!lav_fd->qt_fd) { free(lav_fd); return 0; }
         quicktime_set_video(lav_fd->qt_fd, 1, width, height, fps,
                             (interlaced ? QUICKTIME_MJPA : QUICKTIME_JPEG));
	 if (interlaced)
	    {
	    if (lav_fd->interlacing == Y4M_ILACE_TOP_FIRST)
               lqt_set_fiel(lav_fd->qt_fd, 0, 2, 9);
	    else if (lav_fd->interlacing == Y4M_ILACE_BOTTOM_FIRST)
               lqt_set_fiel(lav_fd->qt_fd, 0, 2, 14);
	    }
         if (asize)
	    quicktime_set_audio(lav_fd->qt_fd, achans, arate, asize, QUICKTIME_TWOS);
         return lav_fd;
#else
	 internal_error = ERROR_FORMAT;
	 return 0;
#endif
      default:
         return 0;
   }
}

int lav_close(lav_file_t *lav_file)
   {
   int res;
   char *tempfile;

   video_format = lav_file->format; internal_error = 0; /* for error messages */

   switch (lav_file->format)
      {
      case 'a':
      case 'A':
         res = AVI_close( lav_file->avi_fd );
         break;
      case 'j':
	 tempfile = (char *)malloc(strlen(lav_file->jpeg_filename) + 
				   strlen(TMP_EXTENSION) + 1);
	 if (tempfile == NULL)
	    {
	    res = -1;
	    break;
	    }
         strcpy(tempfile, lav_file->jpeg_filename);
         strcat(tempfile, TMP_EXTENSION);
         res = close(lav_file->jpeg_fd);
         rename(tempfile, lav_file->jpeg_filename);
	 free(tempfile);
         free(lav_file->jpeg_filename);
         break;
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         res = quicktime_close( lav_file->qt_fd );
         break;
#endif
      default:
         res = -1;
      }
   free(lav_file);
   return res;
   }

int lav_write_frame(lav_file_t *lav_file, uint8_t *buff, long size, long count)
{
   int res, n;
   uint8_t *jpgdata = NULL;
   long jpglen = 0;

   video_format = lav_file->format; internal_error = 0; /* for error messages */

   /* For interlaced video insert the apropriate APPn markers */

   if(lav_file->interlacing != Y4M_ILACE_NONE)
   {
      switch(lav_file->format)
      {
         case 'a':
         case 'A':

            jpgdata = buff;
            jpglen  = size;

            /* Loop over both fields */

            for(n=0;n<2;n++)
            {
               /* For first field scan entire field, for second field
                  scan the JPEG header, put in AVI1 + polarity.
                  Be generous on errors */

               res = scan_jpeg(jpgdata, size, n);
               if (res) { internal_error=ERROR_JPEG; return -1; }

               if(!jpeg_app0_offset) continue;

               /* APP0 marker should be at least 14+2 bytes */
               if(get_int2(jpgdata+jpeg_app0_offset+2) < 16 ) continue;

               jpgdata[jpeg_app0_offset+4] = 'A';
               jpgdata[jpeg_app0_offset+5] = 'V';
               jpgdata[jpeg_app0_offset+6] = 'I';
               jpgdata[jpeg_app0_offset+7] = '1';
               jpgdata[jpeg_app0_offset+8] = lav_file->format=='a' ? n+1 : 2-n;

               /* Update pointer and len for second field */
               jpgdata += jpeg_padded_len;
               jpglen  -= jpeg_padded_len;
            }
            break;

         case 'j':

            jpgdata = buff;
            jpglen = size;
            break;

#ifdef HAVE_LIBQUICKTIME
         case 'q':

            jpgdata = buff;
            jpglen  = size;

            /* Loop over both fields */

            for(n=0;n<2;n++)
            {
               /* Scan the entire JPEG field data - APP1 marker MUST be present */
               res = scan_jpeg(jpgdata,jpglen,0);
               if(res || !jpeg_app1_offset) { internal_error=ERROR_JPEG; return -1; }

               /* Length of APP1 marker must be at least 40 + 2 bytes */
               if ( get_int2(jpgdata+jpeg_app1_offset+2) < 42)
               { internal_error=ERROR_JPEG; return -1; }

               /* Fill in data */
               put_int4(jpgdata+jpeg_app1_offset+ 4,0);
               put_int4(jpgdata+jpeg_app1_offset+ 8,QUICKTIME_MJPG_TAG);
               put_int4(jpgdata+jpeg_app1_offset+12,jpeg_field_size);
               put_int4(jpgdata+jpeg_app1_offset+16,jpeg_padded_len);
               put_int4(jpgdata+jpeg_app1_offset+20,n==0?jpeg_padded_len:0);
               put_int4(jpgdata+jpeg_app1_offset+24,jpeg_quant_offset);
               put_int4(jpgdata+jpeg_app1_offset+28,jpeg_huffman_offset);
               put_int4(jpgdata+jpeg_app1_offset+32,jpeg_image_offset);
               put_int4(jpgdata+jpeg_app1_offset+36,jpeg_scan_offset);
               put_int4(jpgdata+jpeg_app1_offset+40,jpeg_data_offset);

               /* Update pointer and len for second field */
               jpgdata += jpeg_padded_len;
               jpglen  -= jpeg_padded_len;
            }
            break;
#endif
      }
   }
   
   res = 0; /* Silence gcc */
   for(n=0;n<count;n++)
   {
      switch(lav_file->format)
      {
         case 'a':
         case 'A':
            if(n==0)
               res = AVI_write_frame( lav_file->avi_fd, buff, size, 0);
            else
               res = AVI_dup_frame( lav_file->avi_fd );
            break;
         case 'j':
            if (n==0)
               write(lav_file->jpeg_fd, buff, size);
            break;
#ifdef HAVE_LIBQUICKTIME
         case 'q':
            res = quicktime_write_frame( lav_file->qt_fd, buff, size, 0 );
            break;
#endif
         default:
            res = -1;
      }
      if (res) break;
   }
   return res;
}

int lav_write_audio(lav_file_t *lav_file, uint8_t *buff, long samps)
{
   int res = -1;
#ifdef HAVE_LIBQUICKTIME
   int i, j;
   int16_t *buff16 = (int16_t *)buff, **qt_audion;
   int channels = lav_audio_channels(lav_file);
   int bits = lav_audio_bits(lav_file);
#endif

   video_format = lav_file->format; internal_error = 0; /* for error messages */

   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         res = AVI_write_audio( lav_file->avi_fd, buff, samps*lav_file->bps);
         break;
#ifdef HAVE_LIBQUICKTIME
      case 'q':
	if (bits != 16 || channels > 1)
	 {
    /* Deinterleave the audio into the two channels and/or convert
     * bits per sample to the required format.
     */
    qt_audion = malloc(channels * sizeof(*qt_audion));
    for (i = 0; i < channels; i++)
      qt_audion[i] = malloc(samps * sizeof(**qt_audion));

    if (bits == 16)
      for (i = 0; i < samps; i++)
			for (j = 0; j < channels; j++)
			  qt_audion[j][i] = buff16[channels * i + j];
   else if (bits == 8)
		for (i = 0; i < samps; i++)
		  for (j = 0; j < channels; j++)
		    qt_audion[j][i] = ((int16_t)(buff[channels * i + j]) << 8) ^ 0x8000;

   if (bits == 8 || bits == 16)
      res = lqt_encode_audio_track(lav_file->qt_fd, qt_audion, NULL, samps, 0);

	for (i = 0; i < channels; i++)
		free(qt_audion[i]);
		free(qt_audion);
  	} 
	else 
	{
		qt_audion = &buff16;
		res = lqt_encode_audio_track(lav_file->qt_fd, qt_audion, NULL, samps, 0);
  	}
  break;
#endif
      default:
	break;
   }

   return res;
}

long lav_video_frames(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_video_frames(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_video_length(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

int lav_video_width(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_video_width(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_video_width(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

int lav_video_height(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_video_height(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_video_height(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

double lav_frame_rate(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_frame_rate(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_frame_rate(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

int lav_video_interlacing(lav_file_t *lav_file)
{
   return lav_file->interlacing;
}

void lav_video_sampleaspect(lav_file_t *lav_file, int *sar_w, int *sar_h)
{
  *sar_w = lav_file->sar_w;
  *sar_h = lav_file->sar_h;
  return;
}

int lav_video_chroma(lav_file_t *lav_file)
{
	return lav_file->chroma;
}

const char *lav_video_compressor(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_video_compressor(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_video_compressor(lav_file->qt_fd,0);
#endif
   }
   return "N/A";
}

int lav_audio_channels(lav_file_t *lav_file)
{
   if(!lav_file->has_audio) return 0;
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
      {
      case 'a':
      case 'A':
         return AVI_audio_channels(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_track_channels(lav_file->qt_fd,0);
#endif
      }
   return -1;
}

int lav_audio_bits(lav_file_t *lav_file)
{
   if(!lav_file->has_audio) return 0;
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
      {
      case 'a':
      case 'A':
         return AVI_audio_bits(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_audio_bits(lav_file->qt_fd,0);
#endif
      }
   return -1;
}

long lav_audio_rate(lav_file_t *lav_file)
{
   if(!lav_file->has_audio) return 0;
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_audio_rate(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_sample_rate(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

long lav_audio_samples(lav_file_t *lav_file)
{
   if(!lav_file->has_audio) return 0;
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_audio_bytes(lav_file->avi_fd)/lav_file->bps;
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_audio_length(lav_file->qt_fd,0);
#endif
   }
   return -1;
}

long lav_frame_size(lav_file_t *lav_file, long frame)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_frame_size(lav_file->avi_fd,frame);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_frame_size(lav_file->qt_fd,frame,0);
#endif
   }
   return -1;
}

int lav_seek_start(lav_file_t *lav_file)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_seek_start(lav_file->avi_fd);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_seek_start(lav_file->qt_fd);
#endif
   }
   return -1;
}

int lav_set_video_position(lav_file_t *lav_file, long frame)
{
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_set_video_position(lav_file->avi_fd,frame);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_set_video_position(lav_file->qt_fd,frame,0);
#endif
   }
   return -1;
}

int lav_read_frame(lav_file_t *lav_file, uint8_t *vidbuf)
{
int keyframe;

   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_read_frame(lav_file->avi_fd,vidbuf, &keyframe);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         return quicktime_read_frame(lav_file->qt_fd,vidbuf,0);
#endif
   }
   return -1;
}

int lav_set_audio_position(lav_file_t *lav_file, long sample)
{
   if(!lav_file->has_audio) return 0;
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_set_audio_position(lav_file->avi_fd,sample*lav_file->bps);
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         quicktime_set_audio_position(lav_file->qt_fd,sample,0);
#endif
   }
   return -1;
}

long lav_read_audio(lav_file_t *lav_file, uint8_t *audbuf, long samps)
{
#ifdef	HAVE_LIBQUICKTIME
   int64_t last_pos, start_pos;
   int res, i, j;
   int16_t *qt_audio = (int16_t *)audbuf, **qt_audion;
   int channels = lav_audio_channels(lav_file);
   uint8_t b0, b1;
#endif

   if(!lav_file->has_audio)
   {
      internal_error = ERROR_NOAUDIO;
      return(-1);
   }
   video_format = lav_file->format; internal_error = 0; /* for error messages */
   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         return AVI_read_audio(lav_file->avi_fd,audbuf,samps*lav_file->bps)/lav_file->bps;
#ifdef HAVE_LIBQUICKTIME
      case 'q':
	qt_audion = malloc(channels * sizeof (int16_t **));
	for (i = 0; i < channels; i++)
	    qt_audion[i] = (int16_t *)malloc(samps * lav_file->bps);

	start_pos = quicktime_audio_position(lav_file->qt_fd, 0);
	lqt_decode_audio_track(lav_file->qt_fd, qt_audion, NULL, samps, 0);
	last_pos = lqt_last_audio_position(lav_file->qt_fd, 0);
	res = last_pos - start_pos;
	if (res <= 0)
	   goto out;
	/* Interleave the channels of audio into the one buffer provided */
	for (i =0; i < res; i++)
	    {
	    for (j = 0; j < channels; j++)
		qt_audio[(channels*i) + j] = qt_audion[j][i];
	    }

        if (lav_detect_endian())
           {
           i= 0;
           while (i < (2*res) )
                 {
                 b0 = 0;
                 b1 = 0; 
                 b0 = (qt_audio[i] & 0x00FF);
                 b1 =  (qt_audio[i] & 0xFF00) >> 8;
                 qt_audio[i] = (b0 <<8) + b1;
                 i = i +1;
                 } 
            }
out:
	for (j = 0; j < channels; j++)
            free(qt_audion[j]);
	free(qt_audion);
        return(res);
#endif
   }
   return -1;
}

lav_file_t *lav_open_input_file(char *filename)
{
   int n;
   const char *video_comp = NULL;
#ifdef	HAVE_LIBQUICKTIME
   char *audio_comp;
#endif
   unsigned char *frame = NULL; /* Make sure un-init segfaults! */
   long len;
   int jpg_height, jpg_width, ncomps, hf[3], vf[3];
   int ierr;

   lav_file_t *lav_fd = (lav_file_t*) malloc(sizeof(lav_file_t));

   if(lav_fd==0) { internal_error=ERROR_MALLOC; return 0; }

   /* Set lav_fd */

   lav_fd->avi_fd      = 0;
   lav_fd->qt_fd       = 0;
   lav_fd->format      = 0;
   lav_fd->interlacing = Y4M_UNKNOWN;
   lav_fd->sar_w       = 1; /* unknown - assume square pixels */
   lav_fd->sar_h       = 1; 
   lav_fd->has_audio   = 0;
   lav_fd->bps         = 0;
   lav_fd->chroma = Y4M_UNKNOWN;

   /* open video file, try AVI first */

   lav_fd->avi_fd = AVI_open_input_file(filename,1);
   video_format = 'a'; /* for error messages */

   if(lav_fd->avi_fd)
   {
      /* It is an AVI file */
      lav_fd->qt_fd  = 0;
      lav_fd->format = 'a';
      lav_fd->has_audio = (AVI_audio_bits(lav_fd->avi_fd)>0 &&
                           AVI_audio_format(lav_fd->avi_fd)==WAVE_FORMAT_PCM);
      video_comp = AVI_video_compressor(lav_fd->avi_fd);
   }
   else if( AVI_errno==AVI_ERR_NO_AVI )
   {
#ifdef HAVE_LIBQUICKTIME
      if(!quicktime_check_sig(filename))
#endif
	  {
	    /* None of the known formats */
            char errmsg[1024];
	    sprintf(errmsg, "Unable to identify file (not a supported format - avi");
#ifdef HAVE_LIBQUICKTIME
            strcat(errmsg, ", quicktime");
#endif
	    strcat(errmsg, ").\n");
            fprintf(stderr, errmsg);
	    free(lav_fd);
	    internal_error = ERROR_FORMAT; /* Format not recognized */
	    return 0;
	  }
#ifdef HAVE_LIBQUICKTIME
      else
	{
	quicktime_pasp_t pasp;
	int nfields, detail;

	  /* It is a quicktime file */
	 lav_fd->qt_fd = quicktime_open(filename,1,0);
	 video_format = 'q'; /* for error messages */
	 if (!lav_fd->qt_fd)
	    {
	    free(lav_fd);
	    return 0;
	    }
	  lav_fd->avi_fd = NULL;
	  lav_fd->format = 'q';
	  video_comp = quicktime_video_compressor(lav_fd->qt_fd,0);
	  /* We want at least one video track */
	  if (quicktime_video_tracks(lav_fd->qt_fd) < 1)
	     {
	     lav_close(lav_fd);
	     internal_error = ERROR_FORMAT;
	     return 0;
	     }
/*
 * If the quicktime file has the sample aspect atom then use it to set
 * the sar values in the lav_fd structure.  Hardwired (like everywhere else)
 * to only look at track 0.
*/
	  if (lqt_get_pasp(lav_fd->qt_fd, 0, &pasp) != 0)
	     {
	     lav_fd->sar_w = pasp.hSpacing;
	     lav_fd->sar_h = pasp.vSpacing;
	     }
/*
 * If a 'fiel' atom is present (not guaranteed) then use it to set the
 * interlacing type.
*/
	  if (lqt_get_fiel(lav_fd->qt_fd, 0, &nfields, &detail) != 0)
	     {
	     if (nfields == 2)
	        {
		if (detail == 14 || detail == 6)
		   lav_fd->interlacing = Y4M_ILACE_BOTTOM_FIRST;
		else if (detail == 9 || detail == 1)
		   lav_fd->interlacing = Y4M_ILACE_TOP_FIRST;
		else
		   mjpeg_warn("unknown 'detail' in 'fiel' atom: %d", detail);
	        }
	     else
	        lav_fd->interlacing = Y4M_ILACE_NONE;
	     }
	  /* Check for audio tracks */
	  lav_fd->has_audio = 0;
	  if (quicktime_audio_tracks(lav_fd->qt_fd))
	     {
	     audio_comp = quicktime_audio_compressor(lav_fd->qt_fd,0);
	     if (strncasecmp(audio_comp, QUICKTIME_TWOS,4)==0)
		lav_fd->has_audio = 1;
	     }
	 }
#endif
   }
   else
   {
      /* There should be an error from avilib, just return */
      free(lav_fd);
      return 0;
   }

   /* set audio bytes per sample */

   lav_fd->bps = (lav_audio_channels(lav_fd)*lav_audio_bits(lav_fd)+7)/8;
   if(lav_fd->bps==0) lav_fd->bps=1; /* make it save since we will divide by that value */

/*
 * Check compressor.  The YUV checks are not right (the support code appears 
 * incorrect and/or incomplete).  In particular yuv2 is a packed format not
 * planar and YV12 has the U and V planes reversed from IYUV (which is what
 * the support code appears to think is YUV420).  My hunch is that folks are 
 * only using DV and MJPG so the YUV bugs aren't being triggered. 
 *
 * But at least now the checks are consolidated in 1 place instead of being
 * duplicated in two places (editlist.c and this file).
*/

   ierr  = 0;

   if (strncasecmp(video_comp, "yv12", 3) == 0)
      {
      lav_fd->dataformat = DATAFORMAT_YUV420;
/*
 * This is probably not correct.  But since 'yv12' isn't really supported it
 * doesn't matter ;)
*/
      lav_fd->chroma = Y4M_CHROMA_420JPEG;
      }
   else if (strncasecmp(video_comp, "yuv2", 4) == 0)
      {
      lav_fd->dataformat = DATAFORMAT_YUV422;
      lav_fd->chroma = Y4M_CHROMA_422;
      }
   else if (strncasecmp(video_comp, "dv", 2) == 0)
      {
      lav_fd->dataformat = DATAFORMAT_DV2;
      lav_fd->interlacing = Y4M_ILACE_BOTTOM_FIRST;
      }
   else if (strncasecmp(video_comp, "mjp", 3) == 0 ||
            strncasecmp(video_comp, "jpeg", 4) == 0)
      lav_fd->dataformat = DATAFORMAT_MJPG;
   else
      {
      internal_error = ERROR_FORMAT;
      goto ERREXIT;
      }

#ifdef HAVE_LIBDV
   if (lav_fd->dataformat == DATAFORMAT_DV2)
      {
      ierr = check_DV2_input(lav_fd);
      if (ierr)
	 goto ERREXIT;
      }
#endif /* HAVE_LIBDV */

   if (lav_fd->dataformat != DATAFORMAT_MJPG)
      return lav_fd;

/*
 * From here on down is MJPG only code - the yuv and dv cases have been handled
 * above.  Make some checks on the video source.  Read the first frame for this.
*/

   frame = NULL;
   if (lav_set_video_position(lav_fd,0))
      goto ERREXIT;
   if ((len = lav_frame_size(lav_fd,0)) <=0)
      goto ERREXIT;
   if ((frame = (unsigned char*) malloc(len)) == 0)
      {
      ierr=ERROR_MALLOC;
      goto ERREXIT;
      }
   if (lav_read_frame(lav_fd,frame) <= 0)
      goto ERREXIT;
   /* reset video position to 0 */
   if (lav_set_video_position(lav_fd,0))
      goto ERREXIT;
   if (scan_jpeg(frame, len, 1))
      {
      ierr=ERROR_JPEG;
      goto ERREXIT;
      }

   /* We have to look to the JPEG SOF marker for further information
      The SOF marker has the following format:

      FF
      C0
      len_hi
      len_lo
      data_precision
      height_hi
      height_lo
      width_hi
      width_lo
      num_components

      And then 3 bytes for each component:

      Component id
      H, V sampling factors (as nibbles)
      Quantization table number
    */

   /* Check if the JPEG has the special 4:2:2 format needed for
      some HW JPEG decompressors (the Iomega Buz, for example) */

   ncomps = frame[jpeg_image_offset + 9];
   if(ncomps==3)
   {
      for(n=0;n<3;n++)
      {
         hf[n] = frame[jpeg_image_offset + 10 + 3*n + 1]>>4;
         vf[n] = frame[jpeg_image_offset + 10 + 3*n + 1]&0xf;
      }

	  /* Identify chroma sub-sampling format only 420 and 422 supported
	   at present...*/
	  if( hf[0] == 2*hf[1] && hf[0] == 2*hf[2] )
	  {
		 if( vf[0] == vf[1] && vf[0] == vf[2] )
		 {
			 lav_fd->chroma = Y4M_CHROMA_422;
		 }
		 else if( vf[0] == 2*vf[1] && vf[0] == 2*vf[2] )
			 lav_fd->chroma = Y4M_CHROMA_420JPEG;
		 else		
			 lav_fd->chroma = Y4M_UNKNOWN;
	  }
	  else
		  lav_fd->chroma = Y4M_UNKNOWN;
   }

   /* Check if video is interlaced */
   /* height and width are encoded in the JPEG SOF marker at offsets 5 and 7 */

   jpg_height = get_int2(frame + jpeg_image_offset + 5);
   jpg_width  = get_int2(frame + jpeg_image_offset + 7);

   /* check height */

   if( jpg_height == lav_video_height(lav_fd))
      lav_fd->interlacing = Y4M_ILACE_NONE;
   else if ( jpg_height == lav_video_height(lav_fd)/2 )
   {
      /* Video is interlaced */

      switch(lav_fd->format)
      {
         case 'a':
            /* Check the APP0 Marker, if present */

            if(jpeg_app0_offset && 
               get_int2(frame + jpeg_app0_offset + 2) >= 5 &&
               strncasecmp((char*)(frame + jpeg_app0_offset + 4),"AVI1",4)==0 )
            {
                if (frame[jpeg_app0_offset+8]==1)
                   lav_fd->interlacing = Y4M_ILACE_TOP_FIRST;
                else
                   lav_fd->interlacing = Y4M_ILACE_BOTTOM_FIRST;
            }
            else
            {
               /* There is no default, it really depends on the
                  application which produced the AVI */
               lav_fd->interlacing = Y4M_ILACE_TOP_FIRST;
            }
            lav_fd->format = lav_fd->interlacing == Y4M_ILACE_BOTTOM_FIRST ? 'A' : 'a';
            break;
         case 'q':
            lav_fd->interlacing = Y4M_ILACE_TOP_FIRST;
	    break;
      }
   }
   else
   {
      ierr=ERROR_JPEG;
      goto ERREXIT;
   }

   free(frame);
   return lav_fd;

ERREXIT:
   lav_close(lav_fd);
   if(frame) free(frame);
   internal_error = ierr;
   return 0;
}

/* Get size of first field of for a data array containing
   (possibly) two jpeg fields */

int lav_get_field_size(uint8_t * jpegdata, long jpeglen)
{
   int res;

   res = scan_jpeg(jpegdata,jpeglen,0);
   if(res<0) return jpeglen; /* Better than nothing */

   /* we return jpeg_padded len since this routine is used
      for field exchange where alignment might be important */

   return jpeg_padded_len;
}

static char error_string[4096];

const char *lav_strerror(void)
{

   switch(internal_error)
   {
      case ERROR_JPEG:
         sprintf(error_string,"Internal: broken JPEG format");
         internal_error = 0;
         return error_string;
      case ERROR_MALLOC:
         sprintf(error_string,"Internal: Out of memory");
         internal_error = 0;
         return error_string;
      case ERROR_FORMAT:
         sprintf(error_string,"Input file format not recognized");
         internal_error = 0;
         return error_string;
      case ERROR_NOAUDIO:
         sprintf(error_string,"Trying to read audio from a video only file");
         internal_error = 0;
         return error_string;
   }

   switch(video_format)
   {
      case 'a':
      case 'A':
         return AVI_strerror();
#ifdef HAVE_LIBQUICKTIME
      case 'q':
         /* The quicktime documentation doesn't say much about error codes,
            we hope that strerror may give some info */
         sprintf(error_string,"Quicktime error, possible(!) reason: %s",strerror(errno));
         return error_string;
#endif
      default:
         /* No or unknown video format */
         if(errno) strerror(errno);
         else sprintf(error_string,"No or unknown video format");
         return error_string;
   }
}

#ifdef HAVE_LIBDV
static int check_DV2_input(lav_file_t *lav_fd)
	{
	int ierr = 0;
	double len = 0;
	unsigned char *frame = NULL;
	dv_decoder_t *decoder = NULL;
	uint8_t *output[3] = { NULL, NULL, NULL };
	uint16_t pitches[3];

   /* Make some checks on the video source, we read the first frame for that */

	if	(lav_set_video_position(lav_fd,0))
		goto ERREXIT;
	if	((len = lav_frame_size(lav_fd,0)) <=0)
		goto ERREXIT;
	if	((frame = (unsigned char*) malloc(len)) == 0)
		{
		ierr=ERROR_MALLOC;
		goto ERREXIT;
		}

	if	(lav_read_frame(lav_fd, frame) <= 0)
		goto ERREXIT;

	decoder = dv_decoder_new(0,0,0);
	dv_parse_header(decoder, frame);

	switch	(decoder->system)
		{
		case	e_dv_system_525_60:
			if	(dv_format_wide(decoder))
				{
				lav_fd->sar_w = 40;
				lav_fd->sar_h = 33;
				}
			else	{
	 			lav_fd->sar_w = 10;
	 			lav_fd->sar_h = 11;
       				} 
       			break;
		case	e_dv_system_625_50:
			if	(dv_format_wide(decoder))
				{
				lav_fd->sar_w = 118;
				lav_fd->sar_h = 81;
			} else {
				lav_fd->sar_w = 59;
				lav_fd->sar_h = 54;
				} 
			break;
		default:
			ierr = ERROR_FORMAT; /* Neither 525 or 625 system */
			goto ERREXIT;	     /* declare a format error! */
		}

	switch(decoder->sampling) {
	case e_dv_sample_420:
	  /* libdv decodes PAL DV directly as planar YUV 420
	   * (YV12 or 4CC 0x32315659) if configured with the flag
	   * --with-pal-yuv=YV12 which is not (!) the default
	   */
	  if (libdv_pal_yv12 < 0 ) {
	    /* PAL YV12 not already detected.
	     * Setting decoding options for 4:2:2, because libdv pitches
	     * is *int* for YUY2 and *uint16_t* for YV12 (no comment...)
	     * In YV12 mode, even if pitches is 0, the first line is filled.
	     */

	    pitches[0] = decoder->width * 2;
	    pitches[1] = 0;
	    pitches[2] = 0;

	    /* Hacking libdv: set invalid values for chroma. If libdv
	     * was compiled with PAL YV12, the values are overwritten,
	     * otherwise (4:2:2 output) only dv_frame[0] is filled and
	     * chroma planes remain untouched.
	     */
	    output[0] = (uint8_t *)malloc(decoder->width * decoder->height * 2);
	    output[1] = (uint8_t *)malloc(decoder->width * decoder->height / 2);
	    if (!output[0] || !output[1])
	    {
	      ierr=ERROR_MALLOC;
	      goto ERREXIT;
	    } else {
	      output[2] = output[1] + (decoder->width * decoder->height / 4 );
	      output[1][0] = 0;
	      output[1][1] = 255;
	      output[2][0] = 255;
	      output[2][1] = 0;
	      dv_decode_full_frame(decoder, frame, e_dv_color_yuv,
				   output, (int *)pitches);
	      if (output[1][0]==0   && output[1][1]==255 &&
		  output[2][0]==255 && output[2][1]==0) {
		libdv_pal_yv12 = 0;
		lav_fd->chroma = Y4M_CHROMA_422;
		mjpeg_info("Detected libdv PAL output YUY2 (4:2:2)");
	      } else {
		libdv_pal_yv12 = 1;
		lav_fd->chroma = Y4M_CHROMA_420PALDV;
		mjpeg_info("Detected libdv PAL output YV12 (4:2:0)");
	      }
	      free(output[0]);
	      free(output[1]);
	    }
	  } else {
	    if(libdv_pal_yv12 == 0)
	      lav_fd->chroma = Y4M_CHROMA_422;
	    else
	      lav_fd->chroma = Y4M_CHROMA_420PALDV;
	  }
	  break;
	case e_dv_sample_411:
	   /* libdv decodes NTSC DV (native 411) as packed YUV 422 (YUY2 or 4CC 0x32595559)
	    * where the U and V information is repeated.
	    */
	   lav_fd->chroma = Y4M_CHROMA_411;
	   break;
	case e_dv_sample_422:
	   /* libdv decodes PAL DV (native 420) as packed YUV 422 (YUY2 or 4CC 0x32595559)
	    * where the U and V information is repeated.  This can be
	    * transformed to planar 420 (YV12 or 4CC 0x32315659).
	    */
	   lav_fd->chroma = Y4M_CHROMA_422;
	   break;
	case e_dv_sample_none:
	   /* FIXME */
	   break;
	}
	free(frame);
	dv_decoder_free(decoder);
	lav_set_video_position(lav_fd,0);
	return 0;

ERREXIT:
	lav_close(lav_fd);
	if	(frame)
		free(frame);
	if	(decoder)
		dv_decoder_free(decoder);
	if	(output[0])
		free(output[0]);
	if	(output[1])
		free(output[1]);
	if	(ierr)
		internal_error = ierr;
	return 1;
	}
#endif

int lav_fileno(lav_file_t *lav_file)
{
   int res;

   video_format = lav_file->format; 

   switch(lav_file->format)
   {
      case 'a':
      case 'A':
         res = AVI_fileno(lav_file->avi_fd);
         break;
#ifdef HAVE_LIBQUICKTIME
      case 'q':
	res = lqt_fileno((quicktime_t *)lav_file->qt_fd);
	break;
#endif
      default:
         res = -1;
   }
   return res;
}

/* We need this to reorder the 32 bit values for big endian systems */
uint32_t reorder_32(uint32_t todo, int big_endian)
{
  unsigned char b0, b1, b2, b3;
  unsigned long reversed; 

  if( big_endian )
    {
    b0 = (todo & 0x000000FF);
    b1 = (todo & 0x0000FF00) >> 8;
    b2 = (todo & 0x00FF0000) >> 16;
    b3 = (todo & 0xFF000000) >> 24;

    reversed = (b0 << 24) + (b1 << 16) + (b2 << 8) +b3;
    return reversed;
    }
  return todo;
}

int lav_detect_endian (void)
{
    unsigned int fred;
    char     *pfred;

  fred = 2 | (1 << (sizeof(int)*8-8));
  pfred = (char *)&fred;

  if  (*pfred == 1)
      return 1;
  else if(*pfred == 2)
      return 0;
  else
      return -1;
}
