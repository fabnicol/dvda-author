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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#include "mjpeg_logging.h"
#include "lav_io.h"
#include "editlist.h"
#include <math.h>


/* Since we use malloc often, here the error handling */

static void malloc_error(void)
{
	mjpeg_error_exit1("Out of memory - malloc failed");
}

int open_video_file(char *filename, EditList *el, int preserve_pathname)
{
   int i, n, nerr;
   char realname[PATH_MAX];

   /* Get full pathname of file if the user hasn't specified preservation
	  of pathnames...
	*/

   if( preserve_pathname )
   {
	   strcpy(realname, filename);
   }
   else if(realpath(filename,realname)==0)
   {
	   mjpeg_error_exit1( "Cannot deduce real filename: %s", strerror(errno));
   }

   /* Check if this filename is allready present */

   for(i=0;i<el->num_video_files;i++)
      if(strcmp(realname,el->video_file_list[i])==0)
      {
		  mjpeg_error("File %s already open",realname);
		  return i;
      }

   /* Check if MAX_EDIT_LIST_FILES will be exceeded */

   if(el->num_video_files>=MAX_EDIT_LIST_FILES)
   {
	   mjpeg_error_exit1("Maximum number of video files exceeded");
   }

   n = el->num_video_files;
   el->num_video_files++;

   mjpeg_debug("Opening video file %s ...",filename);

   el->lav_fd[n] = lav_open_input_file(filename);
   if(!el->lav_fd[n])
   {
      mjpeg_error_exit1("Error opening %s",filename);
   }
   if(lav_video_chroma(el->lav_fd[n]) != Y4M_CHROMA_422 &&
	   lav_video_chroma(el->lav_fd[n]) != Y4M_CHROMA_420JPEG &&
	   lav_video_chroma(el->lav_fd[n]) != Y4M_CHROMA_411)
   {
      mjpeg_warn("Input file %s is not in  JPEG 4:2:2 or 4:2:0 or 4:1:1 format",
				 filename);
      el->chroma = Y4M_UNKNOWN;
   }
   el->num_frames[n] = lav_video_frames(el->lav_fd[n]);

   el->video_file_list[n] = strdup(realname);
   if(el->video_file_list[n]==0) malloc_error();

   /* Debug Output */

   mjpeg_debug("File: %s, absolute name: %s",filename,realname);
   mjpeg_debug("   frames:      %8ld",lav_video_frames(el->lav_fd[n]));
   mjpeg_debug("   width:       %8d",lav_video_width (el->lav_fd[n]));
   mjpeg_debug("   height:      %8d",lav_video_height(el->lav_fd[n]));
   {
	   const char *int_msg;
	   switch(  lav_video_interlacing(el->lav_fd[n]))
	   {
	   case Y4M_ILACE_NONE :
		   int_msg = "not interlaced";
		   break;
	   case Y4M_ILACE_TOP_FIRST :
		   int_msg = "top field first";	 
		   break;
	   case Y4M_ILACE_BOTTOM_FIRST :
		   int_msg = "bottom field first";
		   break;
	   default:
		   int_msg = "Unknown!";
		   break;
	   }
	   mjpeg_debug("   interlacing: %s", int_msg );
   }
   
   mjpeg_debug("   frames/sec:  %8.3f",lav_frame_rate(el->lav_fd[n]));
   mjpeg_debug("   audio samps: %8ld",lav_audio_samples(el->lav_fd[n]));
   mjpeg_debug("   audio chans: %8d",lav_audio_channels(el->lav_fd[n]));
   mjpeg_debug("   audio bits:  %8d",lav_audio_bits(el->lav_fd[n]));
   mjpeg_debug("   audio rate:  %8ld",lav_audio_rate(el->lav_fd[n]));


   nerr = 0;

   if(n==0)
   {
      /* First file determines parameters */

      el->video_height = lav_video_height(el->lav_fd[n]);
      el->video_width  = lav_video_width (el->lav_fd[n]);
      el->video_inter  = lav_video_interlacing(el->lav_fd[n]);
      el->video_fps = lav_frame_rate(el->lav_fd[n]);
      lav_video_sampleaspect(el->lav_fd[n],
			     &el->video_sar_width,
			     &el->video_sar_height);
      if(!el->video_norm)
      {
		  /* TODO: This guessing here is a bit dubious but it can be over-ridden */
		 if(el->video_fps>24.95 && el->video_fps<25.05)
            el->video_norm = 'p';
         else if (el->video_fps>29.92 && el->video_fps<=30.02)
            el->video_norm = 'n';
         else
         {
			 mjpeg_error_exit1("File %s has %f frames/sec, choose norm with +[np] param",
							   filename,el->video_fps);
         }
      }
      el->audio_chans = lav_audio_channels(el->lav_fd[n]);
      if(el->audio_chans>2)
      {
		  mjpeg_error_exit1("File %s has %d audio channels - cant play that!",
                            filename,el->audio_chans);
      }
      el->has_audio = (el->audio_chans>0);
      el->audio_bits = lav_audio_bits(el->lav_fd[n]);
      el->audio_rate = lav_audio_rate(el->lav_fd[n]);
      el->audio_bps  = (el->audio_bits*el->audio_chans+7)/8;
   }
   else
   {
      /* All files after first have to match the paramters of the first */

      if( el->video_height != lav_video_height(el->lav_fd[n]) ||
          el->video_width  != lav_video_width (el->lav_fd[n]) )
      {
		 mjpeg_error("File %s: Geometry %dx%d does not match %ldx%ld",
					 filename,lav_video_width (el->lav_fd[n]),
					 lav_video_height(el->lav_fd[n]),el->video_width,el->video_height);
         nerr++;
      }
      if( el->video_inter != lav_video_interlacing(el->lav_fd[n]) )
      {
		  mjpeg_error("File %s: Interlacing is %d should be %ld",
					  filename,lav_video_interlacing(el->lav_fd[n]),el->video_inter);
		  nerr++;
      }
      if( fabs(el->video_fps - lav_frame_rate(el->lav_fd[n])) > 0.0000001)
      {
		  mjpeg_error("File %s: fps is %3.2f should be %3.2f",
					  filename,
					  lav_frame_rate(el->lav_fd[n]),
					  el->video_fps);
		  nerr++;
      }
      /* If first file has no audio, we don't care about audio */

      if(el->has_audio)
      {
         if(el->audio_chans != lav_audio_channels(el->lav_fd[n]) ||
            el->audio_bits  != lav_audio_bits(el->lav_fd[n]) ||
            el->audio_rate  != lav_audio_rate(el->lav_fd[n]) )
         {
           mjpeg_error("File %s: Audio is %d chans %d bit %ld Hz,"
					   " should be %d chans %d bit %ld Hz",
					   filename,lav_audio_channels(el->lav_fd[n]),
					   lav_audio_bits(el->lav_fd[n]), lav_audio_rate(el->lav_fd[n]),
					   el->audio_chans, el->audio_bits, el->audio_rate);
            nerr++;
         }
      }

      if(nerr) 
		  exit(1);
   }

   return n;
}

