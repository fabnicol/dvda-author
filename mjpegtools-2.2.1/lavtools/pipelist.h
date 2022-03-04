/*
 *  pipelist.[ch] - provides two functions to read / write pipe
 *                  list files, the "recipes" for lavpipe
 *
 *  Copyright (C) 2001, pHilipp Zabel <pzabel@gmx.de>
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
#ifndef __PIPELIST_H__
#define __PIPELIST_H__


#include "yuv4mpeg.h"


typedef struct {
  int            frame_count;  /* frame_num;    */
  int            input_count;  /* input_num */
  int           *input_index;  /* input_ptr */
  unsigned long *input_offset; /* input_ofs */
  char          *output_cmd;
} PipeSegment; /* PipeSequence */


typedef struct {
  char           video_norm;
  int            frame_count;  /* frame_num; */
  int            source_count; /*input_num;  */
  char         **source_cmd;  /* input_cmd */
  int            segment_count;  /* seq_num; */
  PipeSegment **segments; /* seq_ptr; */
} PipeList;



typedef struct _commandline_params {
  int verbose;
  int offset;
  int frames;
  char *listfile;
} commandline_params_t;


typedef struct _pipe_source {
  int pid;
  int fd;
  char *command;
  y4m_stream_info_t streaminfo;
  y4m_frame_info_t frameinfo;
  int frame_num;   /* number of next frame to be read from source */
} pipe_source_t;


typedef struct _pipe_filter {
  int pid;
  int out_fd;  /* out -to- filter  */
  int in_fd;   /* in -from- filter */
  char *command;
  y4m_stream_info_t out_streaminfo;
  y4m_stream_info_t in_streaminfo;
  y4m_frame_info_t frameinfo;
  unsigned char *yuv[3];  /* buffers for output -to- filter */
} pipe_filter_t;


typedef struct _pipe_sequence {
  PipeList pl;
  commandline_params_t cl;
  pipe_source_t *sources;
  pipe_filter_t *filters;
  pipe_filter_t output;
} pipe_sequence_t;





int read_pipe_list (char *name, PipeList *pl);
int write_pipe_list (char *name, PipeList *pl);

#endif

