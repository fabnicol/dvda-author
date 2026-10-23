/*
 *  lavpipe - combines several input streams and pipes them trough
 *            arbitrary filters in order to finally output a resulting
 *            video stream based on a given "recipe" (pipe list)
 *
 *  Copyright (C) 2001, pHilipp Zabel <pzabel@gmx.de>
 *  Copyright (C) 2001, Matthew Marjanovic <maddog@mir.com>
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "mjpeg_logging.h"
#include "pipelist.h"
#include "yuv4mpeg.h"

static void usage(void)
{
  fprintf(
  stderr, 
  "Usage: lavpipe [options] <pipe list>\n"
  "Options: -o num   Frame offset - skip num frames in the beginning\n"
  "                  if num is negative, all but the last num frames are skipped\n"
  "         -n num   Only num frames are processed (0 means all frames)\n"
  "         -v num  Verbosity of output [0..2]\n"
  );
}

static
char **parse_spawn_command(char *cmdline)
{
  char **argv;
  char *p = cmdline;
  int i, argc = 0;
  
  if (p == NULL) return NULL;
  if (*p == '\0') return NULL;

  argc = 0;
  while (*p != '\0') {    
    while (!isspace(*p)) {
      p++;
      if (*p == '\0') {
	argc++;
	goto END_OF_LINE;
      }
    }
    argc++;     
    while (isspace(*p)) {
      p++;
      if (*p == '\0') goto END_OF_LINE;
    }
  }

 END_OF_LINE:
  argv = malloc(argc+1 * sizeof(argv[0]));

  for (p = cmdline, i=0; i < argc; i++) {
    argv[i] = p;
    while (!isspace(*(++p)));
    p[0] = '\0';
    while (isspace(*(++p)))
      if (p[0] == '\0') break;
  }

  argv[argc] = NULL;  
  return argv;
}

static 
pid_t fork_child_sub(char *command, int *fd_in, int *fd_out)
{
  int n;
  int pipe_in[2];
  int pipe_out[2];
  char **myargv;
  pid_t pid;
  
  if (fd_in) {
    if (pipe(pipe_in)) {
      mjpeg_error_exit1( "Couldn't create input pipe from %s", command);
    }
  }
  if (fd_out) {
    if (pipe(pipe_out)) {
      mjpeg_error_exit1( "Couldn't create output pipe to %s", command);
    }
  }         
  if ((pid = fork ()) < 0) {
    mjpeg_error_exit1("Couldn't fork %s", command);
  }
   
  if (pid == 0) {
    /* child redirects stdout to pipe_in */
    if (fd_in) {
      close(pipe_in[0]);
      close(1);
      n = dup(pipe_in[1]);
      if (n != 1) exit (1);
    }
    /* child redirects stdin to pipe_out */
    if (fd_out) {
      close(pipe_out[1]);
      close(0);
      n = dup(pipe_out[0]);
      if (n != 0) exit(1);
    }
    myargv = parse_spawn_command(command);
    execvp(myargv[0], myargv);
    return -1;
  } else {
    /* parent */
    if (fd_in != NULL) {
      close(pipe_in[1]);
      *fd_in = pipe_in[0];
    }
    if (fd_out != NULL) {
      close(pipe_out[0]);
      *fd_out = pipe_out[1];
    }
    return pid;
  }
}


static pid_t fork_child(const char *command,
			int offset, int num,
			int *fd_in, int *fd_out)
{
  char tmp1[1024], tmp2[1024];
  char *current = tmp1;
  char *next = tmp2;
  char *p;

  strncpy(current, command, 1024);
   
  /* replace $o by offset */
  p = strstr(current, "$o");
  if (p) {
    p[0] = '\0';
    p += 2;
    snprintf(next, 1024, "%s%d%s", current, offset, p);
    p = current;
    current = next;
    next = p;
  }
  
  /* replace $n by number of frames */
  p = strstr(current, "$n");
  if (p) {
    p[0] = '\0';
    p += 2;
    snprintf(next, 1024, "%s%d%s", current, num, p);
    p = current;
    current = next;
    next = p;
  }
  
  mjpeg_debug( "Executing: '%s'", current);
  return fork_child_sub(current, fd_in, fd_out);
}

