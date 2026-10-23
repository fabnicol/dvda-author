/*
    lavaddwav: Add a WAV file as soundtrack to an AVI or QT file

    Usage: lavaddwav AVI_or_QT_file WAV_file Output_file

    Multiple output file version by Nicholas Redgrave 
    <baron@bologrew.demon.co.uk> 8th January 2005
    Use "%02d" style output filenames for multiple files.
    Parameter options added by Nicholas Redgrave 
    <baron@bologrew.demon.co.uk> 15th January 2005

    Major rewrite to add lists of AVI/QT and WAV files
    by Nicholas Redgrave <baron@bologrew.demon.co.uk> 2nd February 2005
    
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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "lav_io.h"
#include "mjpeg_logging.h"

#define FOURCC(a,b,c,d) ( (d<<24) | ((c&0xff)<<16) | ((b&0xff)<<8) | (a&0xff) )

#define FOURCC_RIFF     FOURCC ('R', 'I', 'F', 'F')
#define FOURCC_WAVE     FOURCC ('W', 'A', 'V', 'E')
#define FOURCC_FMT      FOURCC ('f', 'm', 't', ' ')
#define FOURCC_DATA     FOURCC ('d', 'a', 't', 'a')

#define MAX_FILE_NAME 127

/* Structures */

typedef struct
{
  char    chFileName[MAX_FILE_NAME+1];
  long    video_frames;
  int     width;
  int     height;
  int     interlacing;
  double  fps;
  double  time_length;
  int     format;
  long    max_frame_size;

}videoparams;

typedef struct
{
  char    chFileName[MAX_FILE_NAME+1];
  long    data_length;
  long    audio_samps;
  long    audio_rate;
  int     audio_chans;
  int     audio_bits;
  int     audio_bps;
  double  time_length;

}audioparams;


/* Prototypes */
static void usage(void);
void handle_args(int argc, char **argv);
int main(int argc, char **argv);
int open_lav_file(lav_file_t **lav_fd, char *chLavFile, int ientry, int iscan);
int open_wav_file(int *wav_fd, char *chWavFile, int ientry, int iscan);
int count_list_entries(char *chFile);
int fill_video_list_entries(char *chFile);
int fill_audio_list_entries(char *chFile);
void display_video_params(int ientry);
void display_audio_params(int ientry);
int allocate_video_memory(int ientries);
int allocate_audio_memory(int ientries);
void free_video_memory(int ientries);
void free_audio_memory(int ientries);
int compare_input_files(int iVidEntries, int iAudEntries);
long find_max_frame_size(int ientries);

/* Global variables */
int verbose = 1;
static int big_endian = 0;
static int param_maxfilesize = MAX_MBYTES_PER_FILE_32;
static int list_mode = 0;
static videoparams **vp = NULL;
static audioparams **ap = NULL;

static void usage(void) 
{
  fprintf(stderr,
    "Usage:  lavaddwav [params] <AVI_or_QT_file> <WAV_file> <Output_file>\n"
    "  where possible params are:\n"
    "    -m num      maximum size per file [%d MB]\n"
    "    -l          list mode (input files are text lists of AVI//QT and WAV files)\n",
    param_maxfilesize);
  
  fprintf(stderr,"    If the output file is too large for one AVI use %%0xd in\n");
  fprintf(stderr,"    the output filename so that lavaddwav creates several files.\n");
  fprintf(stderr,"    e.g. lavaddwav video.avi sound.wav output%%02d.avi\n");
}

void handle_args(int argc, char **argv)
{
  int p;

  /* disable error messages from getopt */
  opterr = 0;
   
  /* Parse options */
  while( (p = getopt(argc, argv, "lm:")) != -1 )
  {
    switch(p)
    {
      case 'l':
      {
        list_mode = 1;
        break;
      }
      case 'm':
      {
        param_maxfilesize = atoi(optarg);
        break;
      }
      default:
      {
        usage();
        exit(1);
      }
    }
  }

  /* check validity of options */
  if( (param_maxfilesize <= 0) || (param_maxfilesize > MAX_MBYTES_PER_FILE_32) )
  {
    param_maxfilesize = MAX_MBYTES_PER_FILE_32;
    fprintf(stderr,"Maximum size per file out of range - resetting to maximum\n");
  }
}

