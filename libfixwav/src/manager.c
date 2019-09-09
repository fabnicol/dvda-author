/*****************************************************************
*   manager.c
*   Copyright Fabrice Nicol 2008,2009
*   Description: launches subprocesses and manages output.
*
*******************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#ifndef _WIN32
#  include <unistd.h>
#endif

#include "fixwav.h"
#include "fixwav_auxiliary.h"
#include "repair.h"
#include "readHeader.h"
#include "checkData.h"
#include "checkParameters.h"
#include "fixwav_manager.h"
#include "c_utils.h"
#include "structures.h"
#include "winport.h"
#include "commonvars.h"
#include "libiberty.h"

#define off64_t  long long

extern globalData globals;

WaveHeader  *fixwav(WaveData *info, WaveHeader *header)
{

  // NULL init necessary

  int length=0;
  static int section;
  section++;

  // get the file statistics
  errno=0;

  // display the total file size for convenience
  // Patch on version 0.1.1: -int +uint64_t (int is not enough for files > 2GB)
  // NB: under Windows, use stat_file_size if file not open, otherwise use read_file_size

  if (file_exists(info->infile.filename))
      info->infile.filesize = stat_file_size(info->infile.filename);

  // outfile or infile may not exist yet, do not balk out

  if (! info->in_place && filesize(info->infile) == 0)
    {
      if (globals.debugging) foutput( "%s\n", WAR "File size is null; skipping ..." );
      info->repair=FAIL;
      goto getout;
    }

   if (! info->infile.isopen)
   {
       if (info->in_place)
       {
           if (filesize(info->infile) == 0)
               info->infile.fp = fopen(info->infile.filename, "wb+");
           else
               info->infile.fp = fopen(info->infile.filename, "rb+");
       }
       else
          info->infile.fp = fopen(info->infile.filename, "rb");
   }

   if (info->infile.fp == NULL) return NULL;

   info->infile.isopen = true;

  if (info->in_place)
  {
      info->outfile.fp = info->infile.fp;
      info->outfile.isopen = true;
  }

  if (! errno)
    {
      if (globals.debugging) foutput( "\n\n--FIXWAV section %d--\n\n"MSG_TAG "File size is %"PRIu64" bytes\n", section, filesize(info->infile));
    }


  errno=0;

  /* verify that the filename ends with 'wav' */

  if ( ((length=strlen(filename(info->infile)) - 3) <= 0) || ( strncmp(filename(info->infile) + length, "wav", 3)))
    {
      if (globals.debugging) foutput("%s%s%s\n",ERR "Found file '", filename(info->infile),"'");
      if (globals.debugging) foutput("%s\n", ERR "The filename must end in 'wav'.\nExiting ..." );
      info->repair=FAIL;
      goto getout;
    }

  /* checks incompatible options */

  int adjust=0;

  if (info->prepend && info->in_place && filesize(info->infile) != 0)
    {
      if (globals.debugging) foutput( "%s\n",   ERR "fixwav cannot prepend new header to raw data file in in-place mode.");
      if (info->interactive)
      {
          if (globals.debugging) foutput( "%s\n", "       use -o option instead. Press Y to exit...");
          if (isok())
          {
              info->repair=FAIL;
              goto getout;
          }
       }
      adjust=1;
    }

  /* 	  constraints are ordered according to the following hierarchy
   *
   * 		virtual > padding/prune
   *        virtual > prepend/in-place > in-place
   *
   *     if you do it virtual, you don't pad or prune or prepend
   *     if you prepend it's not in place and it's not virtual
   *
   *
   */


  if (info->virtual)
    {
      adjust=(info->prepend)+(info->in_place)+(info->prune)+(info->padding);
      info->prepend=info->in_place=info->prune=info->padding=0;
    }

   // fixwav cannot prepend to existing file. Create empty file first, prepend into it, then write
   // audio, then adjust header.

   if (info->prepend)
   {
       adjust=(info->in_place)+(info->virtual);
       const char*  path = filename(info->infile);
       if (file_exists(path) && stat_file_size(path))
           info->in_place=0;
       info->virtual=0;
   }

  if (adjust)
    if (globals.debugging) foutput(MSG_TAG "Adjusted options are: \n       info->prepend=%d\n       info->in_place=%d\n       info->prune=%d\n       info->padding=%d\n       info->virtual=%d\n",
              info->prepend, info->in_place, info->prune, info->padding, info->virtual);

#ifdef RADICAL_FIXWAV_BEHAVIOUR
  if (globals.silence && header->dwSamplesPerSec * header->wBitsPerSample * header->channels == 0)
    {
      if (globals.debugging)
          foutput("%s", ERR "In silent mode, bit rate, sample rate and channels must be set\n"
                        INF "Correcting options...\n");
      globals.silence=0;
    }