/*
   read_video_files:

   Accepts a list of filenames as input and opens these files
   for subsequent playback.

   The list of filenames may consist of:

   - "+p" or "+n" as the first entry (forcing PAL or NTSC)

   - ordinary  AVI or Quicktimefile names

   -  Edit list file names

   - lines starting with a colon (:) are ignored for third-party editlist extensions

*/

void read_video_files(char **filename, int num_files, EditList *el, 
					  int preserve_pathnames)
{
   FILE *fd;
   char line[1024];
   long index_list[MAX_EDIT_LIST_FILES];
   int i, n, nf, n1, n2, nl, num_list_files;

   nf = 0;

   memset(el,0,sizeof(EditList));

   el->chroma = Y4M_CHROMA_422; /* default, individual file will override */

   /* Check if a norm parameter is present */

   if(strcmp(filename[0],"+p")==0 || strcmp(filename[0],"+n")==0)
   {
      el->video_norm = filename[0][1];
      nf = 1;
      mjpeg_info("Norm set to %s",el->video_norm=='n'?"NTSC":"PAL");
   }

   for(;nf<num_files;nf++)
   {
      /* Check if filename[nf] is a edit list */

      fd = fopen(filename[nf],"r");

      if(fd==0)
      {
         mjpeg_error_exit1("Error opening %s: %s",filename[nf], strerror(errno));
      }

      fgets(line,1024,fd);
      if(strcmp(line,"LAV Edit List\n")==0)
      {
         /* Ok, it is a edit list */
		  mjpeg_debug( "Edit list %s opened",filename[nf]);

         /* Read second line: Video norm */

         fgets(line,1024,fd);
         if(line[0]!='N' && line[0]!='n' && line[0]!='P' && line[0]!='p')
         {
            mjpeg_error_exit1("Edit list second line is not NTSC/PAL");
         }

		 mjpeg_debug("Edit list norm is %s",line[0]=='N'||line[0]=='n'?"NTSC":"PAL");

         if(line[0]=='N'||line[0]=='n')
         {
            if( el->video_norm == 'p')
            {
               mjpeg_error_exit1("Norm allready set to PAL");
            }
            el->video_norm = 'n';
         }
         else
         {
            if( el->video_norm == 'n')
            {
               mjpeg_error_exit1("Norm allready set to NTSC");
            }
            el->video_norm = 'p';
         }

         /* read third line: Number of files */

         fgets(line,1024,fd);
         sscanf(line,"%d",&num_list_files);

		 mjpeg_debug("Edit list contains %d files",num_list_files);

         /* read files */

         for(i=0;i<num_list_files;i++)
         {
            fgets(line,1024,fd);
            n = strlen(line);
            if(line[n-1]!='\n')
            {
               mjpeg_error_exit1("Filename in edit list too long");
            }
            line[n-1] = 0; /* Get rid of \n at end */

            index_list[i] = open_video_file(line,el,preserve_pathnames);
         }

         /* Read edit list entries */

         while(fgets(line,1024,fd))
         {
            if(line[0]!=':') /* ignore lines starting with a : */
            {
               sscanf(line,"%d %d %d",&nl,&n1,&n2);
               if(nl<0 || nl>=num_list_files)
               {
                  mjpeg_error_exit1("Wrong file number in edit list entry");
               }
               if(n1<0) n1 = 0;
               if(n2>=el->num_frames[index_list[nl]]) n2 = el->num_frames[index_list[nl]];
               if(n2<n1) continue;

               el->frame_list = (long*) realloc(el->frame_list,
                                   (el->video_frames+n2-n1+1)*sizeof(long));
               if(el->frame_list==0) malloc_error();
               for(i=n1;i<=n2;i++)
                  el->frame_list[el->video_frames++] = EL_ENTRY(index_list[nl],i);
            }
         }

         fclose(fd);
      }
      else
      {
         /* Not an edit list - should be a ordinary video file */

         fclose(fd);

         n = open_video_file(filename[nf],el, preserve_pathnames);

         el->frame_list = (long*) realloc(el->frame_list,
                             (el->video_frames+el->num_frames[n])*sizeof(long));
         if(el->frame_list==0) malloc_error();
         for(i=0;i<el->num_frames[n];i++)
            el->frame_list[el->video_frames++] = EL_ENTRY(n,i);
      }
   }

   /* Calculate maximum frame size */

   for(i=0;i<el->video_frames;i++)
   {
      n = el->frame_list[i];
      if(lav_frame_size(el->lav_fd[N_EL_FILE(n)],N_EL_FRAME(n)) > el->max_frame_size)
         el->max_frame_size = lav_frame_size(el->lav_fd[N_EL_FILE(n)],N_EL_FRAME(n));
   }

   /* Help for audio positioning */

   el->last_afile = -1;
}

