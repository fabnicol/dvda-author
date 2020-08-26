/* lavinfo - give info 
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
#include <stdio.h>
#include <lav_io.h>
#include <editlist.h>
#include <mjpeg_logging.h>

int verbose = -1;

EditList el;

int main(int argc, char *argv[])
{
  const char *p;

   if(argc <=1) {
      printf("Usage: %s file1 [ file2 file3 ... ]\n",argv[0]);
      printf("\ttakes a list of editlist or lav files, and prints out info about them\n");
      printf("\tmust all be of same dimensions\n");
      exit(1);
   }
	(void)mjpeg_default_handler_verbosity(verbose);
	read_video_files(argv+1, argc-1, &el,0);

   /* Video  */
   printf("video_frames=%li\n",el.video_frames);
   printf("video_width=%li\n",el.video_width);
   printf("video_height=%li\n",el.video_height);
   switch (el.video_inter)
          {
          case Y4M_ILACE_NONE:
	       p = "p";
	       break;
	  case Y4M_ILACE_BOTTOM_FIRST:
	       p = "b";
	       break;
	  case Y4M_ILACE_TOP_FIRST:
	       p = "t";
	       break;
	  case Y4M_ILACE_MIXED:
	       p = "m";
	       break;
	  default:
	       p = "***BOGUS/UNKNOWN***";
	       break;
	  }
   printf("video_inter=%s\n", p);
   printf("video_norm=%s\n",el.video_norm=='n'?"NTSC":"PAL");
   printf("video_fps=%f\n",el.video_fps);
   printf("video_sar_width=%i\n",el.video_sar_width);
   printf("video_sar_height=%i\n",el.video_sar_height);
   printf("max_frame_size=%li\n",el.max_frame_size);
   p = y4m_chroma_keyword(el.chroma);
   if (p == NULL)
      p = "Bogus/unknown chroma sampling";
   printf("chroma=%s\n", p);
   /* Audio */
   printf("has_audio=%i\n",el.has_audio);
   printf("audio_bps=%i\n",el.audio_bps);
   printf("audio_chans=%i\n",el.audio_chans);
   printf("audio_bits=%i\n",el.audio_bits);
   printf("audio_rate=%ld\n",el.audio_rate);
   /* Misc */
   printf("num_video_files=%li\n",el.num_video_files);
   exit(0);
}