/*
 * Main
*/

int main(int argc, char **argv)
{
  int n;
  int wav_fd;
  long i, res, frame_size, max_frame_size;
  long absize, lRemBytes;
  uint8_t *vbuff = NULL;
  uint8_t *abuff = NULL;
  lav_file_t *lav_fd = NULL;
  lav_file_t *lav_out = NULL;
  unsigned long ulOutputBytes;
  int iLavFiles, iWavFiles;
  int iTotBytes;
  char *chOutFile;
  int iOutNum = 1;
  int iCurLav = 0;
  int iCurWav = 0;
  int imulti_out = 0;
  double total_video_duration = 0.0;
  double total_audio_duration = 0.0;

  if(argc < 4)
  {
    /* Show usage */
    usage();
    exit(1);
  }

  /* Handle command line arguments */
  handle_args(argc, argv);

  /* Are we big or little endian? */
  big_endian = lav_detect_endian();
  if( big_endian < 0 )
    exit(1);

  chOutFile = malloc(strlen(argv[optind+2]) + 8 + 1);  /* Extra for safety */
  if (chOutFile == NULL)
     mjpeg_error_exit1("Could not malloc space for output filename");

  /* Process according to mode */
  if( list_mode )
  {
    /* Open and count file entries in lav list file */
    iLavFiles = count_list_entries(argv[optind]);
    if( !iLavFiles )
    {
      mjpeg_error("No entries found in video list file %s", argv[optind]);
      exit(1);
    }
    /* Open and count file entries in wav list file */
    iWavFiles = count_list_entries(argv[optind+1]);
    if( !iWavFiles )
    {
      mjpeg_error("No entries found in audio list file %s", argv[optind+1]);
      exit(1);
    }
  }
  else
  {
    iLavFiles = 1;
    iWavFiles = 1;
  }

  /* Allocate memory for list entry structures */
  if( !allocate_video_memory(iLavFiles) )
  {
    mjpeg_error("Unable to allocate memory for video file list structures");
    exit(1);
  }

  if( !allocate_audio_memory(iWavFiles) )
  {
    mjpeg_error("Unable to allocate memory for audio file list structures");
    free_video_memory(iLavFiles);
    exit(1);
  }



  /* Fill structures with data */
  if( list_mode )
  {
    if( !fill_video_list_entries(argv[optind]) )
    {
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      mjpeg_error("Unable to fill video file list structures");
      exit(1);
    }
    if( !fill_audio_list_entries(argv[optind+1]) )
    {
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      mjpeg_error("Unable to fill audio file list structures");
      exit(1);
    }
  }
  else
  {
    if( !open_lav_file(&lav_fd, argv[optind], 0, 1) )
    {
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      mjpeg_error("Unable to fill video file structure");
      exit(1);
    }
    lav_close(lav_fd);
    display_video_params(0);

    if( !open_wav_file(&wav_fd, argv[optind+1], 0, 1) )
    {
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      mjpeg_error("Unable to fill audio file structure");
      exit(1);
    }
    close(wav_fd);
    display_audio_params(0);
  }


  /* Check that input files are compatible */
  if( list_mode )
  {
    if( !compare_input_files(iLavFiles, iWavFiles) )
    {
      mjpeg_error("Input files are not compatible");
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      exit(1);
    }
  }

  /* Find largest frame size */
  max_frame_size = find_max_frame_size(iLavFiles);
  mjpeg_debug("   Max frame size: %ld", max_frame_size);

  /* Rough check for "%d" style output filename */
  if( strchr(argv[optind+2], 37) != NULL )
    imulti_out = 1;
  else
    /* Copy output filename for non-%d style names */
    strcpy(chOutFile, argv[optind+2]);

  /* Allocate video and audio buffers */
  vbuff = (uint8_t*) malloc(max_frame_size);
  absize = (ap[0]->audio_rate / vp[0]->fps) + 0.5;
  absize *= ap[0]->audio_bps;
  abuff = (uint8_t*) malloc(absize);

  if( (vbuff == NULL) || (abuff == NULL) )
  {
    mjpeg_error("Out of Memory - malloc failed");
    if( vbuff != NULL ){ free(vbuff); }
    if( abuff != NULL ){ free(abuff); }
    free_audio_memory(iWavFiles);
    free_video_memory(iLavFiles);
    exit(1);
  }


  /* Open first video and audio file */
  if( !open_lav_file(&lav_fd, vp[iCurLav]->chFileName, iCurLav, 0) )
  {
    mjpeg_error("Error opening %s", vp[iCurLav]->chFileName);
    free(abuff);
    free(vbuff);
    free_audio_memory(iWavFiles);
    free_video_memory(iLavFiles);
    exit(1);
  }
  total_video_duration += vp[iCurLav]->time_length;

  if( !open_wav_file(&wav_fd, ap[iCurWav]->chFileName, iCurWav, 0) )
  {
    mjpeg_error("Error opening %s", ap[iCurWav]->chFileName);
    free(abuff);
    free(vbuff);
    free_audio_memory(iWavFiles);
    free_video_memory(iLavFiles);
    exit(1);
  }
  total_audio_duration += ap[iCurWav]->time_length;

  /* (First) video and audio file are open - we are ready to multiplex */
  ulOutputBytes = 0;
  i = 0;

   
  /* Outer loop */
  while( iCurLav < iLavFiles )
  {
    if( imulti_out )
      /* build output filename */
      sprintf(chOutFile, argv[optind+2], iOutNum++);

    /* Create output file */
    lav_out = lav_open_output_file(chOutFile,
                                   vp[0]->format,
                                   vp[0]->width,
                                   vp[0]->height,
                                   vp[0]->interlacing,
                                   vp[0]->fps,
                                   ap[0]->audio_bits,
                                   ap[0]->audio_chans,
                                   ap[0]->audio_rate);
    if(!lav_out)
    {
      mjpeg_error("Error creating %s: %s", chOutFile, lav_strerror());
      lav_close(lav_fd);
      if( wav_fd >= 0 ) { close(wav_fd); }
      free(abuff);
      free(vbuff);
      free_audio_memory(iWavFiles);
      free_video_memory(iLavFiles);
      free(chOutFile);
      exit(1);
    }

    /* Loop reading and writing video and audio */
    while(1)
    {

      /* Video loop */
      while( i < vp[iCurLav]->video_frames )
      {
        /* read video frame */
        res = lav_read_frame(lav_fd,vbuff);
        if( res < 0 )
        {
          mjpeg_error("File: %s Reading video frame: %s",vp[iCurLav]->chFileName,lav_strerror());
          lav_close(lav_out);
          lav_close(lav_fd);
          if( wav_fd >= 0 ) { close(wav_fd); }
          free(abuff);
          free(vbuff);
          free_audio_memory(iWavFiles);
          free_video_memory(iLavFiles);
          exit(1);
        }
        frame_size = lav_frame_size(lav_fd,i);

        /* write video frame */
        res = lav_write_frame(lav_out, vbuff, frame_size, 1);
        if( res < 0 )
        {
          mjpeg_error("Writing video frame: %s", lav_strerror());
          lav_close(lav_out);
          lav_close(lav_fd);
          if( wav_fd >= 0 ) { close(wav_fd); }
          free(abuff);
          free(vbuff);
          free_audio_memory(iWavFiles);
          free_video_memory(iLavFiles);
          exit(1);
        }
        ulOutputBytes += (unsigned long)frame_size;

        /* Audio loop */
        lRemBytes = absize;
        iTotBytes = 0;

        while( iCurWav < iWavFiles )
        {
          n = read(wav_fd, &abuff[iTotBytes], lRemBytes);
          iTotBytes += n;
          lRemBytes -= n;

          if( n < 0 )
          {
            /* read error */
            mjpeg_error("Error reading audio from file %s: %s", ap[iCurWav]->chFileName, lav_strerror());
            lav_close(lav_out);
            lav_close(lav_fd);
            close(wav_fd);
            free(abuff);
            free(vbuff);
            free_audio_memory(iWavFiles);
            free_video_memory(iLavFiles);
            exit(1);
          }

          if( iTotBytes < absize )
          {
            /* we need more data from next audio file if possible */
            close(wav_fd);
            wav_fd = -1;
            iCurWav++;
            if( iCurWav == iWavFiles )
            {
              /* no more audio files - make next if block write out data now */
              absize = iTotBytes;
            }
          }

          if( iTotBytes == absize )
          {
            /* got all the bytes we wanted - write them out */
            res = lav_write_audio(lav_out, abuff, iTotBytes/ap[0]->audio_bps);
            if( res < 0 )
            {
              mjpeg_error("Error writing audio: %s",lav_strerror());
              lav_close(lav_out);
              lav_close(lav_fd);
              close(wav_fd);
              free(abuff);
              free(vbuff);
              free_audio_memory(iWavFiles);
              free_video_memory(iLavFiles);
              exit(1);
            }
            ulOutputBytes += (unsigned long)iTotBytes;

            /* we're done for this frame */
            break;

          } /* end of if( iTotBytes == absize ) */


          /* we need more data from next audio file */
          if( !open_wav_file(&wav_fd, ap[iCurWav]->chFileName, iCurWav, 0) )
          {
            mjpeg_error("Error opening %s", ap[iCurWav]->chFileName);
            lav_close(lav_out);
            lav_close(lav_fd);
            free(abuff);
            free(vbuff);
            free_audio_memory(iWavFiles);
            free_video_memory(iLavFiles);
            exit(1);
          }
          total_audio_duration += ap[iCurWav]->time_length;

        } /* end of while( iCurWav < iWavFiles ) */

        /* Increment video frame counter */
         i++;

        /* Check for exceeding maximum output file size */
        if( (ulOutputBytes >> 20) >= (unsigned long)param_maxfilesize )
        {
          if( imulti_out )
          {
            mjpeg_debug("  Starting new sequence: %d",iOutNum);
            lav_close(lav_out);
            ulOutputBytes = 0;
            break;
          }
          /* No limit on QT files */
          if( vp[0]->format != 113 )
          {
            mjpeg_error("Max file size reached use %%0xd in your output filename");
            lav_close(lav_out);
            lav_close(lav_fd);
            if( wav_fd >= 0 ) { close(wav_fd); }
            free(abuff);
            free(vbuff);
            free_audio_memory(iWavFiles);
            free_video_memory(iLavFiles);
            exit(0);
          }
        }

      } /* end of while( i < vp[iCurLav]->video_frames ) */


      /* Do we need to go on to the next video file? */
      if( i == vp[iCurLav]->video_frames )
      {
        iCurLav++;
        if( iCurLav < iLavFiles )
        {
          /* Open next video file */
          lav_close(lav_fd);
          if( !open_lav_file(&lav_fd, vp[iCurLav]->chFileName, iCurLav, 0) )
          {
            mjpeg_error("Error opening %s", vp[iCurLav]->chFileName);
            if( lav_out != NULL ) { lav_close(lav_out); }
            if( wav_fd >= 0 ) { close(wav_fd); }
            free(abuff);
            free(vbuff);
            free_audio_memory(iWavFiles);
            free_video_memory(iLavFiles);
            exit(1);
          }
          total_video_duration += vp[iCurLav]->time_length;
          /* reset video frame counter */
          i = 0;
        }
        else
        {
          /* we've run out of video */
          break;
        }
      } /* end of if( i == vp[iCurLav]->video_frames ) */



      /* Do we need to create a new output file? */
      if( ulOutputBytes == 0 )
      {
        break;
      }

    }/* end of while(1) (Loop reading and writing video and audio) */
      
      
  } /* end of while( iCurLav < iLavFiles ) */


  /* Copy remaining audio if required */

  /* Outer loop */
  while( iCurWav < iWavFiles )
  {
    if( ulOutputBytes == 0 )
    {
      /* build output filename */
      sprintf(chOutFile, argv[optind+2], iOutNum++);
      
      /* Create output file */
      lav_out = lav_open_output_file(chOutFile,
                                     vp[0]->format,
                                     vp[0]->width,
                                     vp[0]->height,
                                     vp[0]->interlacing,
                                     vp[0]->fps,
                                     ap[0]->audio_bits,
                                     ap[0]->audio_chans,
                                     ap[0]->audio_rate);
      if(!lav_out)
      {
        mjpeg_error("Error creating %s: %s", chOutFile, lav_strerror());
        if( wav_fd >= 0 ) { close(wav_fd); }
        free(abuff);
        free(vbuff);
        free_audio_memory(iWavFiles);
        free_video_memory(iLavFiles);
        free(chOutFile);
        exit(1);
      }
    } /* end of if( lav_out == NULL ) */


    /* Audio loop */
    while(1)
    {
      lRemBytes = absize;
      iTotBytes = 0;

      while( iCurWav < iWavFiles )
      {
        n = read(wav_fd, &abuff[iTotBytes], lRemBytes);
        iTotBytes += n;
        lRemBytes -= n;

        if( n < 0 )
        {
          /* read error */
          mjpeg_error("Error reading audio from file %s: %s", ap[iCurWav]->chFileName, lav_strerror());
          lav_close(lav_out);
          close(wav_fd);
          free(abuff);
          free(vbuff);
          free_audio_memory(iWavFiles);
          free_video_memory(iLavFiles);
          exit(1);
        }

        if( iTotBytes < absize )
        {
          /* we need more data from next audio file if possible */
          close(wav_fd);
          wav_fd = -1;
          iCurWav++;
          if( iCurWav == iWavFiles )
          {
            /* no more audio files - make next if block write out data now */
            absize = iTotBytes;
          }
        }

        if( iTotBytes == absize )
        {
          /* got all the bytes we wanted - write them out */
          res = lav_write_audio(lav_out, abuff, iTotBytes/ap[0]->audio_bps);
          if( res < 0 )
          {
            mjpeg_error("Error writing audio: %s",lav_strerror());
            lav_close(lav_out);
            close(wav_fd);
            free(abuff);
            free(vbuff);
            free_audio_memory(iWavFiles);
            free_video_memory(iLavFiles);
            exit(1);
          }
          ulOutputBytes += (unsigned long)iTotBytes;

          /* we're done for this frame */
          break;

        } /* end of if( iTotBytes == absize ) */


        /* we need more data from next audio file */
        if( !open_wav_file(&wav_fd, ap[iCurWav]->chFileName, iCurWav, 0) )
        {
          mjpeg_error("Error opening %s", ap[iCurWav]->chFileName);
          lav_close(lav_out);
          free(abuff);
          free(vbuff);
          free_audio_memory(iWavFiles);
          free_video_memory(iLavFiles);
          exit(1);
        }
        total_audio_duration += ap[iCurWav]->time_length;

      } /* end of while( iCurWav < iWavFiles ) */


      /* Check for exceeding maximum output file size */
      if( (ulOutputBytes >> 20) >= (unsigned long)param_maxfilesize )
      {
        if( imulti_out )
        {
          mjpeg_debug("  Starting new sequence: %d",iOutNum);
          ulOutputBytes = 0;
          break;
        }
        /* No limit on QT files */
        if( vp[0]->format != 113 )
        {
          mjpeg_error("Max file size reached use %%0xd in your output filename");
          lav_close(lav_out);
          if( wav_fd >= 0 ) { close(wav_fd); }
          free(abuff);
          free(vbuff);
          free_audio_memory(iWavFiles);
          free_video_memory(iLavFiles);
          exit(0);
        }
      }

      if( iCurWav == iWavFiles )
      {
        /* no more audio files */
        break;
      }

    } /* end of while(1) (Audio loop) */

    lav_close(lav_out);
    lav_out = NULL;

  } /* end of while( iCurWav < iWavFiles ) */


  /* Tidy up and exit */
  if( lav_out != NULL )
  {
    lav_close(lav_out);
  }
  free(abuff);
  free(vbuff);
  free_audio_memory(iWavFiles);
  free_video_memory(iLavFiles);
  free(chOutFile);

  mjpeg_debug("   Total Video Duration:    %8.3f sec", total_video_duration);
  mjpeg_debug("   Total Audio Duration:    %8.3f sec", total_audio_duration);
  mjpeg_debug(" ");

  /* Success */
  return 0;
}