int write_edit_list(char *name, long n1, long n2, EditList *el)
{
   FILE *fd;
   int i, n, num_files, oldfile, oldframe;
   int index[MAX_EDIT_LIST_FILES];

   /* check n1 and n2 for correctness */

   if(n1<0) n1 = 0;
   if(n2>=el->video_frames) n2 = el->video_frames-1;
   mjpeg_info("Write edit list: %ld %ld %s",n1,n2,name);

   fd = fopen(name,"w");
   if(fd==0)
   {
      mjpeg_error("Can not open %s - no edit list written!",name);
      return -1;
   }
   fprintf(fd,"LAV Edit List\n");
   fprintf(fd,"%s\n",el->video_norm=='n'?"NTSC":"PAL");

   /* get which files are actually referenced in the edit list entries */

   for(i=0;i<MAX_EDIT_LIST_FILES;i++) index[i] = -1;

   for(i=n1;i<=n2;i++) index[N_EL_FILE(el->frame_list[i])] = 1;

   num_files = 0;
   for(i=0;i<MAX_EDIT_LIST_FILES;i++) if(index[i]==1) index[i] = num_files++;

   fprintf(fd,"%d\n",num_files);
   for(i=0;i<MAX_EDIT_LIST_FILES;i++)
      if(index[i]>=0) fprintf(fd,"%s\n",el->video_file_list[i]);

   oldfile  = index[N_EL_FILE(el->frame_list[n1])];
   oldframe = N_EL_FRAME(el->frame_list[n1]);
   fprintf(fd,"%d %d ",oldfile,oldframe);

   for(i=n1+1;i<=n2;i++)
   {
      n = el->frame_list[i];
      if(index[N_EL_FILE(n)]!=oldfile || N_EL_FRAME(n)!=oldframe+1)
      {
         fprintf(fd,"%d\n",oldframe);
         fprintf(fd,"%d %d ",index[N_EL_FILE(n)],N_EL_FRAME(n));
      }
      oldfile  = index[N_EL_FILE(n)];
      oldframe = N_EL_FRAME(n);
   }
   n = fprintf(fd,"%d\n",oldframe);

   /* We did not check if all our prints succeeded, so check at least the last one */

   if(n<=0)
   {
	   mjpeg_error("Error writing edit list: %s", strerror(errno));
	   return -1;
   }

   fclose(fd);

   return 0;
}

