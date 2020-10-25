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



int readHeader(FILE * infile, WaveHeader *header, globalData *globals)
{

  if (infile == NULL)  return 0;

  size_t        count;

  /* read in the HEADER_SIZE byte RIFF header from file */


  uint8_t *p;
  uint8_t buffer[header->header_size_in];
  memset(buffer, 0, header->header_size_in);
  p=buffer;

  fseek(infile, 0, SEEK_SET);

  count=fread(buffer, 1, header->header_size_in, infile ) ;
  /* Total is 44 bytes */

  if  (count != header->header_size_in)
    {
      if (globals->debugging) foutput(ERR "Failed to read header from input file\n       Size is: %d, read: %" PRIu64 " bytes\n", header->header_size_in, count );

      return(FAIL);
    }


#if 0

  Remember the structure of a WAVE_FORMAT_PCM header is:
  (standard header)
  uint8_t header[44]=
  {'R','I','F','F',    //   0 - ChunkID
    0,0,0,0,            //  4 - ChunkSize (filesize - 8 - padbyte)
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
    0,0,0,0             // 40 - 43 Subchunk2Size = Chunksize - 36 = filesize - 44 - padbyte
  };

  Samples follow.
  Pad byte at EF if sample count is odd.

  or else (extended wav)
  N is the offset of "data" + 8.
  N = 46 (minimal extension, no fact), 58 (minimal extension, fact), 68 (long extension, no fact) or 80 (long extention, minimal fact) or 80 + x (long extension, non-standard tagged fact)

  uint8_t header[N]=
  {'R','I','F','F',    //  0 - ChunkID
    0,0,0,0,            //  4 - ChunkSize (filesize - 8 - padbyte)
    'W','A','V','E',    //  8 - Format
    'f','m','t',' ',    // 12 - SubChunkID
    40,0,0,0,           // 16 - SubChunk1ID  // 18 or 40 for PCM as 16 is only for WAVE_FORMAT_PCM
    1,0,                // 20 - AudioFormat (1=16-bit)
    2,0,                // 22 - NumChannels
    0,0,0,0,            // 24 - SampleRate in Hz
    0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
    4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
    16,0,               // 34 - BitsPerSample
    22,0,               // 36 - wav extension  (0 or 22 bytes)
    if not 0:
    0,0,                // 38 - number of valid bits
    0,0,0,0,            // 40 - speaker position mask (dwChannelMmask)
    [16 B]              // 44 - GUID including WAV_FORMAT_PCM or WAV_FORMAT_EXTENSIBLE
    'f','a','c','t',    // 60 - fact chunk, optional for PCM here minimum)
    0,0,0,4,            // 64 - net length of fact chunk
    0,0,0,0,            // 68 - number of samples written (uint32_t)
   // some software pack up various tags in here... + x bytes
    'd','a','t','a',    // 72 + x - Sunchunk2IDO
    0,0,0,0             // 76 + x - 80 Subchunk2Size = filesize - padbyte - N
  };
  Samples follow.
  Pad byte at EF if sample count is odd.
#endif

/* wavwritehdr:  write .wav headers as follows:

bytes      variable      description
0  - 3     'RIFF'/'RIFX' Little/Big-endian
4  - 7     wRiffLength   length of file minus the 8 byte riff header
8  - 11    'WAVE'
12 - 15    'fmt '
16 - 19    wFmtSize       length of format chunk minus 8 byte header
20 - 21    wFormatTag     identifies PCM, ULAW etc
22 - 23    wChannels
24 - 27    dwSamplesPerSecond  samples per second per channel
28 - 31    dwAvgBytesPerSec    non-trivial for compressed formats
32 - 33    wBlockAlign         basic block size
34 - 35    wBitsPerSample      non-trivial for compressed formats

PCM formats then go straight to the data chunk:
36 - 39    'data'
40 - 43     dwDataLength   length of data chunk minus 8 byte header
44 - (dwDataLength + 43)   the data
(+ a padding byte if dwDataLength is odd)

Extended Wav:

36 - 37    wExtSize = 0  the length of the format extension
38 - 41    'fact'
42 - 45    dwFactSize = 4  length of the fact chunk minus 8 byte header
46 - 49    dwSamplesWritten   actual number of samples written out
50 - 53    'data'
54 - 57     dwDataLength  length of data chunk minus 8 byte header
58 - (dwDataLength + 57)  the data
(+ a padding byte if dwDataLength is odd)
*/

#define READ_4_bytes uint32_read_reverse(p), p+=4;
#define READ_2_bytes uint16_read_reverse(p), p+=2;

  /* RIFF chunk */
/* 0-3 */   header->ckID    =READ_4_bytes
/* 4-7 */   header->ckSize  =READ_4_bytes
/* 8-11 */  header->WAVEID=READ_4_bytes

/* FORMAT chunk */
/* 12-15 */ header->fmt_ckID= READ_4_bytes
/* 16-19 */ header->fmt_ckSize  = READ_4_bytes
/* 20-21 */ header->wFormatTag= READ_2_bytes
/* 22-23 */ header->channels = READ_2_bytes
/* 24-27 */ header->dwSamplesPerSec= READ_4_bytes
/* 28-31 */ header->nAvgBytesPerSec=READ_4_bytes
/* 32-33 */ header->nBlockAlign=READ_2_bytes
/* 34-35 */ header->wBitsPerSample =READ_2_bytes

/* We diagnosed for WAV_FORMAT_EXTENSIBLE earlier */

if (header->is_extensible)
{
/* 36-37 +22*/ header->cbSize =READ_2_bytes

/* skipping extension subchunk */
 p +=2;
 header->dwChannelMask = READ_4_bytes
 p += header->cbSize - 6; // 0 or 22
}

/* fact and data chunks were previouslt parsed */

if (header->has_fact)
{
/* 38-41 +22*/ p += 4;


    /* 42-45 +22*/ header->fact_length=READ_4_bytes
    /* 46-49 +22*/ header->n_spl=READ_4_bytes
    // loop for 'data' chunk
}

/* 40-43 or 54-57 +22*/ header->data_cksize= uint32_read_reverse(buffer + header->header_size_in - 4);

  /* point to beginning of file */
  rewind(infile);

  /* and dump the header */
  if (globals->veryverbose)
  {
      if (globals->debugging) foutput( "%s\n", MSG_TAG "Existing header data.\n" INF "Looking for the words 'RIFF', 'WAVE', 'fmt'," );
      if (globals->debugging) foutput( "%s\n", "       or 'data' to see if this is even a somewhat valid WAVE header:" );
      hexdump_header(infile, header->header_size_in);
  }

  return 1;
}