int open_lav_file(lav_file_t **lav_fd, char *chLavFile, int ientry, int iscan)
{
  long i;
  long max_frame_size = 0;
  lav_file_t *lav_tmp;

  *lav_fd = lav_open_input_file(chLavFile);
  if(!*lav_fd)
  {
    mjpeg_error("Error opening %s", chLavFile);
    return 0;
  }

  if( iscan )
  {
    strncpy(&vp[ientry]->chFileName[0], chLavFile, MAX_FILE_NAME);
    vp[ientry]->video_frames = lav_video_frames(*lav_fd);
    vp[ientry]->width        = lav_video_width(*lav_fd);
    vp[ientry]->height       = lav_video_height(*lav_fd);
    vp[ientry]->interlacing  = lav_video_interlacing(*lav_fd);
    vp[ientry]->fps          = lav_frame_rate(*lav_fd);

    if( vp[ientry]->fps <= 0.0 )
    {
      lav_close(*lav_fd);
      mjpeg_error("Framerate illegal");
      return 0;
    }
    
    vp[ientry]->time_length  = vp[ientry]->video_frames / vp[ientry]->fps;

    lav_tmp = *lav_fd;
    vp[ientry]->format       = lav_tmp->format;

    /* find maximum frame size */
    for(i=0; i<vp[ientry]->video_frames; i++)
    {
      if( lav_frame_size(*lav_fd, i) > max_frame_size )
      {
        max_frame_size = lav_frame_size(*lav_fd, i);
      }
    }
    vp[ientry]->max_frame_size = max_frame_size;
  }

  /* success */
  return 1;  
}