int el_get_video_frame(uint8_t *vbuff, long nframe, EditList *el)
{
   int res, n;

   if (nframe<0) nframe = 0;
   if (nframe>el->video_frames) nframe = el->video_frames;
   n = el->frame_list[nframe];

   res = lav_set_video_position(el->lav_fd[N_EL_FILE(n)],N_EL_FRAME(n));
   if (res<0)
      mjpeg_error_exit1("Error setting video position: %s",lav_strerror());
   res = lav_read_frame(el->lav_fd[N_EL_FILE(n)],vbuff);
   if (res<0)
      mjpeg_error_exit1("Error reading video frame: %s",lav_strerror());
   return res;
}

int el_get_audio_data(uint8_t *abuff, long nframe, EditList *el, int mute)
   {
   int res, n, ns0, ns1, asamps;

   if (!el->has_audio) return 0;

   if (nframe<0) nframe = 0;
   if (nframe>el->video_frames) nframe = el->video_frames;
   n = el->frame_list[nframe];

   ns1 = (double)(N_EL_FRAME(n)+1)*el->audio_rate/el->video_fps;
   ns0 = (double) N_EL_FRAME(n)   *el->audio_rate/el->video_fps;

   asamps = ns1-ns0;

   /* if mute flag is set, don't read actually, just return zero data */

   if (mute)
      {
	   /* TODO: A.Stevens 2000 - this looks like a potential overflow
		bug to me... non muted we only ever return asamps/FPS samples */
      memset(abuff,0,asamps*el->audio_bps);
      return asamps*el->audio_bps;
      }

   if(el->last_afile!=N_EL_FILE(n) || el->last_apos!=ns0)
      lav_set_audio_position(el->lav_fd[N_EL_FILE(n)],ns0);

   res = lav_read_audio(el->lav_fd[N_EL_FILE(n)],abuff,asamps);
   if (res<0)
      mjpeg_error_exit1("Error reading audio: %s",lav_strerror());

   if (res<asamps)
      memset(abuff+res*el->audio_bps,0,(asamps-res)*el->audio_bps);

   el->last_afile = N_EL_FILE(n);
   el->last_apos  = ns1;
   return asamps*el->audio_bps;
   }

int el_video_frame_data_format(long nframe, EditList *el)
{
   int n;

   if(el->video_frames<=0) return DATAFORMAT_MJPG; /* empty editlist, return default */
   if(nframe<0) nframe = 0;
   if(nframe>el->video_frames) nframe = el->video_frames;
   n = N_EL_FILE(el->frame_list[nframe]);
   return(el->lav_fd[n]->dataformat);
}
