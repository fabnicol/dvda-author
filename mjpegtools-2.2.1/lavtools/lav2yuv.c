/* lav2yuv - stream any lav input file to stdout as YUV4MPEG data */

/* Copyright (C) 2000, Rainer Johanni, Andrew Stevens */
/* - added scene change detection code 2001, pHilipp Zabel */
/* - broke some common code out to lav_common.c and lav_common.h,
 *   July 2001, Shawn Sulma <lavtools@athos.cx>.  In doing this, I
 *   repackaged the numerous globals into three structs that get passed into
 *   the relevant functions.  See lav_common.h for a bit more information.
 *   Helpful feedback is welcome.
 */
/* - removed a lot of subsumed functionality and unnecessary cruft
 *   March 2002, Matthew Marjanovic <maddog@mir.com>.
 */

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lav_common.h"
#include <stdio.h>

void error(char *text);
void Usage(char *str);
void streamout(void);

int verbose = 1;

EditList el;

void Usage(char *str)
{
   fprintf(stderr,
   "Usage: %s [params] inputfiles\n"
   "   where possible params are:\n"
   "   -m         Force mono-chrome\n"
   "   -S list.el Output a scene list with scene detection\n"
   "   -T num     Set scene detection threshold to num (default: 4)\n"
   "   -D num     Width decimation to use for scene detection (default: 2)\n"
   "   -A w:h     Set output sample aspect ratio\n"
   "              (default:  read from DV files, or guess for MJPEG)\n"
   "   -P w:h     Declare the intended display aspect ratio (used to guess\n"
   "              the sample aspect ratio).  Common values are 4:3 and 16:9.\n"
   "              (default:  read from DV files, or assume 4:3 for MJPEG)\n"
   "   -C chroma  Set output chroma (default: '420jpeg')\n"
   "              '420jpeg', '420mpeg2', '420paldv', '422', '411' are available\n"
   "   -o num     Frame offset - skip num frames in the beginning\n"
   "              if num is negative, all but the last num frames are skipped\n"
   "   -f num     Only num frames are written to stdout (0 means all frames)\n"
   "   -c         Conceal corrupt jpeg frames by repeating previous frame\n"
   "   -x         Exchange fields\n",
  str);
   exit(0);
}

static int lum_mean;
static int last_lum_mean;
static int delta_lum;
static int scene_num;
static int scene_start;

LavParam param;
uint8_t *frame_bufs[6];

static int conceal_errframes;
static int altbuf;