static void alloc_yuv_buffers(unsigned char *yuv[3], y4m_stream_info_t *si)
{
int chroma_ss, ss_v, ss_h, w, h;

  w = y4m_si_get_width(si);
  h = y4m_si_get_height(si);

  chroma_ss = y4m_si_get_chroma(si);
  ss_h = y4m_chroma_ss_x_ratio(chroma_ss).d;
  ss_v = y4m_chroma_ss_y_ratio(chroma_ss).d;

  yuv[0] = malloc (w * h * sizeof(yuv[0][0]));
  yuv[1] = malloc ((w / ss_h) * (h / ss_v) * sizeof(yuv[1][0]));
  yuv[2] = malloc ((w / ss_h) * (h / ss_v) * sizeof(yuv[2][0]));
}

static void free_yuv_buffers(unsigned char *yuv[3])
{
  free(yuv[0]);
  free(yuv[1]);
  free(yuv[2]);
  yuv[0] = yuv[1] = yuv[2] = NULL;
}

static void init_pipe_source(pipe_source_t *ps, char *command)
{
  ps->command = strdup(command);
  y4m_init_stream_info(&(ps->streaminfo));
  y4m_init_frame_info(&(ps->frameinfo));
  ps->pid = -1;
  ps->fd = -1;
  ps->frame_num = 0;
}

static void fini_pipe_source(pipe_source_t *ps)
{
  free(ps->command);
  y4m_fini_stream_info(&(ps->streaminfo));
  y4m_fini_frame_info(&(ps->frameinfo));
}

static void spawn_pipe_source(pipe_source_t *ps, int offset, int count)
{
  ps->pid = fork_child(ps->command,
		       offset, count,
		       &(ps->fd), NULL);
  ps->frame_num = offset;
  mjpeg_debug("spawned source '%s'", ps->command);
}

static void decommission_pipe_source(pipe_source_t *source)
{
  if (source->fd >= 0) {
    close(source->fd);
    source->fd = -1;
  }
  if (source->pid > 0) {
    mjpeg_debug("DIE DIE DIE pid %d", source->pid);
    kill(source->pid, SIGINT);
    source->pid = -1;
  }
}

static void init_pipe_filter(pipe_filter_t *pf, const char *command)
{
  pf->command = strdup(command);
  y4m_init_stream_info(&(pf->out_streaminfo));
  y4m_init_stream_info(&(pf->in_streaminfo));
  y4m_init_frame_info(&(pf->frameinfo));
  pf->yuv[0] = pf->yuv[1] = pf->yuv[2] = NULL;
  pf->pid = -1;
  pf->out_fd = -1;
  pf->in_fd = -1;
}

static void fini_pipe_filter(pipe_filter_t *pf)
{
  free(pf->command);
  free_yuv_buffers(pf->yuv);
  y4m_fini_stream_info(&(pf->out_streaminfo));
  y4m_fini_stream_info(&(pf->in_streaminfo));
  y4m_fini_frame_info(&(pf->frameinfo));
}

static void spawn_pipe_filter(pipe_filter_t *pf, int offset, int count)
{
  pf->pid = fork_child(pf->command,
		       offset, count,
		       &(pf->in_fd), &(pf->out_fd));
  mjpeg_debug("spawned filter '%s'", pf->command);
}

