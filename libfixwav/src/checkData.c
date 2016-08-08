/* ==========================================================================
*
*   checkData.c
*   Original author Pigiron,2007. Revised and expanded by Fabrice Nicol,copyright
*   2008, 2009.
*
*   Description
*       Checks evenness of data byte count and pads with 0s otherwise
*       Copies data to new file (if option -o is selected)
========================================================================== */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include "libiberty.h"
// Requested by truncate()
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __WIN32__
#include <unistd.h>
#endif
#include <sys/types.h>
#include <inttypes.h>

#include "fixwav.h"
#include "fixwav_auxiliary.h"
#include "checkData.h"
#include "fixwav_manager.h"
#include "c_utils.h"
#include "repair.h"
#include "winport.h"
#include "structures.h"


extern globalData globals;


/*********************************************************************
 Function: check_sample_count

 Purpose:  checks whether the number of samples is a whole number

*********************************************************************/

int pad_end_of_file(WaveData* info)
{
  FILE* outfile=info->OUTFILE;
  uint32_t complement=info->padbytes;
  char buf[complement];
  memset(buf, 0, complement);

  printf( ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Writing %d pad bytes...\n", complement);
  end_seek(outfile);
  uint32_t count = fwrite( &buf, 1 , complement , outfile );

  if   (count  != complement)
    {
      printf( "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET" Error appending data to end of existing file\n" );
      printf( ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  %d characters were written out of %d\n", count, complement);
      printf( "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Error appending data to end of existing file\n" );

      if (isok()) return(FAIL);
    }
  return(BAD_DATA);
}




int check_sample_count(WaveData *info, WaveHeader *header)
{


  int r=0;

  if ((r=header->data_size % header->byte_p_spl) 	== 0)
    return(GOOD_HEADER);

   info->padbytes+=header->byte_p_spl - r;
   header->data_size+=info->padbytes;
   header->chunk_size+=info->padbytes;

  return(BAD_DATA);
}

/*********************************************************************
 Function: readjust_sizes

 Purpose:  This function updates the header 'size' fields with
           updated information.
*********************************************************************/

_Bool check_real_size(WaveData *info, WaveHeader *header)
{
  uint64_t size=0;
  char* filepath=NULL;
  FILE* file=NULL;
  FILE* infile=info->INFILE;
  FILE* outfile=info->OUTFILE; 

  /* get the new file statistics, depending on whether there were changes or not */
  /* stat needs a newly opened file to tech stats */


  if (! info->in_place &&  info->repair != GOOD_HEADER && ! info->virtual)
  {
      filepath=info->outfile;
      file = outfile;
  }
  else
  {
      filepath=info->infile;
      file = infile;
  }

  if (file)
  {
//      if (fclose(file) == EOF)
//            {

//              fprintf(stderr, "%s\n", ""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  fclose error: issues may arise.");
//              return(false);
//            }
  }

  secure_open(filepath, "rb", file);
  size=read_file_size(file, filepath);

  /* adjust the Chunk Size */
  if (header->chunk_size == (uint32_t) size - 8)
  {
    if (globals.debugging)
      printf("%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Verifying real chunk size on disc... OK");
  }
  else
  {
      if (globals.debugging) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Verifying real chunk size on disc... fixed:\n       expected size: %u, real size: %lu\n", header->chunk_size+8, size );
      header->chunk_size = (uint32_t) size - 8 ; // if prepending, chunk_size was computed as the full size of raw file -8 bytes to which one must add the size of new header
  }

  if (header->data_size == (uint32_t) size - header->header_size_out)
  {
      if (globals.debugging)
        printf("%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Verifying real data size on disc... OK");
  }
  else
  {
      if (globals.debugging) printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Verifying real data size on disc... fixed:\n       header size: %d, expected size: %u, real size: %lu\n", header->header_size_out, header->data_size+header->header_size_out, size );
      header->data_size = (uint32_t) size - header->header_size_out ;  // if prepending, data_size was computed as the full size of raw file hence this new size minus HEADER_SIZE
  }


   return(false);

}


/****************************************************************************
 Function: check_envenness

 Purpose:  This function adds one padding byte to the end of the data chunk
		   should the byte count be odd.
***************************************************************************/


/* all RIFF chunks (including WAVE "data" chunks) must be word aligned.
* If the sample data uses an odd number of bytes, a padding byte with a value of zero
* must be placed at the end of the sample data. The "data" chunk header's size should not include this byte.*/


int check_evenness(WaveData *info, WaveHeader *header)
{

  if (header->data_size %2)
    {
      printf("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Readjusting output file to even byte count...");

      header->data_size++;
      header->chunk_size++;
      info->padbytes+=1;

      return(BAD_DATA);
    }

  return GOOD_HEADER;

}

int prune(FILE* infile, WaveData *info, WaveHeader *header)
{

  uint8_t p=0;
  uint32_t count=-1;
  uint64_t size=0;

  errno=0;
  size=read_file_size(infile, info->infile);

  if (errno)
     perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not state file size\n");

// Count ending zeros to be pruned
  if (end_seek(infile) == FAIL) return(FAIL);
  do
    {
      if (fseek(infile, -1, SEEK_CUR) == -1) return(FAIL);
      if (fread(&p, 1, 1, infile) != 1) return(FAIL);
#ifndef __WIN32__
      if (globals.debugging) printf("  Offset %"PRIu64"  : %"PRIu8" \n", (uint64_t) ftello(infile), p );
#endif
      if (fseek(infile, -1, SEEK_CUR) == -1) return(FAIL);
      count++;

    }
  while (p == 0);

  if (count > 0)
    {

      printf(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Pruning file: -%"PRIu32" bytes at: %"PRIu32"\n", count, header->chunk_size + 8 -count );
      uint64_t offset;

//  Under Windows API, pruning from end of file
//  otherwise truncate takes full size as an argument

#ifdef __WIN32__
      if (info->in_place) fclose(infile);
      offset=-count;
#else
      offset=size -count;

#endif
      // Truncating only if changes made in place, otherwise truncations results from incomplete copying at Checkout stage

      if ((info->in_place) && (truncate_from_end(info->infile, offset) == -1))
        {
          perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  truncate error\n");
          return(info->repair=FAIL);
        }

#ifdef __WIN32__
      if (info->in_place) outfile=fopen(info->infile, "rb+");
#endif

      printf("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Readjusting byte count...");
      header->chunk_size-=count;
      header->data_size-=count;
      info->prunedbytes=count;

      return(info->repair=BAD_DATA);
    }

  return(NO_PRUNE);
}