int open_wav_file(int *wav_fd, char *chWavFile, int ientry, int iscan)
{
  int n;
  uint32_t fmtlen;
  uint32_t data[64];
  off_t cur_off = 0;
  off_t cur_end = 0;

  *wav_fd = open(chWavFile, O_RDONLY);
  if( *wav_fd < 0 )
  {
    mjpeg_error("Error opening WAV file %s :%s", chWavFile, strerror(errno));
    return 0;
  }

  n = read(*wav_fd,(char*)data,20);
  if( n != 20 )
  {
    mjpeg_error("Error reading WAV file %s :%s", chWavFile, strerror(errno));
    close(*wav_fd);
    return 0;
  }

  /* Make endian safe */
  data[0] = reorder_32(data[0], big_endian);
  data[2] = reorder_32(data[2], big_endian);
  data[3] = reorder_32(data[3], big_endian);
  data[4] = reorder_32(data[4], big_endian);


  if( data[0] != FOURCC_RIFF ||
      data[2] != FOURCC_WAVE ||
      data[3] != FOURCC_FMT  ||
      data[4] > sizeof(data) )
  {
    mjpeg_error("Error in WAV header of %s", chWavFile);
    close(*wav_fd);
    return 0;
  }

  fmtlen = data[4];

  n = read(*wav_fd, (char*)data, fmtlen);
  if( n !=fmtlen )
  {
    perror("open_wav_file");
    close(*wav_fd);
    return 0;
  }

  /* Make endian safe */
  data[0] = reorder_32(data[0], big_endian);
  data[1] = reorder_32(data[1], big_endian);
  data[3] = reorder_32(data[3], big_endian);

  if( (data[0]&0xffff) != 1 )
  {
    mjpeg_error("WAV file is not in PCM format %s", chWavFile);
    close(*wav_fd);
    return 0;
  }

  if( iscan )
  {
    strncpy(ap[ientry]->chFileName, chWavFile, MAX_FILE_NAME);
    ap[ientry]->audio_chans  = (data[0]>>16) & 0xffff;
    ap[ientry]->audio_rate   = data[1];
    ap[ientry]->audio_bits   = (data[3]>>16) & 0xffff;
    ap[ientry]->audio_bps    = (ap[ientry]->audio_chans * ap[ientry]->audio_bits + 7) / 8;

    if( ap[ientry]->audio_bps == 0 ) { ap[ientry]->audio_bps = 1; } /* safety first */
  }

  n = read(*wav_fd,(char*)data,8);
  if( n != 8 )
  {
    mjpeg_error("Read WAV header %s :%s", chWavFile, strerror(errno));
    close(*wav_fd);
    return 0;
  }

  /* Make endian safe */
  data[0] = reorder_32(data[0], big_endian);
  data[1] = reorder_32(data[1], big_endian);

  if( data[0] != FOURCC_DATA )
  {
    mjpeg_error("Error in WAV header %s", chWavFile);
    close(*wav_fd);
    return 0;
  }

  if( iscan )
  {
    /* check length of Wav file - update structure with correct length */
    cur_off = lseek(*wav_fd, 0, SEEK_CUR);
    cur_end = lseek(*wav_fd, 0, SEEK_END);
    if( (long)(cur_end - cur_off) != data[1] )
    {
      mjpeg_warn("Repairing data length mismatch with WAV header %s", chWavFile);
      data[1] = (long)(cur_end - cur_off);
    }
    cur_off = lseek(*wav_fd, cur_off, SEEK_SET);

    ap[ientry]->data_length = data[1];
    ap[ientry]->audio_samps = data[1] / ap[ientry]->audio_bps;
    ap[ientry]->time_length = (double)ap[ientry]->audio_samps / (double)ap[ientry]->audio_rate;
  }

  /* success */
  return 1;
}