void streamout(void)
{

	int framenum, movie_num=0, index[MAX_EDIT_LIST_FILES];
	int concealnum = 0;
	long int oldframe=N_EL_FRAME(el.frame_list[0])-1;
	FILE *fd=NULL;

	int fd_out = 1; /* stdout. */
   
	char temp[32];
	
	y4m_stream_info_t streaminfo;
	y4m_frame_info_t frameinfo;

	y4m_init_stream_info(&streaminfo);
	y4m_init_frame_info(&frameinfo);


	if (!param.scenefile)
		writeoutYUV4MPEGheader(fd_out, &param, el, &streaminfo);
	scene_num = scene_start = 0;
	if (param.scenefile)
	{
		int num_files;
		int i;
		
		param.output_width = 
			param.output_width / param.scene_detection_decimation;
		
		/*  Output file */

		unlink(param.scenefile);
		fd = fopen(param.scenefile,"w");
		if(fd==0)
		{
			mjpeg_error_exit1("Can not open %s - no edit list written!",param.scenefile);
		}
		fprintf(fd,"LAV Edit List\n");
		fprintf(fd,"%s\n",el.video_norm=='n'?"NTSC":"PAL");
		for(i=0;i<MAX_EDIT_LIST_FILES;i++) index[i] = -1;
		for(i=0;i<el.video_frames;i++) index[N_EL_FILE(el.frame_list[i])] = 1;
		num_files = 0;
		for(i=0;i<MAX_EDIT_LIST_FILES;i++) 
			if(index[i]==1) 
				index[i] = num_files++;
		fprintf(fd,"%d\n",num_files);
		for(i=0;i<MAX_EDIT_LIST_FILES;i++)
			if(index[i]>=0) 
				fprintf(fd,"%s\n",el.video_file_list[i]);
		sprintf(temp,"%d %ld",
				index[N_EL_FILE(el.frame_list[0])],
				N_EL_FRAME(el.frame_list[0]));
	}

	for (framenum = param.offset; framenum < (param.offset + param.frames); ++framenum) 
	{
		int rf;
		uint8_t **read_buf;
		uint8_t **frame_buf;
		if(conceal_errframes)
			read_buf = &frame_bufs[4 * (altbuf ^ 1)];
		else
			read_buf = &frame_bufs[0];
		
		rf = readframe(framenum, read_buf, &param, el);
		if(conceal_errframes) {
			if(rf == 2) {  // corrupt frame; repeat previous
				mjpeg_debug("corrupt jpeg data in frame %d; repeating previous frame.", framenum);
				frame_buf = &frame_bufs[4 * altbuf];
				concealnum++;
			} else {  // use new frame
				frame_buf = read_buf;
				altbuf ^= 1;
			}
		} else {
			if(rf == 2)
				mjpeg_debug("corrupt jpeg data in frame %d", framenum);
			frame_buf = &frame_bufs[0];
		}
		
		if (param.scenefile) 
		{
			lum_mean =
				luminance_mean(frame_buf, 
							   param.output_width, param.output_height );
			if (framenum == 0)
			{
				delta_lum = 0;
				movie_num = N_EL_FILE(el.frame_list[0]);
			}
			else
				delta_lum = abs(lum_mean - last_lum_mean);
			last_lum_mean = lum_mean;

			mjpeg_debug( "frame %d/%ld, lum_mean %d, delta_lum %d        ", framenum,
						 el.video_frames, lum_mean, delta_lum);

			if (delta_lum > param.delta_lum_threshold || index[N_EL_FILE(el.frame_list[framenum])] != movie_num ||
				oldframe+1 != N_EL_FRAME(el.frame_list[framenum])) {
				if (delta_lum <= param.delta_lum_threshold)
					fprintf(fd,"%c",'+'); /* Same scene, new file */
				fprintf(fd,"%s %ld\n", temp, N_EL_FRAME(el.frame_list[framenum-1]));
				sprintf(temp,"%d %ld",index[N_EL_FILE(el.frame_list[framenum])], N_EL_FRAME(el.frame_list[framenum]));
				scene_start = framenum;
				scene_num++;
			}

			oldframe = N_EL_FRAME(el.frame_list[framenum]);
			movie_num = N_EL_FILE(el.frame_list[framenum]);

		} 
		else
		{
		  int i;
		  i = y4m_write_frame(fd_out,
				      &streaminfo, &frameinfo, frame_buf);
		  if (i != Y4M_OK)
		    mjpeg_error("Failed to write frame: %s", y4m_strerr(i));
		}
	}
	mjpeg_info( "Repeated frames (for error concealment): %d", concealnum);
	if (param.scenefile)
	{
		fprintf(fd, "%s %ld\n", temp, N_EL_FRAME(el.video_frames-1));
		fclose(fd);
		mjpeg_info( "Editlist written to %s", param.scenefile);
	}
   
y4m_fini_frame_info(&frameinfo);
}