#endif

  /* check incompatible options and identify output and input for inplace mode */

  if (! info->in_place)
    {
      if (strcmp(filename(info->infile), filename(info->outfile)) == 0)
        {
          if (globals.debugging) foutput( "%s\n", ERR "input and output paths are identical. Press Y to exit...");
#        ifndef GUI_BEHAVIOR
          if (isok())
            {
#        endif
              info->repair=FAIL;
              goto getout;
#        ifndef GUI_BEHAVIOR
            }
#        endif
        }

      S_OPEN(info->outfile, "wb")
    }
  else
    {
      if (info->cautious)
      {
          if (globals.debugging) foutput("%s", "" WAR "in-place mode will change original file.\n");
          if (globals.debugging) foutput("%s\n",  ANSI_COLOR_RED "[INT]" ANSI_COLOR_RESET "  Enter Y to continue, otherwise press any key + return to exit.");

          if (!isok())
            {
              info->repair=FAIL;
              goto getout;
            }
      }

      info->outfile = info->infile;
    }


  /* reads header */

  /* pre parse header to find if is extensible and if has 'fact' ; collect facts in this case */

  if (info->prepend) goto Repair;
  errno = 0;
  parse_wav_header(info, header);

  /* if found info tags, dumps them in textfile database, which can only occur if span > 36 */

  if (header->ichunks > 0)
  {

      if (info->database == NULL) info->database=strdup("localdata");
      int l = strlen(info->database) + 10;
      char databasepath[l];
      sprintf(databasepath, "%s%s", info->database, SEPARATOR "database");
      secure_mkdir(info->database, 0755);
      FILE* database = NULL;
      secure_open(databasepath, "ab", database);
      if (database)
      {
        fprintf(database, "Filename    %s\nArtist      %s\nDate        %s\nStyle       %s\nComment     %s\nCopyright   %s\n\n",
                         header->INAM, header->IART, header->ICRD, header->IGNR, header->ICMT, header->ICOP);
        info->filetitle = strdup((const char*) header->INAM);
        fclose(database);
      }
      else
          foutput("%s", ERR "Could not open data base.\n");
   }

  if (header->header_size_in >= FIXBUF_LEN)
  {
       if (globals.debugging) foutput(WAR "Found unsupported WAV header with size exceeding %d byte limit\n", FIXBUF_LEN);
  }

  /* if no GUI, reverting to user input and resetting header_size to 0 if: failed to parse header, or prepending, or header_size > 255 */

  if (readHeader(info->infile.fp, header) == FAIL || info->prepend || header->header_size_in == MAX_HEADER_SIZE)
  {
        header->header_size_in = 0;
        info->repair = BAD_HEADER;
  }

  Repair:

  info->repair = repair_wav(info, header);

  /* for virtual fixwav, no tampering with sample counts on file as file cannot be altered
     if --no-padding is not activated for dvda-author, this will be corrected by core dvda-author routines */

  if (info->virtual) goto Checkout;

  /* to be able to alter sample counts, you must enforce --fixwav=padding on command line, unless you are pruning */

  /****************************************************
  *   Pruning	if requested
  *****************************************************/

  if  (info->prune)
    {
      if (globals.debugging) foutput("%s\n", INF "Pruning stage");

      switch (prune(info, header))
        {
        case NO_PRUNE:
          if (globals.debugging) foutput( "%s\n", INF "Fixwav status 3:\n       File was not pruned (no ending zeros)." );

          break;

        case BAD_DATA   :
          if (globals.debugging) foutput( "%s\n", INF "Fixwav status 3:\n       File was pruned." );
          info->prune=PRUNED;
          break;

        case FAIL       :
          if (globals.debugging) foutput( "%s\n", INF "Fixwav status 3:\n       Pruning failed." );
          info->repair=FAIL;
          goto getout;
        }
    }

 if (!info->padding) goto Checkout;

  /****************************************************
  *	Now checking whole number of samples
  *****************************************************/

  switch (check_sample_count(info, header))
    {
    case GOOD_HEADER:
      if (globals.debugging) foutput( "%s\n", MSG_TAG "Fixwav status 1:\n       Sample count is correct. No changes made to existing file." );
      break;

    case BAD_DATA   :
      if (globals.debugging) foutput("%s\n",  MSG_TAG "Fixwav status 1:\n       Sample count is corrupt." );

      if (globals.debugging  && info->padding)
      {

          if (info->padbytes == 1)
              foutput("%s", MSG_TAG "File was padded with 1 byte for sample count.\n");
          else
              foutput(MSG_TAG "File was padded with %d bytes for sample count.\n", info->padbytes);
      }

      info->repair=BAD_DATA;
      break;

    }

