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

  int repair=GOOD_HEADER;
  _Bool pad_byte = false;

  errno=0;

  /****************************************
  *  On to core audio parameters
  *****************************************/

  if (info->automatic)
  {
      repair = (auto_control(info, header) == BAD_HEADER)? BAD_HEADER: ((repair == BAD_HEADER)? BAD_HEADER : GOOD_HEADER) ;

      if (globals.debugging) foutput(MSG "Audio characteristics found by automatic mode:\n       bits/s=%" PRIu16 ", sample rate=%" PRIu32 ", channels=%" PRIu16 "\n",
                header->wBitsPerSample, header->dwSamplesPerSec, header->channels);

      if (repair == BAD_HEADER)
      {
#       ifndef GUI_BEHAVIOR
          if (info->interactive)
            {
              if (globals.debugging) foutput("%s\n", "[INT]  Please confirm [y/n] ");
              if (!isok())
                {
                  if (globals.debugging) foutput("%s\n", INF "Exiting automatic mode: user mode\n");
                  repair=user_control(info, header);
                }
            }
          else
#       endif
            if (globals.debugging) foutput("%s\n", "[INT]  Non-interactive mode: assuming correct repair.");
     }
  }
# ifndef GUI_BEHAVIOR
  else
    repair= (user_control(info, header) == BAD_HEADER)? BAD_HEADER: (repair == BAD_HEADER)? BAD_HEADER : GOOD_HEADER ;