int count_list_entries(char *chFile)
{
  FILE *fp;
  int ientries = 0;
  char chformat[10];
  char chtmp[MAX_FILE_NAME+1] = "\0";

  sprintf(chformat, "%%%ds",MAX_FILE_NAME);

  fp = fopen(chFile, "rt");
  if( fp == NULL )
  {
    mjpeg_error("Unable to open file list %s", chFile);
    return 0;
  }

  while (fscanf(fp, chformat, chtmp) == 1)
        ientries++;

  fclose(fp);

  /* return the number of entries */
  return ientries;
}

int fill_video_list_entries(char *chFile)
{
  FILE *fp;
  int ientry = 0;
  char chformat[10];
  char chtmp[MAX_FILE_NAME+1] = "\0";
  lav_file_t *lav_tmp = NULL;

  sprintf(chformat, "%%%ds",MAX_FILE_NAME);

  fp = fopen(chFile, "rt");
  if( fp == NULL )
  {
    mjpeg_error("Unable to open video file list %s", chFile);
    return 0;
  }

  while( fscanf(fp, chformat, chtmp) == 1)
  {
    if( !open_lav_file(&lav_tmp, chtmp, ientry, 1)  )
    {
      fclose(fp);
      return 0;
    }
    display_video_params(ientry);
    ientry++;
    lav_close(lav_tmp);
  }

  fclose(fp);
  return 1;
}

