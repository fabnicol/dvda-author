/* ==========================================================================
*
*   repair.c
*   Original code: Pigiron 2007,
*   Revision: Fabrice Nicol 2008-2009, 2016 <fabnicol@users.sourceforge.net>
*
*   Description: Performs repairs on header and file.
*========================================================================== */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fixwav.h"
#include "repair.h"
#include "checkParameters.h"
#include "fixwav_auxiliary.h"
#include "fixwav_manager.h"
#include "c_utils.h"
#include "winport.h"
#include "structures.h"
#include "libiberty.h"
/* global data */

extern globalData globals;


/*********************************************************************
* Function: repair_wav
*
* Purpose:  This function will analyze and repair the header
*********************************************************************/
int
repair_wav(WaveData *info, WaveHeader *header )
{

  FILE* infile=info->INFILE;
  int repair=GOOD_HEADER;
  _Bool pad_byte = false;

  errno=0;

  uint64_t file_size=read_file_size(infile, info->infile);

  /*********************************************************************
  * The RIFF Chunk
  *********************************************************************/

  /* the first 4 bytes should be "RIFF" */
  if (memcmp(&header->chunk_id, "RIFF", 4) == 0)
    {
      printf( "%s", ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET"  Found correct Chunk ID at offset 0\n" );
    }
  else
    {
      printf( "%s", ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET"  Repairing Chunk ID at offset 0\n" );
      if (memmove( &(header->chunk_id), "RIFF", 4 * sizeof(char)) == NULL) return(FAIL);
      repair = BAD_HEADER;
    }

  /* The ChunkSize is the entire file size - 8
   * unless a pad byte was added when the file was written  (patch Aug. 2016) */
  if (header->chunk_size == file_size - 8)
    {
      printf( ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET"  Found correct Chunk Size of %" PRIu32 " bytes at offset 4\n",
                 header->chunk_size);
    }
  else
  if (header->chunk_size == file_size - 7)
    {
      printf( ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET"  Found correct Chunk Size of %" PRIu32 " bytes at offset 4. Pad byte added at EOF.\n",
                 header->chunk_size);
      pad_byte = true;
    }
  else
    {
      /* Supposing file was not 1-byte padded for imparity ... */

      printf( ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Chunk Size of %" PRIu32 " at offset 4 is incorrect: should be %" PRIu32 " bytes\n"
              ANSI_COLOR_BLUE  "[INF]" ANSI_COLOR_RESET "  ... repairing\n",
              header->chunk_size,
              (uint32_t) file_size-8);

      header->chunk_size = (uint32_t) file_size - 8;
      repair = BAD_HEADER;
    }

  /* The Chunk Format should be the letters "WAVE" */

  if (memcmp(&header->chunk_format, "WAVE", 4) == 0)
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Found correct Chunk Format at offset 8" );
    }
  else
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Chunk Format at offset 8 is incorrect\n" ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET"  ... repairing\n" );
      if (memmove(&(header->chunk_format), "WAVE", 4 * sizeof(char)) == NULL)
          return(FAIL);

      repair = BAD_HEADER;
    }

  /*********************************************************************/

  /* The "fmt " Subchunk                                                */
  /*********************************************************************/

  /* The Subchunk1 ID should contain the letters "fmt " */

  if (memcmp(&header->sub_chunk, "fmt", 3) == 0)
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET"  Found correct Subchunk1 ID at offset 12" );
    }
  else
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET"  Subchunk1 ID at offset 12 is incorrect\n" ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET "  ... repairing" );
      // "fmt " ends in a space
      if (memmove( &(header->sub_chunk), "fmt ", 3 * sizeof(char) ) == NULL)
          return(FAIL);
      repair = BAD_HEADER;
    }

  /* The Subchunk1 Size is 16, 18 or 40 for PCM (patch Aug. 2016) */

  if (header->sc_size == 16 || header->sc_size == 18 || header->sc_size == 40)
    {
      printf(ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Found correct Subchunk1 Size of %" PRIu32 " bytes at offset 16\n", header->sc_size );
    }
  else
    {
      printf(ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Subchunk1 Size at offset 16 is incorrect (%d) \n" ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET "  ... repairing.\n",  header->sc_size);
      header->sc_size = 16;
      /* to be corrected later based on fmt chunk real size ? */
      repair = BAD_HEADER;
    }

  /* The Subchunk1 Audio Format is 1 for WAVE_FORMAT_PCM or 0xFFFE for WAVE_FORMAT_EXTENSIBLE*/

  if (header->sc_format == 1 || header->sc_format == 0xFFFE)
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Found correct Subchunk1 Format at offset 20" );
      if (header->sc_format == 0xFFFE)
          printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Found WAVE_FORMAT_EXTENSIBLE header");
    }
  else
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Subchunk1 Format at offset 20 is incorrect\n" ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET "  ... repairing" );
      /* by default, assuming standard PCM data */
      header->sc_format = 1;
      repair = BAD_HEADER;
    }

  /****************************************
  *  On to core audio parameters
  *****************************************/

  if (info->automatic)
  {
      repair = (auto_control(info, header) == BAD_HEADER)? BAD_HEADER: ((repair == BAD_HEADER)? BAD_HEADER : GOOD_HEADER) ;

      printf(ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Audio characteristics found by automatic mode:\n       bits/s=%" PRIu16 ", sample rate=%" PRIu32 ", channels=%" PRIu16 "\n",
                header->bit_p_spl, header->sample_fq, header->channels);

      if (repair == BAD_HEADER)
      {
#       ifndef GUI_BEHAVIOR
          if (info->interactive)
            {
              printf("%s\n", "[INT]  Please confirm [y/n] ");
              if (!isok())
                {
                  printf("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Exiting automatic mode: user mode\n");
                  repair=user_control(info, header);
                }
            }
          else
#       endif
            printf("%s\n", "[INT]  Non-interactive mode: assuming correct repair.");
     }
  }
