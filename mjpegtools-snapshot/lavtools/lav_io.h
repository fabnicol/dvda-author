/*
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

#ifndef LAV_IO_H
#define LAV_IO_H

#include <avilib.h>

#ifdef HAVE_LIBQUICKTIME
#include <quicktime.h>
#include <lqt.h>
#else
typedef void quicktime_t;
#endif

#include <yuv4mpeg.h>

#define MAX_MBYTES_PER_FILE_64 (4194304 - 1024)   /* 4TB-1GB should be enough */
#define MAX_MBYTES_PER_FILE_32 (2048 - 4)         /* 2GB-4MB */

/* raw data format of a single frame */
#define DATAFORMAT_MJPG     0
#define DATAFORMAT_DV2      1
#define DATAFORMAT_YUV420   2
#define DATAFORMAT_YUV422   3

typedef struct
{
   avi_t *avi_fd;
   int	jpeg_fd;
   char	*jpeg_filename;
   quicktime_t *qt_fd;
   int	format;
   int	interlacing;
   int	sar_w;  /* "sample aspect ratio" width  */
   int	sar_h;  /* "sample aspect ratio" height */
   int	has_audio;
   int	bps;
   int	chroma;
   int	dataformat;
} lav_file_t;

#ifdef HAVE_LIBDV
extern int libdv_pal_yv12;
#endif

int  lav_query_APP_marker(char format);
int  lav_query_APP_length(char format);
int  lav_query_polarity(char format);
lav_file_t *lav_open_output_file(char *filename, char format,
                    int width, int height, int interlaced, double fps,
                    int asize, int achans, long arate);
int  lav_close(lav_file_t *lav_file);
int  lav_write_frame(lav_file_t *lav_file, uint8_t *buff, long size, long count);
int  lav_write_audio(lav_file_t *lav_file, uint8_t *buff, long samps);
long lav_video_frames(lav_file_t *lav_file);
int  lav_video_width(lav_file_t *lav_file);
int  lav_video_height(lav_file_t *lav_file);
double lav_frame_rate(lav_file_t *lav_file);
int  lav_video_interlacing(lav_file_t *lav_file);
void lav_video_sampleaspect(lav_file_t *lav_file, int *sar_w, int *sar_h);
int  lav_video_chroma(lav_file_t *lav_file);
const char *lav_video_compressor(lav_file_t *lav_file);
int  lav_audio_channels(lav_file_t *lav_file);
int  lav_audio_bits(lav_file_t *lav_file);
long lav_audio_rate(lav_file_t *lav_file);
long lav_audio_samples(lav_file_t *lav_file);
long lav_frame_size(lav_file_t *lav_file, long frame);
int  lav_seek_start(lav_file_t *lav_file);
int  lav_set_video_position(lav_file_t *lav_file, long frame);
int  lav_read_frame(lav_file_t *lav_file, uint8_t *vidbuf);
int  lav_set_audio_position(lav_file_t *lav_file, long sample);
long lav_read_audio(lav_file_t *lav_file, uint8_t *audbuf, long samps);
lav_file_t *lav_open_input_file(char *filename);
int  lav_get_field_size(uint8_t * jpegdata, long jpeglen);
const char *lav_strerror(void);
int  lav_fileno( lav_file_t *lav_file );
uint32_t reorder_32(uint32_t, int);
int  lav_detect_endian (void);
#endif