int header_size;

Checkout:

  /* checkout stage: check and possibly repair header data */

  header_size = (header->channels > 2 || header->wBitsPerSample > 16) ? HEADER_EXTENSIBLE_SIZE : HEADER_SIZE;
  header->is_extensible = (header_size == HEADER_EXTENSIBLE_SIZE);
  uint8_t *standard_header;

  switch (info->repair)
    {
    case	GOOD_HEADER:
      if (globals.debugging) foutput( "%s\n", MSG_TAG "Fixwav status 4:\n       WAVE header is correct. No changes made to existing header." );
      header->header_out = header->header_in;
      header->header_size_out = header->header_size_in;
      break;

    case	BAD_HEADER :
    case    BAD_DATA :

      if (globals.debugging) foutput( "%s\n", MSG_TAG "Fixwav status 4:\n       WAVE header has to be fixed." );

      standard_header = calloc(header_size, 1);
      if (standard_header == NULL) return NULL;
      memset(standard_header, 0, header_size);
      header->header_out = standard_header;
      header->header_size_out = header_size;

      /* to do: correct in_place facility and check padbytes */

      if ((info->repair = launch_repair(info, header)) == FAIL) break;

      if (! info->virtual)
      {

          if ((info->repair=dvda_write_header(info, header)) != FAIL)
          {
              if (globals.debugging) foutput("%s\n", INF "Header copy successful.\n");
              if (globals.maxverbose)
              {
                  if (! info->outfile.isopen) S_OPEN(info->outfile, "rb+");

                  if (globals.debugging) foutput("%s","Dumping new header:\n\n");

                  hexdump_header(info->outfile.fp, HEADER_SIZE);
              }
          }
          else
          {
              foutput("%s\n", ERR "Header could not be written.\n");
              break;
          }

          if (! info->in_place)
          {
              if (info->prepend) header->header_size_in = 0;  // safe-check, normally no-op

              if (copy_file_p(info->infile.fp, info->outfile.fp,
                              header->header_size_in,
                              filesize(info->infile) - header->header_size_in) == PAD)

              if (info->padbytes) pad_end_of_file(info);

              info->repair=BAD_HEADER;
          }
          else
              if (info->padbytes)
                  pad_end_of_file(info);
      }
      else
      {

          if (globals.debugging) foutput( "%s\n", MSG_TAG "Fixwav status 4:\n       WAVE header is incorrect, yet no changes were made to existing header." );
          free(header->header_out);
          header->header_out = NULL;
          header->header_in = NULL;
          header->header_size_out = 0;
          header->header_size_in = 0;
      }

      break;

    case	FAIL       :
      if (globals.debugging) foutput( "%s\n", MSG_TAG "Fixwav status 4:\n       Failure at repair stage." );

    }

// end of program

getout:

#if 0
  check_real_size(info, header);
#endif

  S_OPEN(info->outfile, "rb+")

  if  (info->repair == BAD_HEADER)
    if (globals.debugging) foutput( "%s\n", INF "Fixwav status--summary:\n       HEADER chunk corrupt: fixed." );

  if (globals.debugging && info->repair == BAD_DATA)
    {
      if (info->prune)
        foutput( "%s\n", INF "Fixwav status--summary:\n       DATA chunk was adjusted after pruning." );
      else
        foutput( "%s\n", INF "Fixwav status--summary:\n       DATA chunk was corrupt: fixed." );
    }


  if (globals.debugging) foutput( "\n--FIXWAV End of section %d --\n\n", section );

  if (! info->virtual)
    {
      errno = 0;

      if (info->in_place)
        info->infile = info->outfile;

      S_CLOSE(info->outfile)
      info->infile.fp = NULL; // necessary
      S_CLOSE(info->infile)

      if (errno)
        {
          if (globals.debugging) foutput("%s\n", WAR "fclose error: issues may arise.");
          return(NULL);
        }
    }
  else
    {
      if (info->infile.fp == NULL)
        {
          if (globals.debugging) foutput("%s\n", WAR "File pointer is NULL.");
          return(NULL);
        }

      errno = 0;
      S_CLOSE(info->infile)
      if (errno)
        {
          if (globals.debugging) foutput("%s\n", WAR "fclose error: issues may arise.");
          return(NULL);
        }
    }

  if ((info->repair == FAIL)  || (filesize(info->outfile) == 0)  || ( (info->repair == GOOD_HEADER) && (!info->in_place) && (!info->virtual) ))
    {
      // getting rid of empty files and useless work copies

      if (file_exists(filename(info->outfile)))
          unlink(filename(info->outfile));

    }

  errno=0;

  if (info->repair != FAIL)
    {

      return(header);
    }

  else return NULL;
}