int main(argc, argv)
	int argc;
	char *argv[];
{
	int n, nerr = 0;
	int exchange_fields = 0;

	y4m_accept_extensions(1);

	param.offset = 0;
	param.frames = 0;
	param.mono = 0;
	param.scenefile = NULL;
	param.delta_lum_threshold = 4;
	param.scene_detection_decimation = 2;
	param.output_width = 0;
	param.output_height = 0;
	param.interlace = -1;
	param.sar = y4m_sar_UNKNOWN;
	param.dar = y4m_dar_4_3;
	param.chroma = Y4M_UNKNOWN;

	while ((n = getopt(argc, argv, "xmYv:S:T:D:o:f:P:A:C:c")) != EOF) {
		switch (n) {

		case 'v':
			verbose = atoi(optarg);
			if (verbose < 0 ||verbose > 2) {
				mjpeg_error( "-v option requires arg 0, 1, or 2");
				nerr++;
			}
			break;
		case 'm':
			param.mono = 1;
			break;
		case 'x':
			exchange_fields = 1;
			break;
		case 'c':
			conceal_errframes = 1;
			break;
		case 'S':
			param.scenefile = optarg;
			break;
		case 'T':
			param.delta_lum_threshold = atoi(optarg);
			break;
		case 'D':
			param.scene_detection_decimation = atoi(optarg);
			break;
		case 'o':
			param.offset = atoi(optarg);
			break;
		case 'f':
			param.frames = atoi(optarg);
			break;

		case 'A':
		  if (y4m_parse_ratio(&(param.sar), optarg)) {
		    mjpeg_error("Couldn't parse ratio '%s'", optarg);
		    nerr++;
		  }
		  break;
		case 'P':
		  if (y4m_parse_ratio(&(param.dar), optarg)) {
		    mjpeg_error("Couldn't parse ratio '%s'", optarg);
		    nerr++;
		  }
			break;
		case 'C':
		  param.chroma = y4m_chroma_parse_keyword(optarg);
		  switch (param.chroma) {
		  case Y4M_CHROMA_420JPEG:
		  case Y4M_CHROMA_420MPEG2:
		  case Y4M_CHROMA_420PALDV:
		  case Y4M_CHROMA_422:
		  case Y4M_CHROMA_411:
		    break;
		  default:
		    mjpeg_error("Unsupported chroma '%s'", optarg);
		    nerr++;
		    break;
		  }
		  break;
		default:
			nerr++;
		}
	}

	if (optind >= argc)
		nerr++;

	if (nerr)
		Usage(argv[0]);

	(void)mjpeg_default_handler_verbosity(verbose);

	/* Open editlist */

	read_video_files(argv + optind, argc - optind, &el,0);

	param.output_width = el.video_width;
	param.output_height = el.video_height;
	if(exchange_fields) {
	  if(el.video_inter == Y4M_ILACE_BOTTOM_FIRST) {
		mjpeg_info("Exchange from BOTTOM_FIRST to TOP_FIRST");
	  	el.video_inter = Y4M_ILACE_TOP_FIRST;
	  }
	  else if(el.video_inter == Y4M_ILACE_TOP_FIRST) {
		mjpeg_info("Exchange from TOP_FIRST to BOTTOM_FIRST");
	  	el.video_inter = Y4M_ILACE_BOTTOM_FIRST;
	  }
	  else {
		mjpeg_warn("Video NOT INTERLACED! Could not exchange fields");
	  }
	}

	if (param.offset < 0) {
		param.offset = el.video_frames + param.offset;
	}
	if (param.offset >= el.video_frames) {
		mjpeg_error_exit1("offset greater than # of frames in input");
	}
	if ((param.offset + param.frames) > el.video_frames) {
		mjpeg_warn("input too short for -f %d", param.frames);
		param.frames = el.video_frames - param.offset;
	}
	if (param.frames == 0) {
		param.frames = el.video_frames - param.offset;
	}

	param.interlace = el.video_inter;

	init(&param, &frame_bufs[0] /*&buffer*/);
	if(conceal_errframes)
		init(&param, &frame_bufs[4] /*&buffer*/);

#ifdef HAVE_LIBDV
	lav_init_dv_decoder();
#endif
	if (param.delta_lum_threshold != -1) 
	{
		streamout();
	}
	else 
	{
		write_edit_list(param.scenefile, 0, el.video_frames, &el);
	}

	return 0;
}
