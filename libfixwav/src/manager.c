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
#define _GNU_SOURCE 1
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>
#include <stdint.h>
#include <inttypes.h>

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


extern globalData globals;

WaveHeader  *fixwav(WaveData *info, WaveHeader *header)
{

  // NULL init necessary
  FILE* infile=NULL, *outfile=NULL;
  int length=0;
  uint64_t size=0;
  static int section;
  section++;

  // get the file statistics
  errno=0;

  // display the total file size for convenience
  // Patch on version 0.1.1: -int +uint64_t (int is not enough for files > 2GB)
  // NB: under Windows, use stat_file_size if file not open, otherwise use read_file_size
  if (info->infile)   size=stat_file_size(info->infile);

  if (!errno)
    {
      printf( "\n\n--FIXWAV section %d--\n\n[MSG]  File size is %"PRIu64" bytes\n", section, (uint64_t)  size );
    }
  else
    {
      perror("[ERR]  Could not stat regular file");
      info->repair=FAIL;
      goto getout;
    }

  if (size == 0)
    {
      printf( "%s\n", "[WAR]  File size is null; skipping ..." );
      info->repair=FAIL;
      goto getout;
    }

  errno=0;

  /* verify that the filename ends with 'wav' */

  if ( ((length=strlen(info->infile) - 3) <= 0) || ( strncmp( info->infile + length, "wav", 3 ) ))
    {
      printf("%s\n", "[WAR]  The filename must end in 'wav'.\nExiting ..." );
      info->repair=FAIL;
      goto getout;
    }

  /* checks incompatible options */

  int adjust=0;

  if ((info->prepend) && (info->in_place))
    {
      printf( "%s\n",   "[ERR]  fixwav cannot prepend new header to raw data file in in-place mode.");
      printf( "%s\n", "       use -o option instead. Press Y to exit...");
      if (isok())
        {
          info->repair=FAIL;
          goto getout;
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

   if (info->prepend)
   {
       adjust=(info->in_place)+(info->virtual);
       info->in_place=0;
       info->virtual=0;
   }

  if (adjust)
    printf("[MSG]  Adjusted options are: \n       info->prepend=%d\n       info->in_place=%d\n       info->prune=%d\n       info->padding=%d\n       info->virtual=%d\n",
              info->prepend, info->in_place, info->prune, info->padding, info->virtual);


#ifdef RADICAL_FIXWAV_BEHAVIOUR
  if ((globals.silence) && ((header->sample_fq)*(header->bit_p_spl)*(header->channels) == 0))
    {
      fprintf(stderr, "%s", "[ERR]  In silent mode, bit rate, sample rate and channels must be set\n[INF]  Correcting options...\n");
      globals.silence=0;
    }
#endif


  /* open the existing file */

  if (info->virtual)
    infile=secure_open(info->infile, "rb+");
  else
    {

      if (!info->in_place)
        {
          if (strcmp(info->infile, info->outfile))
            {
              infile=secure_open(info->infile, "rb");
              outfile=secure_open(info->outfile, "ab");
            }
          else
            {
              printf( "%s\n", "[ERR]  input and output paths are identical. Press Y to exit...");
              if (isok())
                {
                  info->repair=FAIL;
                  goto getout;
                }
            }
        }
      else
        {
          if (info->cautious)
          {
              printf("%s", "[WAR]  in-place mode will change original file.\n");
              printf("%s\n",   "[INT]  Enter Y to continue, otherwise press any key + return to exit.");

              if (!isok())
                {
                  info->repair=FAIL;
                  goto getout;
                }
          }

          printf("[INF]  Opening %s\n", info->outfile);

          info->outfile=info->infile;
          outfile=infile=secure_open(info->infile, "rb+");

        }
    }



  /* reads header */

  uint8_t span=0;
  infochunk ichunk;
  memset(&ichunk, 0, sizeof(infochunk));
  if (!info->prepend) parse_wav_header(infile, &ichunk);
  span=ichunk.span;

  /* if found info tags, dumps them in textfile database, which can only occur of span > 36 */

  if ((span > 36)&&(ichunk.found))
  {
      char databasepath[MAX_OPTION_LENGTH+9]={0};
      snprintf(databasepath, MAX_OPTION_LENGTH+9, "%s%s", info->database, SEPARATOR"database");
      secure_mkdir(info->database, 0755, DEFAULT_DATABASE_FOLDER);
      FILE* database = secure_open(databasepath, "ab");
      fprintf(database, "Filename    %s\nArtist      %s\nDate        %s\nStyle       %s\nComment     %s\nCopyright   %s\n\n", ichunk.INAM, ichunk.IART, ichunk.ICRD, ichunk.IGNR, ichunk.ICMT, ichunk.ICOP);
      info->filetitle=strdup((const char*) ichunk.INAM);
      fclose(database);
   }


  header->header_size=(span > 0)? ((span+8 < 256)? span+8 : MAX_HEADER_SIZE) : MAX_HEADER_SIZE;


  /* reverting to user input and resetting header_size to 0 if: failed to parse header, or prepending, or header_size > 255 */
  if ((readHeader(infile, header) == FAIL) || (info->prepend)|| (header->header_size == MAX_HEADER_SIZE))
  {
      info->interactive=1;
      info->automatic=0;
      header->header_size=0;
      info->repair=BAD_HEADER;
  }

  info->repair = repair_wav(infile, info, header );

  /* for virtual fixwav, no tampering with sample counts on file as file cannot be altered
     if --no-padding is not activated for dvda-author, this will be corrected by core dvda-author routines */

  if (info->virtual) goto Checkout;


  /* to be able to alter sample counts, you must enforce --fixwav=padding on command line, unless you are pruning */



  /****************************************************
  *   Pruning	if requested
  *****************************************************/




  if  (info->prune)
    {

      printf("%s\n", "[INF]  Pruning stage");


      switch (prune(infile, outfile, info, header))
        {
        case NO_PRUNE:
          printf( "%s\n", "[INF]  Fixwav status 3:\n       File was not pruned (no ending zeros)." );

          break;

        case BAD_DATA   :
          printf( "%s\n", "[INF]  Fixwav status 3:\n       File was pruned." );
          info->prune=PRUNED;
          break;

        case FAIL       :
          printf( "%s\n", "[INF]  Fixwav status 3:\n       Pruning failed." );
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
      printf( "%s\n", "[MSG]  Fixwav status 1:\n       Sample count is correct. No changes made to existing file." );
      break;

    case BAD_DATA   :
      printf("%s\n",  "[MSG]  Fixwav status 1:\n       Sample count is corrupt." );

      if (info->padding)
      {

          if (info->padbytes == 1) printf("%s", "[MSG]  File was padded with 1 byte for sample count.\n");
          else  printf("[MSG]  File was padded with %d bytes for sample count.\n", info->padbytes);
      }

      info->repair=BAD_DATA;
      break;

    }


  /****************************************************
   *	Now checking evenness of sample count
   *****************************************************/

  switch (check_evenness(info, header))
    {
    case GOOD_HEADER:
      printf( "%s\n", "[MSG]  Fixwav status 2:\n       Even count of bytes." );
      break;

    case BAD_DATA   :
      printf( "%s\n", "[MSG]  Fixwav status 2:\n       Byte count is odd." );
      if (info->padding) printf( "%s\n", "[MSG]  File was padded with one byte." );
      info->repair=BAD_DATA;
      break;

    }



Checkout:

  /* checkout stage: check and possibly repair header data */

  switch (info->repair)
    {
    case	GOOD_HEADER:
      printf( "%s\n", "[MSG]  Fixwav status 4:\n       WAVE header is correct. No changes made to existing header." );
      break;

    case	BAD_HEADER :
    case    BAD_DATA :

      printf( "%s\n", "[MSG]  Fixwav status 4:\n       WAVE header corrupt." );

      uint8_t standard_header[HEADER_SIZE]={0};


      if ((info->repair=launch_repair(info, header, standard_header)) == FAIL) break;

          if (!info->virtual)
            {
              uint64_t   input_file_byte_size=header->chunk_size+8-HEADER_SIZE;

              if ((info->repair=write_header(standard_header, outfile, info)) != FAIL)
                {

                  printf("%s\n", "[INF]  Header copy successful.\n");
                  if (fclose(outfile) != 0) return(NULL);
                  outfile=secure_open(info->outfile, "rb+");
                  if (globals.debugging) {
                       printf("%s","Dumping new header:\n\n");
                       hexdump_header(outfile, HEADER_SIZE);
                  }
                }
              else break;

              if (!info->in_place)
                {
                  if (copy_file_p(infile, outfile, (info->prepend) ? 0 : header->header_size, input_file_byte_size) == PAD)
                      if (info->padbytes) pad_end_of_file(outfile, info->padbytes);

                  info->repair=BAD_HEADER;
                }
              else
                if (info->padbytes) pad_end_of_file(outfile, info->padbytes);


            }


      break;


    case	FAIL       :
      printf( "%s\n", "[MSG]  Fixwav status 4:\n       Failure at repair stage." );

    }

// end of program

getout:

  if (check_real_size(infile, outfile, info, header)) goto Checkout;

  if  (info->repair == BAD_HEADER)
    printf( "%s\n", "[INF]  Fixwav status--summary:\n       HEADER chunk corrupt: fixed." );

  if  (info->repair == BAD_DATA)
    {
      if (info->prune == PRUNED)
        printf( "%s\n", "[INF]  Fixwav status--summary:\n       DATA chunk was adjusted after pruning." );
      else
        printf( "%s\n", "[INF]  Fixwav status--summary:\n       DATA chunk was corrupt: fixed." );
    }


  printf( "\n--FIXWAV End of section %d --\n\n", section );

  if (!info->virtual)
    {
      if ((infile == NULL) || (outfile == NULL) )
        {
          fprintf(stderr, "%s\n", "[WAR]  File pointer is NULL.");
          return(NULL);
        }

      if ((fclose(infile) == EOF) || ((!info->in_place)  && (fclose(outfile) == EOF)) )
        {

          fprintf(stderr, "%s\n", "[WAR]  fclose error: issues may arise.");
          return(NULL);
        }
    }
  else
    {
      if (infile == NULL)
        {
          fprintf(stderr, "%s\n", "[WAR]  File pointer is NULL.");
          return(NULL);
        }

      if (fclose(infile) == EOF)
        {

          fprintf(stderr, "%s\n", "[WAR]  fclose error: issues may arise.");
          return(NULL);
        }
    }

  if ((info->repair == FAIL)  || (size == 0)  || ( (info->repair == GOOD_HEADER) && (!info->in_place) && (!info->virtual) ))
    {
      // getting rid of empty files and useless work copies
      errno=0;
      unlink(info->outfile);
      if (errno)
        printf("%s%s\n", "[ERR]  unlink: ", strerror(errno));
    }

  if (info->repair != FAIL)
    {
      errno=0;
      return(header);
    }

  else return NULL;
}