# ifndef GUI_BEHAVIOR
  else
    repair= (user_control(info, header) == BAD_HEADER)? BAD_HEADER: (repair == BAD_HEADER)? BAD_HEADER : GOOD_HEADER ;
# endif

  printf("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Core audio characteristics were checked.\n       Now processing data subchunk");

  /*********************************************************************/
  /* The "data" Subchunk                                               */
  /*********************************************************************/

  /* The Subchunk2 ID is the ASCII characters "data" */

  if (memcmp(&header->data_chunk, "data", 4) == 0)
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Found correct Subchunk2 ID" );
    }
  else
    {
      printf("%s\n",  ANSI_COLOR_GREEN "[MSG]" ANSI_COLOR_RESET "  Subchunk2 ID at offset 36 is incorrect\n" ANSI_COLOR_BLUE "[INF]" ANSI_COLOR_RESET "  ... repairing\n" );
      if (memmove(&header->data_chunk,"data", 4*sizeof(char) ) == NULL) return(FAIL);
      repair = BAD_HEADER;
    }

  /* The Subchunk2 Size = NumSample * NumChannels * BitsPerSample/8 */

  if (header->data_size == file_size - header->header_size_in || (pad_byte && header->data_size == file_size - header->header_size_in - 1))  // -1 if pad byte was added
    {
      printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Found correct Subchunk2 Size of %"PRIu32" bytes at offset %d\n", 
             header->data_size,
             header->header_size_in-4);
      if (pad_byte)
        printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Pad byte was not taken into account.\n");
    }
  else
    {
      printf(ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Subchunk2 Size at offset %d is incorrect: found %"PRIu32" bytes instead of %"PRIu32"\n"ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  ... repairing\n",
             header->header_size_in-4,
             header->data_size,
             (uint32_t) file_size - header->header_size_in - (uint32_t) pad_byte);
             
      header->data_size = file_size - header->header_size_in - (uint32_t) pad_byte;
      repair = BAD_HEADER;
    }

  return(repair);

}