int fill_audio_list_entries(char *chFile)
{
  FILE *fp;
  int ientry = 0;
  char chformat[10];
  char chtmp[MAX_FILE_NAME+1] = "\0";
  int wav_tmp;

  sprintf(chformat, "%%%ds",MAX_FILE_NAME);

  fp = fopen(chFile, "rt");
  if( fp == NULL )
  {
    mjpeg_error("Unable to open audio file list %s", chFile);
    return 0;
  }

  while( fscanf(fp, chformat, chtmp) == 1)
  {
    if( !open_wav_file(&wav_tmp, chtmp, ientry, 1)  )
    {
      fclose(fp);
      return 0;
    }
    display_audio_params(ientry);
    ientry++;
    close(wav_tmp);
  }

  fclose(fp);
  return 1;
}

void display_video_params(int ientry)
{

   mjpeg_debug("File: %s",                  vp[ientry]->chFileName);
   mjpeg_debug("   format:      %8c",       vp[ientry]->format);
   mjpeg_debug("   frames:      %8ld",      vp[ientry]->video_frames);
   mjpeg_debug("   width:       %8d",       vp[ientry]->width);
   mjpeg_debug("   height:      %8d",       vp[ientry]->height);
   mjpeg_debug("   interlacing: %8d",       vp[ientry]->interlacing);
   mjpeg_debug("   frames/sec:  %8.3f",     vp[ientry]->fps);
   mjpeg_debug("   duration:    %8.3f sec", vp[ientry]->time_length);
   mjpeg_debug(" ");

}