# endif

  if (globals.debugging) foutput("%s\n", INF "Core audio characteristics were checked.\n       Now processing data subchunk");

  /*********************************************************************
  * The RIFF Chunk
  *********************************************************************/

  /* the first 4 bytes should be "RIFF" */
  if (memcmp(&header->ckID, "RIFF", 4) == 0)
    {
      if (globals.debugging) foutput( "%s", MSG "Found correct RIFF ID at offset 0\n" );
    }
  else
    {
      if (globals.debugging) foutput( "%s", INF "Repairing RIFF ID at offset 0\n" );
      if (memmove( &(header->ckID), "RIFF", 4 * sizeof(char)) == NULL) return(FAIL);
      repair = BAD_HEADER;
    }

  /* The ChunkSize is the entire file size - 8
   * unless a pad byte was added when the file was written  (patch Aug. 2016)
       "This size value does not include the size of the ckID or ckSize fields
        or the pad byte at the end of
        ckData."
  */

  if (header->ckSize == filesize(info->infile)  - 8 )
    {
      if (globals.debugging) foutput( MSG "Found correct audio chunk Size of %" PRIu32 " bytes at offset 4\n",  header->ckSize);
    }
  else
  if ((header->ckSize & 1) == 1 && header->ckSize == filesize(info->infile)  - 9)
    {
      if (globals.debugging) foutput( MSG "Found correct audio chunk Size of %" PRIu32 " bytes at offset 4. Pad byte added at EOF.\n",  header->ckSize);
      pad_byte = true;
    }
  else
    {
      /* there is here no other logical option than to consider the possible pad byte at eof as part of audio data */

      if (globals.debugging) foutput( MSG "audio chunk Size of %" PRIu32 " at offset 4 is incorrect: should be %" PRIu32 " bytes\n"
              INF "... repairing\n",
              header->ckSize,
              (uint32_t) filesize(info->infile)  - 8);

      header->ckSize = (uint32_t) filesize(info->infile)  - 8;
      pad_byte = false;
      repair = BAD_HEADER;
    }

  /* The Chunk Format should be the letters "WAVE" */

  if (memcmp(&header->WAVEID, "WAVE", 4) == 0)
    {
      if (globals.debugging) foutput("%s\n",  MSG "Found correct WAVE Format at offset 8" );
    }
  else
    {
      if (globals.debugging) foutput("%s\n",  MSG "WAVE Format at offset 8 is incorrect\n" INF "... repairing\n" );
      if (memmove(&(header->WAVEID), "WAVE", 4 * sizeof(char)) == NULL)
          return(FAIL);

      repair = BAD_HEADER;
    }

  /*********************************************************************/

  /* The "fmt " Subchunk                                                */
  /*********************************************************************/

  /* The Subchunk1 ID should contain the letters "fmt " */

  if (memcmp(&header->fmt_ckID, "fmt ", 4) == 0)
    {
      if (globals.debugging) foutput("%s\n",  MSG "Found correct fmt ID at offset 12" );
    }
  else
    {
      if (globals.debugging) foutput("%s\n",  MSG "fmt ID at offset 12 is incorrect\n" INF "... repairing" );
      // "fmt " ends in a space
      if (memmove( &(header->fmt_ckID), "fmt ", 4 * sizeof(char) ) == NULL)
          return(FAIL);
      repair = BAD_HEADER;
    }



 /* Consequences on WAVE_FORMAT type correction */

  /* The fmt chunk Size is 16, 18 or 40 for PCM (patch Aug. 2016) */

  if (header->fmt_ckSize == 16 || header->fmt_ckSize == 18 || header->fmt_ckSize == 40)
    {
      if (globals.debugging) foutput(MSG "Found correct fmt chunk Size of %" PRIu32 " bytes at offset 16\n", header->fmt_ckSize );
    }
  else
    {
      if (globals.debugging) foutput(MSG "fmt chunk Size at offset 16 is incorrect (%d) \n" INF "... repairing.\n",  header->fmt_ckSize);

      /* for mono or stereo, stick to WAVE_FORMAT_PCM. Otherwise move to WAVE_FORMAT_EXTENSIBLE with fmt_ckSize 40 */

      switch (header->channels)
      {
         case 1:
         case 2: header->fmt_ckSize = 16;
          break;
         default:
          header->fmt_ckSize = 40;
      }

      /* to be corrected later based on fmt chunk real size ? */
      repair = BAD_HEADER;
    }

  /* The Subchunk1 Audio Format is 1 for WAVE_FORMAT_PCM or 0xFFFE for WAVE_FORMAT_EXTENSIBLE*/

  if (header->wFormatTag == 1 || header->wFormatTag == 0xFFFE)
    {
      if (globals.debugging) foutput("%s\n",  MSG "Found correct wave Format Tag at offset 20" );
      if (header->wFormatTag == 0xFFFE)
          if (globals.debugging) foutput("%s\n",  MSG "Found WAVE_FORMAT_EXTENSIBLE header");
    }
  else
    {
      if (globals.debugging) foutput("%s\n",  MSG "Subchunk1 Format at offset 20 is incorrect\n" INF "... repairing" );
      switch (header->channels)
      {
         case 1:
         case 2:       header->wFormatTag = 1;
          break;
         default:
                header->wFormatTag = 0xFFFE;
      }

      repair = BAD_HEADER;
    }

  /*********************************************************************/
  /* The "data" Subchunk                                               */
  /*********************************************************************/

  /* The data chunk ID is the ASCII characters "data" */

  if (memcmp(&header->data_ckID, "data", 4) == 0)
    {
      if (globals.debugging) foutput("%s\n",  MSG "Found correct data chunk ID" );
    }
  else
    {
      if (globals.debugging) foutput("%s\n",  MSG "data chunk ID is incorrect\n" INF "... repairing\n" );
      if (memmove(&header->data_ckID,"data", 4*sizeof(char) ) == NULL) return(FAIL);
      repair = BAD_HEADER;
    }

  /* The data chunk Size = NumSample * NumChannels * BitsPerSample/8 */

  if (header->data_cksize == filesize(info->infile)  - header->header_size_in
      || (pad_byte && header->data_cksize == filesize(info->infile)  - header->header_size_in - 1))  // -1 if pad byte was added
    {
      if (globals.debugging) foutput(MSG  "  Found correct data ckSize of %"PRIu32" bytes at offset %d\n",
             header->data_cksize,
             header->header_size_in - 4);
      if (pad_byte && globals.debugging) foutput("%s\n", MSG "  Pad byte was not taken into account.");
    }
  else
    {
      if (globals.debugging)
          foutput(MSG "data_ckSize at offset %d is incorrect: found %"
                  PRIu32 " bytes instead of\n       %"
                  PRIu32 " = file size (%"
                  PRIu32 ") - header size (%"
                  PRIu16 ") - pad byte (%d)\n"
                  INF "... repairing\n",
             header->header_size_in - 4,
             header->data_cksize,
             (uint32_t) filesize(info->infile) - header->header_size_in - (uint32_t) pad_byte,
             (uint32_t) filesize(info->infile), header->header_size_in, pad_byte);
             
      header->data_cksize = filesize(info->infile)  - header->header_size_in - (uint32_t) pad_byte;
      repair = BAD_HEADER;
    }

  return(repair);
}