static void decommission_pipe_filter(pipe_filter_t *filt)
{
  if (filt->in_fd >= 0) {
    close(filt->in_fd);
    filt->in_fd = -1;
  }
  if (filt->out_fd >= 0) {
    close(filt->out_fd);
    filt->out_fd = -1;
  }
  if (filt->pid > 0) {
    mjpeg_debug("DIE DIE DIE pid %d", filt->pid);
    kill(filt->pid, SIGINT);
    filt->pid = -1;
  }
  free_yuv_buffers(filt->yuv);
}    

/*
 * make sure all the sources needed for this segment are cued up
 *  and ready to produce frames
 *
 */

static
void open_segment_inputs(PipeSegment *seg, pipe_filter_t *filt,
			 int frame, int segnum, int total_frames,
			 PipeList *pl, commandline_params_t *cl,
			 pipe_source_t *sources) 
{
  int i, j, k;
  
  for (i = 0; i < seg->input_count; i++) {
    int in_index = seg->input_index[i];
    int in_offset = seg->input_offset[i];
    pipe_source_t *source = &(sources[in_index]);

    mjpeg_debug("OSI:  input %d == source %d: '%s'",
		i, in_index, source->command);

    /* spawn the source if not already running */
    if (source->fd < 0) {
      
      /* calculate # of frames we want to get from this stream */
      /* need to look if we can use this in successive sequences and
	 what param_frames is */
      
      int offset = in_offset + frame;
      int count = seg->frame_count - frame; /* until end of sequence */
      
      for (j = segnum + 1; j < pl->segment_count; j++) {
	PipeSegment *other = pl->segments[j];
	/*	mjpeg_debug("checking  i %d   j %d", i, j); */
	for (k = 0; k < other->input_count; k++) {
	  /*	  mjpeg_debug("checking  i %d   j %d  k %d", i, j, k); */
	  if (in_index == other->input_index[k]) {
	    if ((offset + count) == other->input_offset[k]) {
	      count += other->frame_count; /* add another sequence */
	    } else
	      goto FINISH_CHECK; /* need to reopen with other offset */
	  } else
	    goto FINISH_CHECK; /* stream will not be used in
				  segment j anymore */
	}
      }
    FINISH_CHECK:
      /*      mjpeg_debug("finish-check  i %d   j %d  k %d", i, j, k); */
      if (count > cl->frames - total_frames) {
	count = cl->frames - total_frames;
      }

      /******** why have 'count'?  let the source keep making frames...
	 ...we'll just kill it when we are done anyway! *********/
      /*      spawn_pipe_source(source, offset, count);*/
      spawn_pipe_source(source, offset, 0);

      if (y4m_read_stream_header(source->fd, &(source->streaminfo)) != Y4M_OK)
	mjpeg_error_exit1("Bad source header!");

      mjpeg_debug("read header");
      y4m_log_stream_info(mjpeg_loglev_t("debug"), "src: ", &(source->streaminfo));
    } else {
      mjpeg_debug("...source %d is still alive.", in_index);
    }
    
    if (i == 0) {
      /* first time:  copy stream info to filter */
      y4m_copy_stream_info(&(filt->out_streaminfo), &(source->streaminfo));
      mjpeg_debug("copied info");
    } else {
      /* n-th time:  make sure source streams match */
      if (y4m_si_get_width(&(filt->out_streaminfo)) != 
	  y4m_si_get_width(&(source->streaminfo))) 
	mjpeg_error_exit1("Stream mismatch:  frame width");
      if (y4m_si_get_height(&(filt->out_streaminfo)) != 
	  y4m_si_get_height(&(source->streaminfo))) 
	mjpeg_error_exit1("Stream mismatch:  frame height");
      if (y4m_si_get_interlace(&(filt->out_streaminfo)) != 
	  y4m_si_get_interlace(&(source->streaminfo))) 
	mjpeg_error_exit1("Stream mismatch:  interlace");
      mjpeg_debug("verified info");
    }
  }
  
}

