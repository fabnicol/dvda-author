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

  uint32_t complement=info->padbytes;
  char buf[complement];
  memset(buf, 0, complement);

  if (globals.debugging) foutput( INF "Writing %d pad bytes...\n", complement);
  end_seek(fileptr(info->outfile));
  uint32_t count = fwrite( &buf, 1 , complement , fileptr(info->outfile));

  if   (count  != complement)
    {
      if (globals.debugging) foutput( "%s\n", ERR "Error appending data to end of existing file\n" );
      if (globals.debugging) foutput( INF "%d characters were written out of %d\n", count, complement);
      if (globals.debugging) foutput( "%s\n", ERR "Error appending data to end of existing file\n" );

      if (isok()) return(FAIL);
    }
  return(BAD_DATA);
}

int check_sample_count(WaveData *info, WaveHeader *header)
{
  int r=0;

  if ((r=header->data_cksize % header->nBlockAlign) 	== 0)
    return(GOOD_HEADER);

   info->padbytes+=header->nBlockAlign - r;
   header->data_cksize+=info->padbytes;
   header->ckSize+=info->padbytes;

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

  filestat_t file = (info->in_place ||  info->repair == GOOD_HEADER || info->virtual)? info->infile : info->outfile;

  /* get the new file statistics, depending on whether there were changes or not */
  /* stat needs a newly opened file to tech stats */


  s_open(file, "rb+");
  _Bool pad_byte = (header->ckSize % 2 == 1);

  /* adjust the Chunk Size */
  if (header->ckSize == (uint32_t) filesize(file) - 8 - (int) pad_byte)
  {
      if (globals.debugging) foutput("%s\n", MSG "Verifying real chunk size on disc... OK");
  }
  else
  {
      if (globals.debugging) foutput(INF "Verifying real chunk size on disc... fixed:\n       expected size: %u, real size: %lu\n", header->ckSize + 8 + (int) pad_byte, size);
      header->ckSize = (uint32_t) size - 8 ; // if prepending, ckSize was computed as the full size of raw file -8 bytes to which one must add the size of new header. Possible pad byte considered audio.
  }

  if (header->data_cksize == (uint32_t) filesize(file) - header->header_size_out - (int) (header->data_cksize % 2 == 1))
  {
      if (globals.debugging)
        foutput("%s\n", MSG "Verifying real data size on disc... OK");
  }
  else
  {
      if (globals.debugging) foutput(INF "Verifying real data size on disc... fixed:\n       header size: %d, expected size: %u, real size: %lu\n", header->header_size_out,
                                     header->data_cksize + header->header_size_out + (int) (header->data_cksize % 2 == 1),
                                     filesize(file));

      header->data_cksize = (uint32_t) filesize(file) - header->header_size_out ;  // if prepending, data_cksize was computed as the full size of raw file hence this new size minus HEADER_SIZE
  }

   return(false);
}

int prune(WaveData *info, WaveHeader *header)
{

  uint8_t p=0;
  uint32_t count=-1;
  uint64_t size=0;

 s_open(info->infile, "rb+");

  if (errno)
  {
     perror("\n"ERR "Could not state file size\n");
     return FAIL;
  }

// Count ending zeros to be pruned

  if (end_seek(fileptr(info->infile)) == FAIL)
      return(FAIL);

  do
    {
      if (fseek(fileptr(info->infile), -1, SEEK_CUR) == -1) return(FAIL);
      if (fread(&p, 1, 1, fileptr(info->infile)) != 1) return(FAIL);
      if (globals.debugging)  foutput("  Offset %"PRIu64"  : %"PRIu8" \n", (uint64_t) ftello(fileptr(info->infile)), p );
      if (fseek(fileptr(info->infile), -1, SEEK_CUR) == -1) return(FAIL);
      ++count;

    }
  while (p == 0);

  if (count > 0)
    {

      if (globals.debugging) foutput(INF "Pruning file: -%"PRIu32" bytes at: %"PRIu32"\n", count, header->ckSize + 8 - count );
      uint64_t offset;

//  Under Windows API, pruning from end of file
//  otherwise truncate takes full size as an argument

#if defined __WIN32__ || defined _WIN32 || defined __WIN32 || defined _WIN64 || defined __WIN64
      if (info->in_place) s_close(info->infile);
      offset=-count;
#else
      offset=size -count;

#endif
      // Truncating only if changes made in place, otherwise truncations results from incomplete copying at Checkout stage

      if ((info->in_place) && (truncate_from_end(filename(info->infile), offset) == -1))
        {
          perror("\n"ERR "truncate error\n");
          return(info->repair=FAIL);
        }

      if (globals.debugging) foutput("%s\n", INF "Readjusting byte count...");
      header->ckSize-=count;
      header->data_cksize-=count;
      info->prunedbytes=count;

      return(info->repair=BAD_DATA);
    }

  return(NO_PRUNE);
}