void display_audio_params(int ientry)
{
   mjpeg_debug("File: %s",                  ap[ientry]->chFileName);
   mjpeg_debug("   audio samps: %8ld",      ap[ientry]->audio_samps);
   mjpeg_debug("   audio chans: %8d",       ap[ientry]->audio_chans);
   mjpeg_debug("   audio bits:  %8d",       ap[ientry]->audio_bits);
   mjpeg_debug("   audio rate:  %8ld",      ap[ientry]->audio_rate);
   mjpeg_debug("   duration:    %8.3f sec",(double)ap[ientry]->audio_samps/(double)ap[ientry]->audio_rate);
   mjpeg_debug(" ");

}

int allocate_video_memory(int ientries)
{
  int i;

  /* allocate memory for array of pointers */
  vp = malloc(ientries * sizeof(videoparams *));
  if( vp == NULL )
  {
    mjpeg_error("Unable to allocate memory for video file list structure array");
    return 0;
  }
  memset(vp, 0 , ientries * sizeof(videoparams*));

  /* allocate memory for each structure */
  for(i=0; i<ientries; i++)
  {
    vp[i] = malloc(sizeof(videoparams));
    if( vp[i] == NULL )
    {
      mjpeg_error("Unable to allocate memory for video file list structures");
      return 0;
    }
    memset(vp[i], 0 , sizeof(videoparams));
  }

  return 1;
}