static
void setup_segment_filter(PipeSegment *seg, pipe_filter_t *filt, int frame)
{
  mjpeg_debug("OSO:  '%s'", filt->command);
  if (strcmp(filt->command, "-")) {
    /* ...via a filter command:
     *     o spawn filter process
     *     o write source stream info to filter
     *     o read filter's result stream info
     *     o alloc yuv buffers for source->filter transfers
     */
    /* ... why does the 'count' matter, if lavpipe controls the frame
     *      flow anyway???????? */
    spawn_pipe_filter(filt, frame, (seg->frame_count - frame));
    y4m_write_stream_header(filt->out_fd, &(filt->out_streaminfo));
    y4m_read_stream_header(filt->in_fd, &(filt->in_streaminfo));
    mjpeg_debug("SSF:  read filter result stream header");
    y4m_log_stream_info(mjpeg_loglev_t("debug"), "result: ", &(filt->in_streaminfo));
    alloc_yuv_buffers(filt->yuv, &(filt->out_streaminfo));
  } else {
    /* ...no filter; direct output:
     *     o result stream info is just a copy of the source stream info
     */
    filt->out_fd = filt->in_fd = -1;
    y4m_copy_stream_info(&(filt->in_streaminfo), &(filt->out_streaminfo));
    mjpeg_debug("SSF:  copied source header");
  }
}


static
void process_segment_frames(pipe_sequence_t *ps, int segnum,
			    int *frame, int *total_frames)
{
  pipe_source_t *sources = ps->sources;
  pipe_filter_t *the_output = &(ps->output);
  PipeList *pl = &(ps->pl);
  commandline_params_t *cl = &(ps->cl);

  PipeSegment *seg = ps->pl.segments[segnum];
  pipe_filter_t *filt = &(ps->filters[segnum]);


  mjpeg_debug("PSF:  segment %d,  initial frame = %d", segnum, *frame);
  while (*frame < seg->frame_count) {
    int i;
    
    for (i = 0; i < seg->input_count; i++) {
      int in_index = seg->input_index[i];
      pipe_source_t *source = &(sources[in_index]);
      unsigned char **yuv;
      
      if (filt->out_fd >= 0)
        /* filter present; write/read through filter's buffer first */
	yuv = filt->yuv;
      else
	/* no filter present; source -> direct to output buffer */
	yuv = the_output->yuv;

      mjpeg_debug("read frame %03d > input %d, src %d  fd = %d", 
		  *frame, i, in_index, source->fd);
      if (y4m_read_frame(source->fd,
			 &(source->streaminfo), &(source->frameinfo),
			 yuv) != Y4M_OK) {
	int err = errno;
	mjpeg_error("ERRNO says:  %s", strerror(err));
	mjpeg_error_exit1("lavpipe: input stream error in stream %d,"
			  "sequence %d, frame %d", 
			  i, segnum, *frame);
      }
      source->frame_num += 1;
      
      if (filt->out_fd >= 0)
	y4m_write_frame(filt->out_fd,
			&(filt->out_streaminfo), &(filt->frameinfo),
			yuv);
    }
    
    if (filt->in_fd >= 0) {
      if (y4m_read_frame(filt->in_fd,
			 &(filt->in_streaminfo), &(filt->frameinfo),
			 the_output->yuv) != Y4M_OK) {
	mjpeg_error_exit1( "lavpipe: filter stream error in sequence %d,"
			   "frame %d",
			   segnum, *frame);
      }
    }
    
    /* output result */
    y4m_write_frame(the_output->out_fd, 
		    &(the_output->out_streaminfo), 
		    &(the_output->frameinfo),
		    the_output->yuv);
    
    (*frame)++;
    if (++(*total_frames) == cl->frames) {
      segnum = pl->segment_count - 1; /* = close all input files below */
      break;
    }
  }
}

/* 
 * this is just being picky, but...
 * 
 * Close all sources used by current segment, but only if they will
 *  cannot be used in current state by upcoming segments.
 *
 */