int launch_repair(WaveData *info, WaveHeader *header)
{
  uint8_t *p=header->header_out;
  
  if (globals.debugging) foutput( "%s", INF "Writing new header...\n" );

  /* if -o option is not used, fixwav will overwrite existing data; confirmation dialog */

  if (info->in_place)
    {
      if (info->cautious)
      {
         if (globals.debugging) foutput( "\n%s", "[INT]  Overwrite the existing file? [y/n] " );
         if (!isok())

            {   /* user's bailing */
              if (globals.debugging && info->repair == BAD_DATA )
              {
                foutput( "%s",WAR "Header may still be corrupt.\n" );
              }
              else
              {
                foutput( "%s", INF "No changes made to existing file\n" );
              }
              return(info->repair) ;
            }
      }
    }

  uint32_copy_reverse(p, header->ckID), p+=4;
  uint32_copy_reverse(p, header->ckSize), p+=4;
  uint32_copy_reverse(p, header->WAVEID), p+=4;
  uint32_copy_reverse(p, header->fmt_ckID), p+=4;
  uint32_copy_reverse(p, header->fmt_ckSize), p+=4;
  uint16_copy_reverse(p, header->wFormatTag), p+=2;
  uint16_copy_reverse(p, header->channels), p+=2;
  uint32_copy_reverse(p, header->dwSamplesPerSec), p+=4;
  uint32_copy_reverse(p, header->nAvgBytesPerSec), p+=4;
  uint16_copy_reverse(p, header->nBlockAlign), p+=2;
  uint16_copy_reverse(p, header->wBitsPerSample), p+=2;

  /* WAVE_FORMAT_EXTENSIBLE SPECIFICS */

  if (header->channels > 2)
  {

      header->cbSize = 0x16; // extension is 22 bytes
      uint16_copy_reverse(p, header->cbSize), p+=2;
      uint16_copy_reverse(p, header->wBitsPerSample), p+=2;  // in principle, wValidBitsPerSample
      uint32_copy_reverse(p, header->dwChannelMask), p+=4;

      const uint8_t GUID[16] = {1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71};

      memcpy(p, GUID, 16), p+=16;

      const uint8_t FACT[8] = {'f', 'a', 'c', 't', 4, 0, 0, 0};

      memcpy(p, FACT, 8), p+=8;

      uint32_copy_reverse(p, header->data_cksize/(header->channels * header->wBitsPerSample / 8)), p+=4;
  }

  uint32_copy_reverse(p, header->data_ckID), p+=4;
  uint32_copy_reverse(p, header->data_cksize);
 
  return(info->repair) ;
}

int write_header(WaveData *info, WaveHeader *header)
{
  // Only repairing headers virtually to cut down computing times (--fixwav-virtual)

  if (info->virtual) return(info->repair);

  S_CLOSE(info->outfile)

  if (file_exists(filename(info->outfile)))
      S_OPEN(info->outfile, "rb+")
  else
      S_OPEN(info->outfile, "wb")


  int count=0;

  //closing and opening again for security/sync, may be a non-op//
#if 0
  if ((!info->virtual) && ( (outfile == NULL) || (fclose(outfile) != 0) || ((outfile=fopen(info->outfile, "rb+")) == NULL) ))
    {
      if (globals.debugging) foutput("%s\n", ERR "Failed to close/open file");
      return(FAIL);
    }
#endif
  // in manager.c a sanity check ensures that if (info->prepend) then !(info->in_place)
  // Otherwise copying fixed header to new file or in place, depending on option

  /* try to seek right offset depending on type of file */

  if (info->in_place && globals.debugging)
  {
     if (globals.debugging) foutput("%s\n", INF "Overwriting header...");
  }


  count = fwrite(header->header_out,
                 header->header_size_out,
                 1,
                 info->outfile.fp);

  fclose(info->outfile.fp);

  if (count != 1)
  {
    if (globals.debugging) foutput("%s\n", ERR "Error updating wav file.");
    return(FAIL);
  }

  if (errno)
  {
    perror("\n"ERR "Error in launch repair module\n");
    return(FAIL);
  }

  return(COPY_SUCCESS);
}