int launch_repair(WaveData *info, WaveHeader *header)
{
  uint8_t *p=header->header_out;
  
  printf( "%s", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Writing new header...\n" );

  /* if -o option is not used, fixwav will overwrite existing data; confirmation dialog */

  if (info->in_place)
    {
      if (info->cautious)
      {
          printf( "\n%s", "[INT]  Overwrite the existing file? [y/n] " );
         if (!isok())

            {   /* user's bailing */
              if ( info->repair == BAD_DATA )
                printf( "%s",""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  Header may still be corrupt.\n" );
              else
                printf( "%s", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  No changes made to existing file\n" );
              return(info->repair) ;
            }
      }
    }

  /* if OK */
  /* readjusting sizes after normalization */
  /* if prepending, always not in place and header_size_in=0 */


  /* refresh header sizes
   * hence we substract the shrinking from original header to the standard one
   * to which one must add the prepending of a standard header, should it occur
   */

  header->data_size += header->header_size_in-header->header_size_out;  // will be padded by zeros

 // otherwise unchanged
  
  header->chunk_size=header->data_size+header->header_size_out-8;
  
  /* write the new header at the beginning of the file */
  /* again, copying manually rather than invoking fread to ensure cross-compiler/platform portability */

  uint32_copy_reverse(p, header->chunk_id), p+=4;
  uint32_copy_reverse(p, header->chunk_size), p+=4;
  uint32_copy_reverse(p, header->chunk_format), p+=4;
  uint32_copy_reverse(p, header->sub_chunk), p+=4;
  uint32_copy_reverse(p, header->sc_size), p+=4;
  uint16_copy_reverse(p, header->sc_format), p+=2;
  uint16_copy_reverse(p, header->channels), p+=2;
  uint32_copy_reverse(p, header->sample_fq), p+=4;
  uint32_copy_reverse(p, header->byte_p_sec), p+=4;
  uint16_copy_reverse(p, header->byte_p_spl), p+=2;
  uint16_copy_reverse(p, header->bit_p_spl), p+=2;
  uint32_copy_reverse(p, header->data_chunk), p+=4;
  uint32_copy_reverse(p, header->data_size);
 
  return(info->repair) ;
}


int write_header(WaveData *info, WaveHeader *header)
{
  FILE* outfile=info->OUTFILE; 
  // Only repairing headers virtually to cut down computing times (--fixwav-virtual)

  if (info->virtual) return(info->repair);

  int count=0;

  //closing and opening again for security/sync, may be a non-op//

  if ((!info->virtual) && ( (outfile == NULL) || (fclose(outfile) != 0) || ((outfile=fopen(info->outfile, "rb+")) == NULL) ))
    {
      printf("%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Failed to close/open file");
      return(FAIL);
    }

  // in manager.c a sanity check ensures that if (info->prepend) then !(info->in_place)
  // Otherwise copying fixed header to new file or in place, depending on option

  /* try to seek right offset depending on type of file */

  if (info->in_place && globals.debugging)
    {
      printf("%s\n", ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Overwriting header...");
    }

  count=fwrite(header->header_out, header->header_size_out, 1, outfile ) ;
 if (header->is_extensible)
    {
    /* wiping out wavext+14 bytes (36 for SoX 24-bit headers)*/
      for (int i=0; i < header->wavext+WAV_EXTENSION_LENGTH; i++)
        fputc(0,outfile);
    }

  if (count != 1)
    {
      fprintf( stderr, "%s\n", ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Error updating wav file.");
      return(FAIL);
    }

  if (errno)
    {
      perror("\n"ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Error in launch repair module\n");
      return(FAIL);
    }

  return(COPY_SUCCESS);
}