static
void close_segment_inputs(pipe_sequence_t *ps, int segnum, int frame)
{
  PipeList *pl = &(ps->pl);
  pipe_source_t *sources = ps->sources;
  PipeSegment *seg = pl->segments[segnum];
  int i;

  for (i = 0; i < seg->input_count; i++) {
    int current_index = seg->input_index[i];
    pipe_source_t *source = &(sources[current_index]);

    if (source->fd >= 0) {
      /* if it's still alive...
       * ...iterate over remaining segments, and see if they can
       *    use this source. 
       */
      int s;
      for (s = segnum + 1; s < pl->segment_count; s++) {
	int j;
	PipeSegment *next_seg = pl->segments[s];
	
	for (j = 0; j < next_seg->input_count; j++) {
	  int index = next_seg->input_index[j];
	  int offset = next_seg->input_offset[j];
	  
	  if ( (index == current_index) &&
	       (offset == source->frame_num) ) {
	    /* A segment input offset matches the current frame...
	     * ...this source can still be used.
	     */
	    mjpeg_info("allowing input %d (source %d) to live",
		       i, current_index);
	    goto KEEP_SOURCE;
	  }
	}
      }
      mjpeg_info( "closing input %d (source %d)", i, current_index);
      decommission_pipe_source(source);
KEEP_SOURCE: ;
    }
  }
}

static
void parse_command_line(int argc, char *argv[], commandline_params_t *cl)
{
  char c;
  int err;

  cl->verbose = 1;
  cl->offset = 0;
  cl->frames = 0; 
  cl->listfile = NULL;
  
  err = 0;
  while ((c = getopt(argc, argv, "o:n:v:")) != EOF) {
    switch (c) {
    case 'o':
      cl->offset = atoi(optarg);
      break;
    case 'n':
      cl->frames = atoi(optarg);
      break;
    case 'v':
      cl->verbose = atoi(optarg);
      if ( (cl->verbose < 0) || (cl->verbose > 2) ) {
	usage();
	exit(1);
      }
      break;
    default:
      err++;
    }
  }
  if ((optind >= argc) || (err)) {
    usage();
    exit(1);
  }
  cl->listfile = strdup(argv[optind]);
}

static
void initialize_pipe_sequence(pipe_sequence_t *ps, int argc, char **argv)
{
  int i;
  commandline_params_t *cl = &(ps->cl);
  PipeList *pl = &(ps->pl);

  init_pipe_filter(&(ps->output), "");
  ps->output.out_fd = 1;

  /* parse command-line arguments */
  parse_command_line(argc, argv, cl);

  /* set-up logging */
  (void)mjpeg_default_handler_verbosity(cl->verbose);

  /* read pipe 'recipe' */
  if (read_pipe_list(cl->listfile, pl) < 0) {
    mjpeg_error_exit1( "lavpipe: couldn't open \"%s\"", cl->listfile);
  }
  
  /* a negative offset means "from the end" */
  if (cl->offset < 0) {
    cl->offset = pl->frame_count + cl->offset;
  }   
  if ((cl->offset >= pl->frame_count) ||
      (cl->offset < 0)) {
    mjpeg_error_exit1( "error: offset greater than # of frames in input");
  }

  /* zero frame count means "all frames" */
  if (cl->frames == 0) {
    cl->frames = pl->frame_count - cl->offset;
  }
  if ((cl->offset + cl->frames) > pl->frame_count) {
    mjpeg_warn( "input too short for -n %d", cl->frames);
    cl->frames = pl->frame_count - cl->offset;
  }
  
  /* initialize pipe sources */
  ps->sources = malloc(pl->source_count * sizeof(ps->sources[0]));
  for (i = 0; i < pl->source_count; i++) 
    init_pipe_source(&(ps->sources[i]), pl->source_cmd[i]);

  /* initialize pipe filters */
  ps->filters = malloc(pl->segment_count * sizeof(ps->filters[0]));
  for (i = 0; i < pl->segment_count; i++) 
    init_pipe_filter(&(ps->filters[i]), pl->segments[i]->output_cmd);
  
}