int allocate_audio_memory(int ientries)
{
  int i;

  /* allocate memory for array of pointers */
  ap = malloc(ientries * sizeof(audioparams *));
  if( ap == NULL )
  {
    mjpeg_error("Unable to allocate memory for audio file list structure array");
    return 0;
  }
  memset(ap, 0 , ientries * sizeof(audioparams*));


  /* allocate memory for each structure */
  for(i=0; i<ientries; i++)
  {
    ap[i] = malloc(sizeof(audioparams));
    if( ap[i] == NULL )
    {
      mjpeg_error("Unable to allocate memory for audio file list structures");
      return 0;
    }
    memset(ap[i], 0 , sizeof(audioparams));
  }

  return 1;
}

void free_video_memory(int ientries)
{
  int i;

  if( vp == NULL )
  {
    return;
  }

  for(i=0; i<ientries; i++)
  {
    if( vp[i] != NULL )
    {
      free(vp[i]);
    }
  }

  free(vp);
  vp = NULL;
}

void free_audio_memory(int ientries)
{
  int i;

  if (ap == NULL)
     return;

  for(i=0; i<ientries; i++)
  {
    if (ap[i] != NULL)
       free(ap[i]);
  }

  free(ap);
  ap = NULL;
}

int compare_input_files(int iVidEntries, int iAudEntries)
{
  int i;

  /* compare video files */
  for(i=0; i<iVidEntries; i++)
  {
    if( (vp[0]->format      != vp[i]->format)      ||
        (vp[0]->width       != vp[i]->width)       ||
        (vp[0]->height      != vp[i]->height)      ||
        (vp[0]->interlacing != vp[i]->interlacing) ||
        (vp[0]->fps         != vp[i]->fps) )
    {
      /* mismatch */
      return 0;
    }
  }

  /* compare audio files */
  for(i=0; i<iAudEntries; i++)
  {
    if( (ap[0]->audio_chans != ap[i]->audio_chans) ||
        (ap[0]->audio_bits  != ap[i]->audio_bits)  ||
        (ap[0]->audio_rate  != ap[i]->audio_rate) )
    {
      /* mismatch */
      return 0;
    }
  }

  /* file formats match */
  return 1;
}

long find_max_frame_size(int ientries)
{
  int i;
  long max_frame_size = vp[0]->max_frame_size;

  for(i=0; i<ientries; i++)
  {
    if( max_frame_size < vp[i]->max_frame_size )
    {
      max_frame_size = vp[i]->max_frame_size;
    }
  }

  return max_frame_size;
}
