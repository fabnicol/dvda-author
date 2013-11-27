/*****************************************************************
*   readHeader.c
*   Copyright Fabrice Nicol 2008
*   Description: reads headers.
*   Rewrite of Pigiron's 2007 originalwork to accommodate
*   big-endian platforms.
*
*******************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include "fixwav.h"
#include "fixwav_auxiliary.h"
#include "readHeader.h"
#include "fixwav_manager.h"
#include "c_utils.h"
#include "structures.h"
#include "libiberty.h"

extern globalData globals;

int readHeader(FILE * infile, WaveHeader *header)
{
  size_t        count;

  /* read in the HEADER_SIZE byte RIFF header from file */


  uint8_t *p;
  uint8_t temp[header->header_size];
  memset(temp, 0, header->header_size);
  p=temp;
  /* Patch against Pigiron's original work: code was not portable across compilers (structure header was
   * not necessarily packed, unless using gcc's __attribute((packed))__ or similar. Also, using code
   * was not portable to big-endian machines, considering wav headers are packed in little-endian order.
   * This version  fixes both issues */

  rewind(infile);
  count=fread(temp, header->header_size, 1, infile ) ;
  /* Total is 44 bytes */

  if ( count != 1)
    {
      fprintf( stderr, ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Failed to read header from input file\n       Size is: %d, read: %d bytes\n", header->header_size, count );

      return(FAIL);
    }


#if 0

  // Remember the structure of a header is:

  {'R','I','F','F',    //  0 - ChunkID
    0,0,0,0,            //  4 - ChunkSize (filesize-8)
    'W','A','V','E',    //  8 - Format
    'f','m','t',' ',    // 12 - SubChunkID
    16,0,0,0,           // 16 - SubChunk1ID  // 16 for PCM
    1,0,                // 20 - AudioFormat (1=16-bit)
    2,0,                // 22 - NumChannels
    0,0,0,0,            // 24 - SampleRate in Hz
    0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
    4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
    16,0,               // 34 - BitsPerSample
    'd','a','t','a',    // 36 - Subchunk2ID
    0,0,0,0             // 40 - Subchunk2Size
  };

#endif


#define READ_4_bytes uint32_read_reverse(p), p+=4;
#define READ_2_bytes uint16_read_reverse(p), p+=2;

  /* RIFF chunk */
/* 0-3 */ header->chunk_id    =READ_4_bytes
/* 4-7 */   header->chunk_size  =READ_4_bytes
/* 8-11 */  header->chunk_format=READ_4_bytes

/* FORMAT chunk */
/* 12-15 */ header->sub_chunk= READ_4_bytes
/* 16-19 */ header->sc_size  = READ_4_bytes
/* 20-21 */ header->sc_format= READ_2_bytes
/* 22-23 */ header->channels = READ_2_bytes
/* 24-27 */ header->sample_fq= READ_4_bytes
/* 28-31 */ header->byte_p_sec=READ_4_bytes
/* 32-33 */ header->byte_p_spl=READ_2_bytes
/* 34-35 */ header->bit_p_spl =READ_2_bytes

/* DATA chunk */
/* 36-39 */ header->data_chunk=READ_4_bytes
/* 40-43 */ header->data_size= uint32_read_reverse(p);


  /* point to beginning of file */
  rewind(infile);

  /* and dump the header */
  printf( "%s\n", ANSI_COLOR_GREEN"[MSG]"ANSI_COLOR_RESET"  Existing header data.\n"ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Looking for the words 'RIFF', 'WAVE', 'fmt'," );
  printf( "%s\n", "       or 'data' to see if this is even a somewhat valid WAVE header:" );
  hexdump_header(infile, header->header_size);



  return 1;
}