static
void process_pipe_sequence(pipe_sequence_t *ps)
{
  int segm_num;       /* current segment number */
  int segm_frame;     /* frame number, within a segment */
  int sequ_frame;     /* cumulative/total frame number  */
  int first_iteration;
  
  /* find start segment/frame, given overall lavpipe offset ("-o") */
  segm_frame = ps->cl.offset;
  for (segm_num = 0;
       segm_frame >= ps->pl.segments[segm_num]->frame_count;
       segm_num++) {
    segm_frame -= ps->pl.segments[segm_num]->frame_count;
  }

  /* process the segments */
  first_iteration = 1;
  sequ_frame = 0;
  while ( (segm_num < ps->pl.segment_count) && 
	  (sequ_frame < ps->cl.frames) ) {
    PipeSegment *seg = ps->pl.segments[segm_num];
    pipe_filter_t *filt = &(ps->filters[segm_num]);
    
    mjpeg_debug("starting segment %d, frame %d", segm_num, segm_frame);
    
    open_segment_inputs(seg, filt, segm_frame, segm_num, sequ_frame,
			&ps->pl, &ps->cl, ps->sources);
    setup_segment_filter(seg, filt, segm_frame);
    
    if (first_iteration) {
      /* Initialize the final output stream, just once */
      
      /* The final output stream parameters are taken from the output
       *  parameters of the first segment's output filter.
       * (If there is no filter (i.e. "-", direct output), then the
       *  parameters will end up coming from the first segment's source
       *  stream.)
       */
      y4m_copy_stream_info(&(ps->output.out_streaminfo),
			   &(filt->in_streaminfo));
      y4m_write_stream_header(ps->output.out_fd, &(ps->output.out_streaminfo));
      alloc_yuv_buffers(ps->output.yuv,  &(ps->output.out_streaminfo));
      mjpeg_debug("output stream initialized");
      first_iteration = 0;
    } else {
      /* For succeeding segments, make sure that the new filter's stream is
       *  consistent with the final output stream.
       */
      if (y4m_si_get_width(&(filt->in_streaminfo)) != 
	  y4m_si_get_width(&(ps->output.out_streaminfo)))
	mjpeg_error_exit1("Stream mismatch:  frame width");
      if (y4m_si_get_height(&(filt->in_streaminfo)) != 
	  y4m_si_get_height(&(ps->output.out_streaminfo)))
	mjpeg_error_exit1("Stream mismatch:  frame height");
      if (y4m_si_get_interlace(&(filt->in_streaminfo)) != 
	  y4m_si_get_interlace(&(ps->output.out_streaminfo)))
	mjpeg_error_exit1("Stream mismatch:  interlace");
      mjpeg_debug("filter stream verified");
    }      
    
    process_segment_frames(ps, segm_num, &segm_frame, &sequ_frame);
    decommission_pipe_filter(filt);
    close_segment_inputs(ps, segm_num, segm_frame);
    
    /* prepare for next sequence */
    segm_num++;
    segm_frame = 0;
  }
}

static
void cleanup_pipe_sequence(pipe_sequence_t *ps)
{
  int i;
  PipeList *pl = &(ps->pl);

  /* free/fini everything */
  fini_pipe_filter(&ps->output);
  for (i = 0; i < pl->source_count; i++) 
    fini_pipe_source(&(ps->sources[i]));
  free(ps->sources);
  for (i = 0; i < pl->segment_count; i++) 
    fini_pipe_filter(&(ps->filters[i]));
  free(ps->filters);
}


int main (int argc, char *argv[]) 
{
  pipe_sequence_t ps;

  initialize_pipe_sequence(&ps, argc, argv);
  process_pipe_sequence(&ps);
  cleanup_pipe_sequence(&ps);
  return 0;
}
