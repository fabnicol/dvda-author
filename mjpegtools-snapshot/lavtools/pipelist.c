/*
 *  pipelist.[ch] - provides two functions to read / write pipe
 *                  list files, the "recipes" for lavpipe
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
#include <string.h>

#include "mjpeg_logging.h"

#include "pipelist.h"

extern int verbose;

int read_pipe_list (char *name, PipeList *pl)
{
   FILE *fd;
   char  line[1024];
   PipeSegment *seq;
   int   i, j, n;

   fd = fopen (name, "r");

   fgets (line, 1024, fd);
   if (strcmp (line, "LAV Pipe List\n") == 0) {

      /* 1. video norm */

      fgets (line, 1024, fd);
      if(line[0]!='N' && line[0]!='n' && line[0]!='P' && line[0]!='p') {
	 fclose(fd);
         mjpeg_error_exit1("Pipe list second line is not NTSC/PAL");
      }
     mjpeg_debug("Pipelist norm is %s",line[0]=='N'||line[0]=='n'?"NTSC":"PAL");

     if(line[0]=='N'||line[0]=='n')
         pl->video_norm = 'n';
     else
         pl->video_norm = 'p';

      /* 2. input streams */

      fgets (line, 1024, fd);
      if (sscanf (line, "%d", &(pl->source_count)) != 1) {
	 fclose(fd);
         mjpeg_error_exit1( "pipelist: # of input streams expected, \"%s\" found", line);
      }	  

      mjpeg_info( "Pipe list contains %d input streams", pl->source_count);

      pl->source_cmd = (char **) malloc (pl->source_count * sizeof (char *));

      for (i=0; i<pl->source_count; i++) {
         fgets(line,1024,fd);
         n = strlen(line);
         if(line[n-1]!='\n') {
            fclose(fd);
            mjpeg_error_exit1("Input cmdline in pipe list too long");
	 }
         line[n-1] = 0; /* Get rid of \n at end */
         pl->source_cmd[i] = (char *) malloc (n);
         strncpy (pl->source_cmd[i], line, n);
      }
      
      /* 3. sequences */
      
      pl->frame_count = 0;
      pl->segment_count = 0;
      pl->segments = (PipeSegment **) malloc (32 * sizeof (PipeSegment *));
      while (fgets (line, 1024, fd)) {
         seq = (PipeSegment *) malloc (sizeof (PipeSegment));
         
         /* 3.1. frames in sequence */

         if (sscanf (line, "%d", &(seq->frame_count)) != 1) {
            fclose(fd);
            mjpeg_error_exit1( "pipelist: # of frames in sequence expected, \"%s\" found", line);
         }
         if (seq->frame_count < 1) {
            fclose(fd);
	    mjpeg_error_exit1( "Pipe list contains sequence of length < 1 frame");
         }
         mjpeg_debug( "Pipe list sequence %d contains %d frames", 
					  pl->segment_count, seq->frame_count);
         pl->frame_count += seq->frame_count;
         
         /* 3.2. input streams */
     
         if (fgets (line, 1024, fd) == NULL) {
            fclose(fd);
            mjpeg_error_exit1("pipelist: end of file reading # of streams");
         }
         if (sscanf (line, "%d", &(seq->input_count)) != 1) {
            fclose(fd);
            mjpeg_error_exit1( "pipelist: # of streams in sequence expected, \"%s\" found", line);
         }
         seq->input_index = (int *) malloc (seq->input_count * sizeof (int));
         seq->input_offset = (unsigned long *) malloc (seq->input_count * sizeof (unsigned long));
         for (i=0; i<seq->input_count; i++) {
            fgets (line, 1024, fd);	/* ignore eof - will detect later */
            j = sscanf (line, "%d %lud", &(seq->input_index[i]), &(seq->input_offset[i]));
            if (j == 1) {
               /* if no offset is given, assume ofs = 0 */
               seq->input_offset[i] = 0;
               j++;
            }
            if (j != 2) {
               fclose(fd);
               mjpeg_error( "pipelist: input stream index & offset expected, \"%s\" found", line);
               return -1;
            }
            if (seq->input_index[i] >= pl->source_count) {
               fclose(fd);
               mjpeg_error( "Sequence requests input stream that is not contained in pipe list");
               return(-1);
            }
         }

         /* 3.3. output cmd */

         if (fgets(line, 1024, fd) == NULL) {
            fclose(fd);
            mjpeg_error_exit1( "Error in pipe list: Unexpected end");
         }
         n = strlen(line);
         if(line[n-1]!='\n') {
            fclose(fd);
            mjpeg_error("Output cmdline in pipe list too long");
            return(-1);
         }
         line[n-1] = 0; /* Get rid of \n at end */
         seq->output_cmd = (char *) malloc (n);
         strncpy (seq->output_cmd, line, n);
         
         pl->segments[pl->segment_count++] = seq;
         if ((pl->segment_count % 32) == 0)
         pl->segments = (PipeSegment **) realloc (pl->segments, 
                   (pl->segment_count + 32) * sizeof (PipeSegment *));
            pl->segments = (PipeSegment **) realloc (pl->segments, sizeof (pl->segments) + 32 * sizeof (PipeSegment *));
      }
      fclose(fd);
      return 0;
   }
   /* errno = EBADMSG; */
   fclose(fd);
   return -1;
}

int write_pipe_list (char *name, PipeList *pl)
{
   FILE *fd;
   int   i, n;

   fd = fopen (name, "w");

   fprintf (fd, "LAV Pipe List\n");

   /* 1. video norm */

   fprintf (fd, "%s\n", (pl->video_norm == 'n') ? "NTSC" : "PAL");

   /* 2. input streams */

   fprintf (fd, "%d\n", pl->source_count);

   for (i=0; i<pl->source_count; i++)
      fprintf (fd, "%s\n", pl->source_cmd[i]);

   /* 3. sequences */

   for (i=0; i<pl->segment_count; i++) {
      
      PipeSegment *seq = pl->segments[i];

      /* 3.1. frames in sequence */
      
      fprintf (fd, "%d\n", seq->frame_count);

      /* 3.2. input streams */
      
      fprintf (fd, "%d\n", seq->input_count);
      for (n=0; n<seq->input_count; n++)
         fprintf (fd, "%d %lud\n", seq->input_index[n], seq->input_offset[n]);

      /* 3.3. output cmd */
      
      fprintf (fd, "%s\n", seq->output_cmd);
   }
   fclose(fd);
   return 0;

}
