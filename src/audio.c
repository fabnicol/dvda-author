/*

File:    audio.c
Purpose: Deal with reading the input audio files.

dvda-author  - Author a DVD-Audio DVD

Copyright Dave Chapman <dave@dchapman.com> 2005 ;
2008 revisions:
Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> 2008 (SoX and FLAC 1.2.1 integration, code refactoring);
          Lee and Tim Feldkamp 2008 (multichannel features);

The latest version can be found at http://dvd-audio.sourceforge.net

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __GNU_LIBRARY__
#include <unistd.h>
#endif
#include "export.h"
#include "audio2.h"
#include "audio.h"
#include "stream_decoder.h"
#include "fixwav.h"
#include "fixwav_manager.h"

#include "auxiliary.h"
#include "commonvars.h"
#include "command_line_parsing.h"
#include "winport.h"
#ifndef WITHOUT_sox
#include "sox.h"
#include "libsoxconvert.h"
#endif
#include "multichannel.h"
#include "file_input_parsing.h"


extern globalData globals;

/*
*
* Multichannel reference tables are structured as follows:
*  { { 16-bit  { channels { permutated values }}}, { 24-bit  { channels { permutated values }}} }
*
* S is the table of direct conversion (WAV to AOB) and _S the table of reverse conversion (AOB to WAV).
*
* Documentation on how to obtain the following permutation tables and relation to channel-mapping: see file wav-aob.mappings.ods (Aug. 2016, Fabrice Nicol)
*
*/

/*
ID\Chan 0   1   2   3   	4   5     info->channels
    0 	C                              1
    1 	L 	R                          2
    2 	L 	R 	S                      3
    3 	L 	R 	Ls 	Rs                 4
    4 	L 	R 	Lfe                    3
    5 	L 	R 	Lfe	S                  4
    6 	L 	R 	Lfe	Ls  	Rs         5
    7 	L 	R 	C                      3
    8 	L 	R 	C 	S                  4
    9 	L 	R 	C 	Ls 	    Rs         5
    10 	L 	R 	C 	Lfe                4
    11 	L 	R 	C 	Lfe 	S          5
    12 	L 	R 	C 	Lfe 	Ls 	Rs     6
    13 	L 	R 	C 	S                  4
    14 	L 	R 	C 	Ls 	    Rs         5
    15 	L 	R 	C 	Lfe                4
    16 	L 	R 	C 	Lfe 	S          5
    17 	L 	R 	C 	Lfe 	Ls 	Rs     6
    18 	L 	R 	Ls 	Rs   	Lfe        5
    19 	L 	R 	Ls 	Rs 	    C          5
    20 	L 	R 	Ls 	Rs 	    C  	Lfe    6
*/

const uint8_t channels[21] = {1,2,3,4,3,4,5,3,4,5,4,5,6,4,5,4,5,6,5,5,6};

// number of channels x sampling rate in Hz x bit rate = bandwidth <= 9,6 Mbps
// Each permutation table has length : 2 x sample size (nchannels x nbitspersample / 8 e.g. 2 x 6 x 24 / 8 = 36 for 6-ch. 24-bit audio, whatever the sample rate)
// So audio input must come in pair values of samples.

static const uint8_t  S[2][6][36]=
{   {   {0}, // 4
        {0}, // 8
        {5, 4, 11, 10, 1, 0, 3, 2, 7, 6, 9, 8}, //12 OK for all sampling rates <= 96 kHz
        {5, 4, 7, 6, 13, 12, 15, 14, 1,  0, 3, 2, 9, 8, 11, 10}, // 16 OK for all sampling rates <= 96 kHz
        {7, 6, 9, 8, 17, 16, 19, 18, 1, 0, 3, 2, 5, 4, 11, 10, 13, 12, 15, 14}, //20 OK for all sampling rates <= 96 kHz
        {7, 6, 9, 8, 11, 10, 19, 18, 21, 20, 23, 22, 1, 0, 3, 2, 5, 4, 13, 12, 15, 14, 17, 16} //24  // OK for all sampling rates <= 96 kHz
    },
    {   {2,  1,  5,  4,  0,  3}, //6  OK for all sampling rates <= 96 kHz
        {2, 1, 5, 4, 8, 7, 11, 10, 0, 3, 6, 9}, //12 OK for all sampling rates <= 96 kHz
        {8, 7, 17, 16, 6, 15, 2, 1, 5, 4, 11, 10, 14, 13, 0, 3, 9, 12}, //18 OK for all sampling rates <= 96 kHz
        {8,  7,  11,  10,  20,  19,  23,  22,  6,  9,  18,  21,  2,  1,  5,  4,  14,  13,  17,  16,  0,  3,  12,  15}, //24 OK for all sampling rates <= 96 kHz
        {11, 10, 14, 13, 26, 25, 29, 28, 9, 12, 24, 27, 2, 1, 5, 4, 8, 7, 17, 16, 20, 19, 23, 22, 0, 3, 6, 15, 18, 21},  // 30, out for 88.1 kHz and 96 kHz.
        {8, 7, 11, 10, 26, 25, 29, 28, 6, 9, 24, 27, 2, 1, 5, 4, 14, 13, 17, 16, 20, 19, 23, 22, 32, 31, 35, 34, 0, 3, 12, 15, 18, 21, 30, 33 }  // 36, out for 88.1 kHz and 96 kHz.
    }
};



// Direct conversion table when separate mono channels are given on command line

#if 0
static uint8_t  C[2][7][36][2]=
{
    {
     //3:1 3:0|4:1 4:0|3:3 3:2|4:3 4:2 |1:1 1:0|2:1 2:0 |5:1  5:0|6:1  6:0|1:3  1:2| 2:3 2:2|5:3 5:2 | 6:3 6:2



    },
 //24-bit:
 //   per output 0-based channel number: {1-based input channel number, 0-based byte rank}
    {
        {{1,1}, {1,0}},

        {{1,1}, {1,0}, {2,1}, {2,0}},

        {{1,2}, {1,1}, {2,2}, {2,1}, {1,5}, {1,4}, {2,5}, {2,4},
         {1,0}, {2,0}, {1,3}, {2,3}},

        {{3,2}, {3,1}, {3,5}, {3,4}, {3,0}, {3,3}, {1,2}, {1,1},
         {2,2}, {2,1}, {1,5}, {1,4}, {2,5}, {2,4}, {1,0}, {2,0},
         {1,3}, {2,3}},

        {{3,2}, {3,1}, {4,2}, {4,1}, {3,5}, {3,4}, {4,5}, {4,4},
         {3,0}, {4,0}, {3,3}, {4,3}, {1,2}, {1,1}, {2,2}, {3,1},
         {1,5}, {1,4}, {2,5}, {2,4}, {1,0}, {2,0}, {1,3}, {2,3}},

        {{1,2}, {3,1}, {4,2}, {4,1}, {5,2}, {5,1}, {3,5}, {3,4},
         {4,5}, {4,4}, {5,5}, {5,4}, {3,0}, {4,0}, {5,0}, {3,3},
         {4,3}, {5,3}, {1,2}, {1,1}, {2,2}, {2,1}, {1,5}, {1,4},
         {2,5}, {2,5}, {1,0}, {2,0}, {1,3}, {2,3}},

        {{3,2}, {3,1}, {4,2}, {4,1}, {3,5}, {3,4}, {4,5}, {4,4},
         {3,0}, {4,0}, {3,3}, {4,3}, {1,2}, {1,1}, {2,2}, {2,1},
         {5,2}, {5,1}, {6,2}, {6,1}, {1,5}, {1,4}, {2,5}, {2,4},
         {5,5}, {5,4}, {6,5}, {6,4}, {1,0}, {2,0}, {5,0}, {6,0},
         {1,3}, {2,3}, {5,3}, {6,3}}
    }
};
#endif

#ifndef WITHOUT_FLAC

void flac_metadata_callback (const FLAC__StreamDecoder GCC_UNUSED *dec, const FLAC__StreamMetadata *meta, void *data)
{

    fileinfo_t *info = (fileinfo_t *) data;

    if (meta->type==FLAC__METADATA_TYPE_STREAMINFO)
    {
        info->bitspersample=(uint8_t) meta->data.stream_info.bits_per_sample;
        info->samplerate=meta->data.stream_info.sample_rate;
        info->channels=(uint8_t) meta->data.stream_info.channels;
        info->numsamples=meta->data.stream_info.total_samples;
    }
}

FLAC__StreamDecoderWriteStatus flac_null_write_callback(const FLAC__StreamDecoder GCC_UNUSED *dec,
        const FLAC__Frame GCC_UNUSED *frame,
        const FLAC__int32 GCC_UNUSED * const buf[],
        void GCC_UNUSED *data)
{
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus flac_write_callback(const FLAC__StreamDecoder GCC_UNUSED *dec,
        const FLAC__Frame GCC_UNUSED *frame,
        const FLAC__int32 GCC_UNUSED * const buf[],
        void GCC_UNUSED *data)
{
    fileinfo_t *info = (fileinfo_t*) data;

    unsigned int c_samp, c_chan, d_samp;
    uint32_t data_size = frame->header.blocksize * frame->header.channels * (info->bitspersample / 8);
    uint32_t samples = frame->header.blocksize;
    uint32_t i;

    if ((info->audio->n+data_size) > sizeof(info->audio->buf))
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Internal error - FLAC buffer overflown")
    }


        // Store data in interim buffer in WAV format - i.e. Little-endian interleaved samples
        i=info->audio->n;
    for (c_samp = d_samp = 0; c_samp < samples; c_samp++)
    {
        for (c_chan = 0; c_chan < frame->header.channels; c_chan++, d_samp++)
        {
            info->audio->buf[i++]=(buf[c_chan][c_samp]&0xff);
            info->audio->buf[i++]=(buf[c_chan][c_samp]&0xff00)>>8;
            if (info->bitspersample==24)
            {
                info->audio->buf[i++]=(buf[c_chan][c_samp]&0xff0000)>>16;
            }
        }
    }
    info->audio->n=i;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



void flac_error_callback(const FLAC__StreamDecoder GCC_UNUSED *dec,
                         FLAC__StreamDecoderErrorStatus GCC_UNUSED status, void GCC_UNUSED *data)
{
    foutput("%s", ERR "FLAC error callback called.\n");
}

#endif

int calc_info(fileinfo_t* info)
{
//PATCH: provided for null dividers.
    if (info->samplerate == 0 || info->channels == 0 || info->channels == 0)
    {
        foutput("%s%s%s\n", ANSI_COLOR_RED"\n[ERR] ", info->filename, ANSI_COLOR_RESET"  Null audio characteristics");
        return(NO_AFMT_FOUND);
    }

// assemble numbers for the various combinations
    short int table_index=(info->bitspersample == 24)? 1 : 0 ;

    static const uint16_t T[2][6][11]=     // 16-bit table
    {
         {{ 	2000, 16,  1984,  2010,	2028, 22, 11, 16, 16 /*old: 10*/, 0, 0 },
            {	2000, 16,  1984,  2010,	2028, 28 /*old: 22*/ , 11, 16, 10 /*old: 16*/, 0, 0 },
            { 	2004, 24,  1980,  2010,	2028, 24 /*old: 22*/, 15, 12, 10 /*old: 12*/, 0, 0 },
            { 	2000, 16,  1980,  2010,	2028, 28 /*old: 22*/, 11, 16, 10 /*old: 16*/, 0, 0 },
            { 	2000, 20,  1980,  2010, 2028, 22, 15, 16, 10 /*old: 16*/, 0, 0 },
            { 	1992, 24,  1992, 1993,  2014, 22, 10, 10, 10 /*old: 4*/, 17, 14}},
        // 24-bit table
        {{    	2004, 24,  1980,  2010,	2028, 22, 15, 12, 12 /*old: 10*/, 0, 0 },
            { 	2004, 24,  1980,  2010,	2028, 24 /*old: 22*/, 15, 12, 10 /*old: 12*/, 0, 0 },
            { 	1998, 18,  1980,  2010,	2020 /*old 2026*/, 28 /*old: 22 */, 15, 10 /* old 16 */, 10 /* old 16*/, 0, 8 /* old 6 */ },
            { 	1992, 24,  1968,  1993,	2014, 22, 10, 10,  10 /*old: 8*/, 17, 14 },
            { 	1980,  0,  1980,  2010, 2002 /* old 2008 */, 22, 15, 10 /* old 16 */, 10 /*old: 16*/, 0, 26 /* old 20 */},  // out for 88.1 kHz and 96 kHz.
            { 	1980,  0,  1980,  2010, 2008, 22, 15, 16, 16 /* old 14*/, 0, 20 }}  // out for 88.1 kHz and 96 kHz.
    };

//
// The following equations are always true by necessity:
//    firstpack_lpcm_headerquantity + firstpack_pes_padding + payload - firstpackdecrement = 1995
//    midpack_lpcm_headerquantity   + payload + midpack_pes_padding = 2016
//    first/mid_pes_padding > 6
//

    info->sampleunitsize = 2 * info->channels * info->bitspersample / 8;

//
// Revision of sampleunitsize (8 July 2019) may have to do with some gapless issues.
// Previous code was:
//  info->sampleunitsize = (table_index == 1)? info->channels * 6 :
//                                    ((info->channels > 2)? info->channels * 4 :
//                                     info->channels * 2);

#define X T[table_index][info->channels-1]

    info->lpcm_payload = X[0];
    info->firstpackdecrement = (uint8_t) X[1];

    info->SCRquantity = X[2];
    info->firstpack_audiopesheaderquantity = X[3];
    info->midpack_audiopesheaderquantity   = X[4];
    info->lastpack_audiopesheaderquantity  = X[5];
    info->firstpack_lpcm_headerquantity    = (uint8_t) X[6];
    info->midpack_lpcm_headerquantity      = (uint8_t) X[7];
    info->lastpack_lpcm_headerquantity     = (uint8_t) X[8];
    info->firstpack_pes_padding            = (uint8_t) X[9];
    info->midpack_pes_padding              = (uint8_t) X[10];

#undef X

    info->bytespersecond = (info->samplerate * info->bitspersample * info->channels)/8;

    switch (info->samplerate)
    {
    case 44100:
    case 48000:
        info->bytesperframe = 5;
        break;
    case 88200:
    case 96000:
        info->bytesperframe = 10;
        break;

    case 176400:
    case 192000:
        info->bytesperframe = 20;
        break;

    }

    info->bytesperframe *= info->channels * info->bitspersample;

    info->numsamples
            = (info->numbytes * 8) / (info->channels * info->bitspersample);

    info->PTS_length = (90000.0 * info->numsamples) / info->samplerate;  // = duration in seconds x 90000

    // a rounding error of 1 in PTS_LENGTH = samplerate / 90000 in numsamples = samplerate / (8 x 90000) x nchannels x bitspersample in bytes.

    return(AFMT_WAVE);
}


// This function cleans up the list of input files by filtering out non-compliant audio input (format undefined or impossible to correct by other options)
// Group and track numbers are readjusted and shifted down (recursively) when one or more files are rejected

command_t *scan_audiofile_characteristics(command_t *command)
{
    short int  l, delta=0;
    int error=0;
    static uint8_t i,j;

    // retrieving information as to sound file format

    error = audiofile_getinfo(&command->files[i][j]);

    // dealing with format information

    switch (error)
    {
        case AFMT_WAVE:
            if (globals.debugging) foutput(MSG_TAG "Found WAVE format for %s\n", command->files[i][j].filename);
            command->files[i][j].type=AFMT_WAVE;
            j++;
            break;

        case AFMT_WAVE_FIXED:
            if (globals.debugging) foutput(MSG_TAG "Found WAVE format (fixed) for %s\n", command->files[i][j].filename);
            command->files[i][j].type=AFMT_WAVE;
            j++;
            break;

        case AFMT_WAVE_GOOD_HEADER:
            if (globals.debugging) foutput(MSG_TAG "Found WAVE format (original) for %s\n", command->files[i][j].filename);
            command->files[i][j].type=AFMT_WAVE;
            j++;
            break;

        case AFMT_MLP:
            if (globals.debugging) foutput(MSG_TAG "Found MLP format for %s\n", command->files[i][j].filename);
            command->files[i][j].type = AFMT_MLP;
            j++;
            break;

    #ifndef WITHOUT_FLAC
        case AFMT_FLAC:
            if (globals.debugging) foutput(MSG_TAG "Found FLAC format for %s\n", command->files[i][j].filename);
            error=flac_getinfo(&command->files[i][j]);
            j++;
            break;

    #if !defined WITHOUT_libogg
    #if defined HAVE_OGG_FLAC && HAVE_OGG_FLAC == 1
        case AFMT_OGG_FLAC:
            if (globals.debugging) foutput(MSG_TAG "Found Ogg FLAC format for %s\n", command->files[i][j].filename);
            error=flac_getinfo(&command->files[i][j]);
            j++;
            break;
    #endif
    #endif
    #endif

        case NO_AFMT_FOUND:
            if (globals.debugging) foutput(ERR "No compatible format was found for %s\n       Skipping file...\n", command->files[i][j].filename);

            // House-cleaning rules: getting rid of files with unknown format

            // taking off one track;

            command->ntracks[i]--;

            // group demotion: if there is no track left in groups, taking off one group

            if (command->ntracks[i] == 0)
            {
                // taking off one group

                command->ngroups--;

                // getting out of both loops, check end on inner=end of outer
                if (i == command->ngroups-command->nvideolinking_groups)
                  {
                      if (i) return(command);
                      else exit(EXIT_FAILURE);
                  }

                // shifting indices for ntracks: all groups have indices decremented, so ntracks[g+1] is now ntracks[g]

                for (l=i; l < command->ngroups-command->nvideolinking_groups; l++)
                {
                    command->ntracks[l]=command->ntracks[l+1];
                    if (globals.debugging)
                        foutput(INF "Shifting track count for group=%d->%d\n", l+1, l+2);
                }
                // delta is a flag for group demotion
                delta=1;


            }
            // shifting indices for files (two cases: delta=0 for same group track-shifting, delta=1 for group demotion

            for (l=j; l < command->ntracks[i+delta]; l++)
            {
                // a recursion is unavoidable save for j=last track in group
                int i_shift=i+delta;
                int l_shift=l+1-delta;
                if (globals.debugging)
                    foutput(INF "Shifting indices for group=%d->%d, track=%d->%d\n", i+1, i_shift+1, l+1, l_shift+1);

                command->files[i][l]=command->files[i_shift][l_shift];
            }
            break;
    }

// assigning channel
// if AFMT was found, j will have been incremented earlier
// otherwise it is necessary to reparse again files[i][j] as indices have been shifted

   if (error != NO_AFMT_FOUND && (j))
         command->files[i][j-1].dvdv_compliant=
                  ((command->files[i][j-1].bitspersample == 16  || command->files[i][j-1].bitspersample == 24)
                 &&(command->files[i][j-1].samplerate  == 96000 || command->files[i][j-1].samplerate  == 48000));


    bool increment_group=(j == command->ntracks[i]);

    j *= 1-increment_group;
    i += increment_group;

    if (i == command->ngroups-command->nvideolinking_groups)

        return command;
// recursion
    if (command->files[i][j].filename) scan_audiofile_characteristics(command);
    return(command);
}

uint8_t extract_audio_info(fileinfo_t *info)
{
    info->type=AFMT_WAVE;

    /* parsing header again with FIXWAV utility */

    //static bool cut;

    //if (!cut)
        info->type=fixwav_repair(info);

    //cut=((info->type == AFMT_WAVE_FIXED) || (info->type == AFMT_WAVE_GOOD_HEADER));

    if (calc_info(info) == NO_AFMT_FOUND)
    {
      return(info->type=NO_AFMT_FOUND);
    }

    return(info->type);
}


static inline int compute_header_size(FILE* fp)
{
    WaveHeader header;
    memset(&header, 0, sizeof(header));
    //uint8_t span=0;
    WaveData wavinfo =
    {

      .database = NULL,
      .filetitle = NULL,
      .automatic = true,
      .prepend = false,
      .in_place = true,
      .cautious = false,
      .interactive = false,
      .padding = false,
      .prune = false,
      .virtual = true,
      .repair = 0,
      .padbytes = 0,
      .prunedbytes = 0,
      .infile = { false, 0, "unknown", NULL},
      .outfile = {false, 0, "st", NULL },
    };

    wavinfo.infile.fp = fp;
    parse_wav_header(&wavinfo, &header);
    return header.header_size_in;
}


static inline int extract_audio_info_by_all_means(char* path, uint8_t* header, fileinfo_t* info)
{

    if (header[4] == 0xF8 && header[5] == 0x72 && header[6] == 0x6F && header[7] == 0xBB)
    {
        unsigned long pathl = strlen(path);
        if (pathl > 3)
        {
         if (path[pathl - 1] == 'p'
             && path[pathl - 2] == 'l'
             && path[pathl - 3] == 'm'
             && path[pathl - 4] == '.')
         {
             foutput("%s %s\n", INF "Auditing MLP file", path);
         }
         else
         {
              if (globals.veryverbose)
              {
                  foutput("%s %s %s\n", WAR "Audio file", path, "has MLP major sync header yet no file extenion .mlp. Proceeding as if MLP...");
              }
         }

        }
        else
        {
            if (globals.veryverbose)
            {
                foutput("%s %s %s\n", WAR "Audio file", path, "has MLP major sync header yet no file extenion .mlp. Proceeding as if MLP...");
            }
        }

        switch (header[8])
        {
            case 0x00:
            case 0x0f:
                info->bitspersample = 16;
            break;

            case 0x11:
            case 0x1f:
                info->bitspersample = 20;
                break;

            case 0x22:
            case 0x2f:
                info->bitspersample = 24;
                break;

            default:
                break;
        }

        switch (header[9])
        {
            case 0x00:
            case 0x0f:
                info->samplerate = 48000;
                break;

            case 0x11:
            case 0x1f:
                info->samplerate = 96000;
                break;

            case 0x22:
            case 0x2f:
                info->samplerate = 192000;
                break;

            case 0x88:
            case 0x8f:
                info->samplerate = 44100;
                break;

            case 0x99:
            case 0x9f:
                info->samplerate = 88200;
                break;

            case 0xaa:
            case 0xaf:
                info->samplerate = 176400;
                break;

            default:
                break;
        }

        info->cga =  check_cga_assignment(header[11]);
        info->channels = cga_to_channel(info->cga);
        if (info->cga == 0xFF)
        {
            foutput(ERR "Could not detect number of channels for MLP file %s with channel group assignment: %d\n", info->filename, header[11]);
        }
        else
        if (info->channels == 0)
        {
            foutput(ERR "Could not detect number of channels for MLP file %s with channel group assignment: %d (%s)\n", info->filename, info->cga, cga_define[info->cga]);
        }

        return(info->type = AFMT_MLP);
    }
    else
    if (memcmp(header,"RIFF",4) != 0 || memcmp(&header[8],"WAVEfmt",7) != 0)
    {
#      ifndef WITHOUT_FLAC

        // Other formats than WAV: parsing headers

        if (memcmp(header,"fLaC",4) == 0 )
            return(info->type = AFMT_FLAC);

        if ((memcmp(header,"OggS",4) == 0 ) && (memcmp(header+0x17, "FLAC", 4) != 0))
            return(info->type = AFMT_OGG_FLAC);

#      endif
#      ifndef WITHOUT_sox

            if (globals.sox_enable)
            {
                // When RIFF fmt headers are not recognized, they are processed by Sox first if -S -F is on command line then checked by fixwav
                // yest SoX may crash for seriously mangled headers

                if (!globals.fixwav_force)
                {

                    if (launch_sox(&path) == NO_AFMT_FOUND)
                       return(info->type);
                      // It is necessary to reassign info->file_size as conversion may have marginal effects on size (due to headers/meta-info)
                    else
                      // PATCH looping back to get info
                    {
                       info->filename = path;

                       return(info->type = audiofile_getinfo(info));
                    }
          // yet without the processing tail below (preserving new header[] array and info structure)

                }
                else
                {
                 // Other way round if -S -Fforce, as fixwav processes first before SoX
                  info->type = extract_audio_info(info);

                  switch(info->type)
                  {
                     case AFMT_WAVE_GOOD_HEADER :
                     case AFMT_WAVE_FIXED :
                        return (info->type=AFMT_WAVE);

                     default:
                       // PATCH looping back to get info

                        if (launch_sox(&path) == NO_AFMT_FOUND)
                        return(info->type);
                      // It is necessary to reassign info->file_size as conversion may have marginal effects on size (due to headers/meta-info)
                        else
                      // PATCH looping back to get info
                        {
                            if (info->audio) free(info->audio);
                            return(info->type = audiofile_getinfo(info));
                        }
                          // yet without the processing tail below (preserving new header[] array and info structure)
                  }
               }
            }

#      endif

    }

  // BUGFIX: consequence of generalizing use of fixwav

  return(info->type = extract_audio_info(info));
}


static inline void clean_file(fileinfo_t* info, int u)
{
    FILE* fp = info->mergeflag ? info->audio->channel_fp[u] : info->audio->fp;
    fseek(fp, -10 * 2048, SEEK_END); // 10 sectors back
    char temp[10 * 2048] = {0};
    for (int c = 0; c < 10 * 2048; ++c)
    {
        temp[c] = getc(fp);
        if (temp[c] == 0) temp[c] = 0x20;
    }
    temp[10 * 2048 -1] = 0;

    char* ret = strstr(temp, "LIST");

    if (ret)
    {
        fseek(fp, 0, SEEK_END);
        off_t size = ftello(fp);
        fclose(fp);
        off_t l = strlen(ret);
        if (globals.debugging) foutput(INF "%s\n", "Untagging LIST chunks.");
        truncate(info->mergeflag ? info->given_channel[u] : info->filename, size - l - 1);
        if (info->mergeflag)
            info->audio->channel_fp[u] = fopen(info->given_channel[u], "rb");
        else
            info->audio->fp = fopen(info->filename, "rb");
    }
    else
    {
        fclose(fp);
        if (globals.debugging) foutput(INF "%s\n", "No LIST chunks.");
        if (info->mergeflag)
            info->audio->channel_fp[u] = fopen(info->given_channel[u], "rb");
        else
            info->audio->fp = fopen(info->filename, "rb");
    }
}

static inline int process_audiofile_info(fileinfo_t* info)
{
// C99 needed

change_directory(globals.settings.workdir);
info->type = NO_AFMT_FOUND;

if (info->mergeflag)
{
     for (int u=0; u < info->channels; u++)
     {
      info->audio->channel_fp[u] = fopen(info->given_channel[u], "r+b");
      clean_file(info, u); // UNtagging

      if (globals.debugging) foutput(INF "Opening %s to get info\n", info->given_channel[u]);

      int span = compute_header_size(info->audio->channel_fp[u]);

      info->channel_header_size[u] = (span > 0) ? span + 8 : MAX_HEADER_SIZE;
      uint8_t header[info->channel_header_size[u]];
      memset(header, 0, info->channel_header_size[u]);

      /* PATCH: real size on disc is needed */

#    if defined __WIN32__
        info->file_size = read_file_size(info->audio->channel_fp[u],(TCHAR*) info->given_channel[u]);
     #else
        info->file_size = read_file_size(info->audio->channel_fp[u], info->given_channel[u]);
     #endif

     //fread(header, info->channel_header_size[u],1,info->audio->channel_fp[u]);
     fseek(info->audio->channel_fp[u], 0, SEEK_SET);

     if (info->channel_header_size[u] > (span = fread(header, 1, info->channel_header_size[u], info->audio->channel_fp[u])))
     {
         foutput(ERR "Could not read header of size %d for channel %d, just read %d character(s)\n", info->channel_header_size[u],u+1, span);
         clean_exit(EXIT_FAILURE);
     }

      fclose(info->audio->channel_fp[u]);

      info->type = extract_audio_info_by_all_means(info->given_channel[u], header, info);
     }
}
else
{
  info->audio->fp = fopen(info->filename, "r+b");

  // clean_file(info, 0); // UNtagging

  if (info->audio->fp == NULL)
  {
      fprintf(stderr, ERR "Failed to open %s to get info\n", info->filename);
      fprintf(stderr, ERR "Currentdir name is : %s\n", fn_get_current_dir_name());
      perror("Impossible to open file");
      EXITING
  }

  if (globals.debugging) foutput(INF "Opening %s to get info\n", info->filename);

  info->header_size = MAX_HEADER_SIZE;
  uint8_t header[info->header_size];
  memset(header, 0, info->header_size);

  /* PATCH: real size on disc is needed */

  #if defined __WIN32__
        info->file_size = read_file_size(info->audio->fp,(TCHAR*) info->filename);
  #else
        info->file_size = read_file_size(info->audio->fp, info->filename);
  #endif

  fseek(info->audio->fp, 0, SEEK_SET);
  int span = 0;
  if (info->header_size > (span = fread(header, 1, info->header_size, info->audio->fp)))
  {
    foutput(ERR "Could not read header of size %d, just read %d character(s)\n", info->header_size, span);
    perror("       ");
    clean_exit(EXIT_FAILURE);
  }
  if (info->audio) free(info->audio);
  info->type = extract_audio_info_by_all_means(info->filename, header, info);
}

return (info->type);
}


static inline int wav_getinfo_merged(fileinfo_t *info)
{

    uint8_t nchannels=info->channels;
    uint8_t bitspersample=info->bitspersample;
    uint32_t samplerate=info->samplerate;
    uint64_t numbytes=info->numbytes;

    info->type=process_audiofile_info(info);

    if (info->channels != nchannels)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  At least one non-mono channel was given.")
    }

    info->bitspersample /= nchannels;

    if (info->bitspersample != bitspersample)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  At least one channel did not have the same bit depth as others.")
    }

    info->samplerate /= nchannels;

    if (info->samplerate != samplerate)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  At least one channel did not have the same sample rate as others.")
    }


    if (info->numbytes / nchannels != numbytes)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  At least one channel did not have the same number of bytes as others.")
    }

    // Audio characteristics retained are those of the multichannel file, after the above sanity tests.
    return(info->type);
}


int audiofile_getinfo(fileinfo_t* info)
{

    info->audio = (audio_input_t*) calloc(1, sizeof(audio_input_t));

    if (info->audio == NULL)
    {
      foutput("%s\n", ERR "Could not open audio file: filepath pointer is null");
      EXITING
    }

    if (info->mergeflag)
        wav_getinfo_merged(info);

    if (info->filename == NULL)
    {
      foutput("%s\n", ERR "Could not open audio file: filepath pointer is null");
      EXITING
    }
    else
    {
      if (globals.debugging) foutput(INF "Opening %s to get info\n", info->filename);
    }

    process_audiofile_info(info);

    return(info->type);
}

#ifndef WITHOUT_FLAC

int flac_getinfo(fileinfo_t* info)
{
    FLAC__StreamDecoder* flac;
    FLAC__StreamDecoderInitStatus  result=0;

    flac = FLAC__stream_decoder_new();

    if (flac!=NULL)
    {

        if (info->type == AFMT_FLAC )

            result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_file  	(
                        /*FLAC__StreamDecoder *  */ 	 flac,
                        /*FILE * */ 	info->filename,
                        /*FLAC__StreamDecoderWriteCallback */ 	flac_null_write_callback,
                        /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                        /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                        (void *) info
                    );

        else
        {
#ifdef FLAC_API_SUPPORTS_OGG_FLAC
#if (FLAC_API_SUPPORTS_OGG_FLAC == 1)
            result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_ogg_file  	(
                        /*FLAC__StreamDecoder *  */ 	 flac,
                        /*FILE * */ 	info->filename,
                        /*FLAC__StreamDecoderWriteCallback */ 	flac_null_write_callback,
                        /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                        /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                        (void *) info
                    );
#endif
#endif
        }

    }
    else
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Fatal error - could not create FLAC OR OGG FLAC decoder\n")
    }


    if (result!=FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {

        FLAC__stream_decoder_delete(flac);
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Failed to initialise FLAC decoder\n");
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(flac))
    {

        FLAC__stream_decoder_delete(flac);
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Failed to read metadata from FLAC file\n");
    }
    FLAC__stream_decoder_finish(flac);

    if (((info->bitspersample!=16) && (info->bitspersample!=24)) || (info->channels > 2))
    {
        return(1);
    }


    info->numbytes=info->numsamples*info->channels*(info->bitspersample/8);
    calc_info(info);
    FLAC__stream_decoder_delete(flac);
    return(0);
}
#endif

int fixwav_repair(fileinfo_t *info)
{
    static WaveHeader  waveheader;

    char temp[strlen(info->filename)-4+1];
    memset(temp, 0, sizeof(temp));

    if (temp == NULL)
    {
        foutput("%s\n", ERR "Could not allocate fixwav filename string");
        return(NO_AFMT_FOUND);
    }

    strncpy(temp, info->filename, strlen(info->filename)-4);

    char *outstring=print_time(0);
    short int memory_allocation=sizeof(temp)+5+strlen(outstring)+strlen(globals.fixwav_suffix)+4+1;

    if (memory_allocation> CHAR_BUFSIZ)
            foutput("%s\n", WAR "Shortening -o filename");
	memory_allocation=Min(memory_allocation, CHAR_BUFSIZ);
    char buf[memory_allocation];
    memset(buf, 0, memory_allocation);


    snprintf(&buf[0], memory_allocation, "%s%s%s%s", temp, globals.fixwav_suffix, outstring, ".wav");
    free(outstring);
    // If new string longer than heap allocation of reference strings, cut it
    /* Default sub-options*/

    WaveData wavedata=
    {
        .database = globals.settings.fixwav_database,  /* database path for collecting info chunks in headers */
        .filetitle = NULL,
        globals.fixwav_automatic, /* automatic behaviour */
        globals.fixwav_prepend, /* do not prepend a header */
        globals.fixwav_in_place, /* do not correct in place */
        globals.fixwav_cautious, /* whether to ask user about overwrite */
        globals.fixwav_interactive, /* interactive */
        globals.fixwav_padding, /* padding */
        globals.fixwav_prune, /* prune */
        globals.fixwav_virtual_enable, /* whether header should be fixed virtually */
        0  /* repair status */,
        0, /* padbytes */
        0, /* pruned bytes */
        .infile = {false, 0, info->filename, NULL},  /* filestat */
        .outfile = {false, 0, buf, NULL}
    };


    if (globals.debugging)
    {
        foutput(INF "Fixwav diagnosis for: %s\n", info->filename);

        SINGLE_DOTS
    }

    WaveHeader* res = fixwav(&wavedata, &waveheader);

    if (res == NULL )
    {

        if (globals.debugging) SINGLE_DOTS
        foutput("\n%s\n", INF "Fixwav repair was unsuccessful; file will be skipped.");

        return(NO_AFMT_FOUND);
    }
    else
    {
        if (globals.debugging) SINGLE_DOTS

        info->samplerate    = waveheader.dwSamplesPerSec;
        info->bitspersample = (uint8_t) waveheader.wBitsPerSample;
        info->channels      = (uint8_t) waveheader.channels;
        info->numbytes      = waveheader.data_cksize;
        info->file_size     = info->numbytes+waveheader.header_size_out;
        info->header_size   = waveheader.header_size_out;

        if (wavedata.repair == GOOD_HEADER)
        {
            foutput("%s", MSG_TAG "Proceeding with same file...\n");
            return(AFMT_WAVE_GOOD_HEADER);
        }
        else
        {
            if (! globals.fixwav_virtual_enable)
            {
                if (! wavedata.in_place)
                {
                    // info->filename is either allocated on the command-line heap itself (for free, with -g) or freshly allocated with -i
                    // with -g filenames it is not OK to free or realloc, see free_memory, one could just do info->filename=buf;
                    // tolerating here a modest memory loss in this case

                    info->filename=strdup(buf);
                }

                foutput(MSG_TAG "Proceeding with fixed file %s:\n", filename(wavedata.outfile));
            }
            else
            if (globals.debugging)
                foutput(MSG_TAG "Proceeding with virtual header and same file %s:\n", info->filename);

            foutput(MSG_TAG "Bits per sample=%d Sample frequency: %d\n       Bit depth:%d Channels:%d\n", waveheader.wBitsPerSample,
                    waveheader.dwSamplesPerSec, waveheader.wBitsPerSample, waveheader.channels );

            return(AFMT_WAVE_FIXED);
        }
    }
}


//#ifndef WITHOUT_sox

char* replace_file_extension(char * filename)
{
    int l=0,s=strlen(filename);

    do
        l++;
    while ((l < s-1) && (filename[s-l] != '.'));

    if (1 == s-l)
    {
        foutput("%s\n", ERR "To convert to WAV SoX.needs to indentify audio format and filename.\n       Use extension of type '.format'\n");
        if (globals.debugging)  foutput("%s\n", INF "Skipping file.\n ");
        return(NULL);
    }

// Requires C99
    int size=s-l;

    char new_wav_name[size+l+10];


    memcpy(new_wav_name, filename, size);
    memcpy(new_wav_name+size, "_sox_", 5);
    memcpy(new_wav_name+size+5, filename+size+1, l-1);
    memcpy(new_wav_name+size+l+4, ".wav", 4);
    new_wav_name[size+l+8]='\0';

    // Here, tolerating a memory leak for -g filenames.
    // -i filenames could in principle be freed.
    // filename cannot be altered directly as there the suffix increases its size and -g names cannot be reallocated

    filename=strdup(new_wav_name);

    return (filename);
}

#ifndef WITHOUT_sox
int launch_sox(char** filename)
{

    char* new_wav_name=replace_file_extension(*filename);

    if (new_wav_name == NULL)
    {
        perror(ERR "SoX string suffix was not allocted");
        return(NO_AFMT_FOUND);
    }

    if (globals.debugging)
        foutput("%s       %s -->\n       %s \n", MSG_TAG "Format is neither WAV nor FLAC\n"INF "Converting to WAV with SoX...\n", *filename, new_wav_name);

    unlink(new_wav_name);
    errno=0;
    if (soxconvert(*filename, new_wav_name) == 0)
    {
        if (globals.debugging)  foutput("%s\n", INF "File was converted.");

        *filename=new_wav_name;
         return(AFMT_WAVE);
    }
    if (globals.debugging)  foutput("%s\n", INF "SoX could not convert file.");

    free(new_wav_name);
    return(NO_AFMT_FOUND);

}
#endif

// to be used somewhere

void mono_channel_open(fileinfo_t* info)
{
    if (info->type != AFMT_WAVE) return;

    for (int u=0; u < info->channels; u++)
    {
        info->audio->channel_fp[u]=fopen(info->given_channel[u],"rb");
        if (info->audio->channel_fp[u]==NULL)
        {
            return;
        }
#ifdef __WIN32__
        info->channel_size[u] = read_file_size(info->audio->channel_fp[u], (TCHAR*) info->given_channel[u]);
#else
        info->channel_size[u] = read_file_size(info->audio->channel_fp[u], info->given_channel[u]);
#endif
        fseek(info->audio->channel_fp[u], info->header_size,SEEK_SET);

        info->audio->bytesread=0;
    }
}


int audio_open(fileinfo_t* info)
{
#ifndef WITHOUT_FLAC
    FLAC__StreamDecoderInitStatus result=0;
#endif

    info->audio=malloc(sizeof(audio_input_t));
    info->audio->n=0;
    info->audio->eos=0;


    if (info->type == AFMT_WAVE || info->type == AFMT_MLP)
    {
        if (info->mergeflag)
        {
            for (int u=0; u < info->channels; u++)
            {
                info->audio->channel_fp[u]=fopen(info->given_channel[u],"rb");
                if (info->audio->channel_fp[u]==NULL)
                {
                    return(1);
                }
        #ifdef __WIN32__
                info->file_size += read_file_size(info->audio->channel_fp[u], (TCHAR*) info->given_channel[u]);
        #else
                info->file_size += read_file_size(info->audio->channel_fp[u], info->given_channel[u]);
        #endif
                fseek(info->audio->channel_fp[u], info->channel_header_size[u],SEEK_SET);
            }

        }
        else
        {
            if (globals.debugging) foutput("%s %s\n", INF "Opening", info->filename);

            info->audio->fp = fopen(info->filename,"rb");

            if (info->audio->fp == NULL)
            {
                return(1);
            }

#           ifdef _WIN32
              info->file_size = read_file_size(info->audio->fp, (TCHAR*) info->filename);
#           else
              info->file_size = read_file_size(info->audio->fp, info->filename);
#           endif

            if (info->type == AFMT_MLP)
                fseek(info->audio->fp, info->header_size, SEEK_SET);
        }

        info->audio->bytesread=0;
    }
#ifndef WITHOUT_FLAC
    else
    {
        info->audio->flac=FLAC__stream_decoder_new();

        if (info->audio->flac!=NULL)
        {

            if  (info->type==AFMT_FLAC)
            {

                result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_file  	(
                            /*FLAC__StreamDecoder *  */ 	 info->audio->flac,
                            /*char * */ 	info->filename,
                            /*FLAC__StreamDecoderWriteCallback */ 	flac_write_callback,
                            /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                            /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                            (void *) info
                        );
                if ((globals.debugging) && (result == FLAC__STREAM_DECODER_INIT_STATUS_OK))
                    foutput("%s\n", MSG_TAG "FLAC decoder was initialized");
            }
            else

                if  (info->type==AFMT_OGG_FLAC)
                {

                    result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_ogg_file  	(
                                /*FLAC__StreamDecoder *  */ 	 info->audio->flac,
                                /*char * */ 	info->filename,
                                /*FLAC__StreamDecoderWriteCallback */ 	flac_write_callback,
                                /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                                /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                                (void *) info
                            );

                    if ((globals.debugging) && (result == FLAC__STREAM_DECODER_INIT_STATUS_OK))
                        foutput("%s\n", MSG_TAG "OGG_FLAC decoder was initialized");
                }
                else
                {
                    EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Type of file unknown")
                }



                    if (result!=FLAC__STREAM_DECODER_INIT_STATUS_OK)
                    {
                        FLAC__stream_decoder_delete(info->audio->flac);

                        /* error diagnosis */

                        if (globals.debugging)

                            switch (result)
                            {
                            case   FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER  :
                                foutput ("%s\n", ERR "The library was not compiled with support\n       for the given container format. ");
                                break;
                            case   FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS :
                                foutput("%s\n",  ERR "A required callback was not supplied.");
                                break;
                            case   FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR :
                                foutput("%s\n", ERR "An error occurred allocating memory.");
                                break;
                            case   FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE :
                                foutput("%s\n", ERR "fopen() failed in FLAC__stream_decoder_init_file()\n       or FLAC__stream_decoder_init_ogg_file(). ");
                                break;
                            case   FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED :
                                foutput("%s\n", ERR "FLAC__stream_decoder_init_*() was called when the decoder was already initialized,\n       usually because FLAC__stream_decoder_finish() was not called.");
                                break;
                            default :
                                foutput("%s\n", ERR "Error unknown by FLAC API.");
                            }

                        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Failed to initialise FLAC decoder\n");
                    }


            if (!FLAC__stream_decoder_process_until_end_of_metadata(info->audio->flac))
            {
                FLAC__stream_decoder_delete(info->audio->flac);
                EXIT_ON_RUNTIME_ERROR_VERBOSE( ERR "Failed to read metadata from FLAC file\n")

            }



        }
        else    EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Could not initialise FLAC decoder")

        }
#endif


    return(0);
}




/*
For the cases of 16-bit mono and stereo, the only change is from little-endian to big-endian, so a pair of bytes 01 23
is converted to the order 23 12.  Alternatively, we describe this in terms of byte labels as

16-bit 1 or 2 channels
WAV:  0  1
AOB:   1  0

In the original dvda-author, the mono and stereo 24-bit cases were illustrated thus:

24-bit mono samples are packed as follows:
         0   1  2  3  4  5
	WAV: 01 23 45 12 34 56
	AOB: 45 23 56 34 01 12
          2  1  5  4  0  3

 24-bit Stereo samples are packed as follows:
    	 0   1  2  3  4  5  6  7  8  9  10 11
	WAV: 01 23 45 bf 60 8c 67 89 ab b7 d4 e3
	AOB: 45 23 8c 60 ab 89 e3 d4 01 bf 67 b7
         2   1  5  4  8  7  11 10 0  3  6   9

For brevity, we use only the more compact label-based description for each of the 12 cases treated here.
The in-place transformation code that derives from this description was machine generated to reduce the chance for transcription errors,. Identity expressions, e.g. x[i+1] = x[i+1], are omitted.

The 12 cases are as follows. Their values are encoded in matrix S to be found in src/include/multichannel.h
The length of each pattern must be equal to info->sampleunitsize i.e, if Nc is the number of channels:
for 24-bit audio : Nc x 6
for 16-bit audio : Nc x 2 for Nc <= 2 and Nc x 4 for Nc > 2

 16-bit 1  channel
WAV: 0  1
AOB: 1  0

 16-bit 2  channel
c1    0   1
c2           0   1

WAV:  0  1   2   3
AOB:  1  0   3   2
     1:1 1:0 2:1 2:0  [reads : bit 1 of ch1 in original wav audio followed by bit 0 of ch1 followed by bit 1 of ch2 followed by bit 0 of ch2]

 16-bit 3  channel
c1   0   1
c2          0   1
c3                   0   1
                         c1    0   1
                         c2            0   1
                         c3                    0   1

WAV: 0  1 | 2   3  | 4  5   | 6   7  | 8  9  | 10  11
AOB: 5  4 | 11  10 | 1  0   | 3   2  | 7  6  | 9   8
   3:1 3:0|3:3 3:2 |1:1 1:0 |2:1 2:0 |1:3 1:0| 2:3 2:0  [bit 1 of ch 3 in original wav followed by...]


 16-bit 4  channel
c1   0   1
c2          0   1
c3                   0   1
c4                            0   1
                                  c1    0   1
                                  c2             0   1
                                  c3                      0   1
                                  c4                               0    1

WAV: 0  1  | 2  3  | 4   5 |  6   7   | 8  9  | 10  11  | 12  13 | 14  15
AOB: 5  4  | 7  6  | 13  12|  15  14  | 1  0  | 3   2   | 9   8  | 11  10
    3:1 3:0|4:1 4:0|3:3 3:2| 4:3 4:2  |1:1 1:0|2:1  2:0 | 1:3 1:2| 2:3 2:0


 16-bit 5  channel
c1   0   1
c2          0   1
c3                   0   1
c4                            0   1
c5                                    0   1
                                          c1    0   1
                                          c2             0   1
                                          c3                      0   1
                                          c4                              0    1
                                          c5                                       0   1


WAV: 0  1  | 2  3  | 4  5  | 6   7  | 8   9  | 10  11 | 12  13 | 14  15 | 16  17 | 18  19
AOB: 5  4  | 7  6  | 9  8  | 15  14 | 17  16 | 19  18 | 1    0 |  3   2 | 11  10 | 13  12
    3:1 3:0|4:1 4:0|5:1 5:0|3:3 3:2 |4:3 4:2 | 5:3 5:2|1:1  1:0| 2:1 2:0|1:3  1:0|2:3  2:0


 16-bit 6  channel

c1   0   1
c2          0   1
c3                   0   1
c4                            0   1
c5                                    0   1
c6                                            0   1    ----------------------  2nd wav sample ------------
                                                  c1    0   1
                                                  c2             0   1
                                                  c3                      0   1
                                                  c4                                0  1
                                                  c5                                        0   1
                                                  c6                                                 0   1

WAV: 0  1  | 2  3  | 4  5  | 6   7  | 8  9  | 10  11 | 12  13 | 14  15 | 16  17 | 18  19 | 20  21 | 22  23
AOB: 5  4  | 7  6  | 17 16 | 19  18 | 1  0  |  3  2  | 9   8  | 11  10 | 13  12 | 15  14 | 21  20 | 23  22
    3:1 3:0|4:1 4:0|3:3 3:2|4:3 4:2 |1:1 1:0|2:1 2:0 |5:1  5:0|6:1  6:0|1:3  1:2| 2:3 2:2|5:3 5:2 | 6:3 6:2




 24-bit 1  channel
WAV: 0  1  2  3  4  5
AOB: 2  1  5  4  0  3

 24-bit 2  channel

c1   0   1   2
c2                0   1   2
                          c1    0   1   2 -----2nd sample---
                          c2                0   1   2

WAV: 0   1   2    3   4   5     6   7   8   9   10  11
AOB: 2   1   5    4   8   7     11  10  0   3   6   9
     1:2 1:1 2:2  2:1 1:5 1:4   2:5 2:4 1:0 2:0 1:3 2:3



 24-bit 3  channel
c1   0   1   2
c2                 0   1   2
c3                              0   1   2
                                       c1    0   1   2 ----------------- 2nd wav sample ----
                                       c2                0   1   2
                                       c3                              0   1   2


WAV: 0   1   2  |  3    4   5 | 6   7   8 |  9   10  11| 12  13  14 |  15  16  17
AOB: 8   7   17 |  16   6   15| 2   1   5 |  4   11  10| 14  13  0  |  3   9   12
     3:2 3:1 3:5| 3:4  3:0 3:3|1:2 1:1 2:2| 2:1 1:5 1:4| 2:5 2:4 1:0|  2:0 1:3 2:3


 24-bit 4  channel
c1   0   1   2
c2                 0   1   2
c3                             0   1   2
c4                                           0   1    2
                                                     c1   0   1   2 ----------------- 2nd wav sample ----
                                                     c2               0   1   2
                                                     c3                             0   1   2
                                                     c4                                          0   1   2

WAV: 0   1   2  | 3   4    5  | 6   7   8  | 9  10   11 | 12  13  14 | 15  16  17 | 18  19  20 | 21  22  23
AOB: 8   7   11 | 10  20  19  | 23  22  6  | 9  18   21 | 2   1    5 | 4   14  13 | 17  16   0 | 3   12  15
     3:2 3:1 4:2| 4:1 3:5 3:4 |4:5  4:4 3:0|4:0 3:3  4:3| 1:2 1:1 2:2|3:1  1:5 1:4| 2:5 2:4 1:0|2:0  1:3 2:3


 24-bit 5  channel
c1   0   1   2
c2                 0   1   2
c3                             0   1   2
c4                                         0   1    2
c5                                                     0   1    2
                                                               c1   0   1   2 ----------------- 2nd wav sample --------------
                                                               c2               0   1   2
                                                               c3                            0   1   2
                                                               c4                                        0   1   2
                                                               c5                                                    0   1    2

WAV: 0   1    2 |  3   4   5 | 6   7   8 | 9   10  11 | 12  13  14 |15  16  17 |18  19  20 | 21  22  23| 24  25  26 |27  28  29
AOB: 8   7   11 | 10  14  13 | 23  22  26| 25  29  28 | 6   9   12 |21  24  27 |2   1   5  | 4   17  16| 20  19  0  |3   15  18
     1:2 3:1 4:2| 4:1 5:2 5:1|3:5 3:4 4:5|4:4 5:5 5:4 | 3:0 4:0 5:0|3:3 4:3 5:3|1:2 1:1 2:2|2:1 1:5 1:4| 2:5 2:4 1:0|2:0 1:3 2:3




 24-bit 6  channel

c1   0   1   2
c2                 0   1   2
c3                             0   1   2
c4                                         0   1    2
c5                                                     0   1    2
c6                                                                  0   1   2 ----------------------  2nd wav sample ---------------------------------------
                                                                            c1   0   1   2
                                                                            c2                 0  1  2
                                                                            c3                              0  1  2
                                                                            c4                                          0  1   2
                                                                            c5                                                       0   1    2
                                                                            c6                                                                     0   1   2

WAV: 0   1   2   | 3   4   5 | 6   7   8  | 9  10  11 | 12  13  14 | 15  16  17 | 18  19  20 | 21  22  23 | 24  25  26 | 27  28  29 | 30  31  32 | 33  34  35
AOB: 8   7   11  | 10  26  25| 29  28  6  | 9  24  27 | 2   1   5  | 4   14  13 | 17  16  20 | 19  23  22 | 32  31  35 | 34  0   3  | 12  15  18 | 21  30  33
     3:2 3:1 4:2 |4:1 3,2 3,1| 4,2 4,1 3:0|4:0 3,0 4,0| 1:2 1:1 2:2| 2:1 5:2 5:1| 6:2 6:1 1,2| 1,1 2,2 2,1| 5,2 5,1 6,2| 6,1 1:0 2:0| 5:0 6:0 1,0| 2,0 5,0 6,0
     3:2 3:1 4:2 |4:1 3:5 3:4| 4:5 4:4 3:0|4:0 3:3 4:3| 1:2 1:1 2:2| 2:1 5:2 5:1| 6:2 6:1 1:5| 1:4 2:5 2:4| 5:5 5:4 6:5| 6:4 1:0 2:0| 5:0 6:0 1:3| 2:3 5:3 6:3


The sequences presented above may be verified by constructing an .aob file with an easily identifiable sequence of bytes
and presenting it to a (correct) dvd-audio extraction program.  The byte order in the extracted wave file should be the inverse
of that given above.

Now follows the actual manipulation code.  Note that performing the transformation in place requires that the order of execution within a given case be respected.
*/


inline static void interleave_sample_extended(int channels, int count, uint8_t * buf)
{
    int i, size=channels*4;
    uint8_t x;
    uint8_t _buf[size];
    switch (channels)
    {
        case 1:
        case 2:

            for (i = 0; i < count; i += 2)
            { x = buf[i+1]; buf[i+1] = buf[i]; buf[i] = x; }
            break;

        default:

            for (i = 0; i < count; i += size)
                permutation(buf+i, _buf, 0, channels, S, size);
            break;
    }
}

inline static void interleave_24_bit_sample_extended(int channels, int count, uint8_t * buf)

{

    int i, size=channels*6;
    uint8_t _buf[size];


    for (i = 0; i < count; i += size)
        permutation(buf+i, _buf, 1, channels, S, size);


}


// Read numbytes of audio data, and convert it to DVD byte order

uint32_t audio_read(fileinfo_t* info, uint8_t* _buf, uint32_t *bytesinbuffer)
{
    uint32_t requested_bytes = AUDIO_BUFFER_SIZE - *bytesinbuffer,
             buffer_increment = 0,
            rounded_buffer_increment = 0;

    static uint16_t offset;

    static uint8_t fbuf[36];

    uint8_t *buf = _buf + *bytesinbuffer;

    FLAC__bool result;

    //PATCH: provided for null audio characteristics, to ensure non-zero divider

    if (info->sampleunitsize == 0)
          EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Sample unit size is null");

    if (requested_bytes + offset >= info->sampleunitsize)
    {
        requested_bytes -= (requested_bytes + offset) % info->sampleunitsize;
    }

    if (offset)
    {
       memcpy(buf, fbuf, offset);
       if (globals.debugging)
           foutput(WAR "File: %s. Adding %d bytes from last packet for gapless processing...\n", info->filename, offset);
       buffer_increment = offset;
    }

    if (info->type == AFMT_WAVE)
    {
        uint32_t request = (*bytesinbuffer + offset + requested_bytes < AUDIO_BUFFER_SIZE) ? requested_bytes : AUDIO_BUFFER_SIZE - (*bytesinbuffer + offset);

        buffer_increment += fread(buf + offset, 1, request, info->audio->fp);

        if (info->audio->bytesread + buffer_increment > info->numbytes)
        {
            buffer_increment = info->numbytes-info->audio->bytesread;
        }

        info->audio->bytesread += buffer_increment;
        uint32_t bytesread = buffer_increment;

        while (info->audio->bytesread < info->numbytes
               && bytesread < requested_bytes)
        {
            uint32_t request = (*bytesinbuffer + offset + requested_bytes < AUDIO_BUFFER_SIZE) ? requested_bytes - bytesread : AUDIO_BUFFER_SIZE - (*bytesinbuffer + offset + bytesread);

            buffer_increment = fread(buf + bytesread + offset, 1, request, info->audio->fp);

            if (info->audio->bytesread + buffer_increment > info->numbytes)
            {
                buffer_increment = info->numbytes - info->audio->bytesread;
            }

            info->audio->bytesread += buffer_increment;
            bytesread += buffer_increment;
        }

        buffer_increment = bytesread;
    }

#ifndef WITHOUT_FLAC

    else if ((info->type == AFMT_FLAC) || (info->type == AFMT_OGG_FLAC))
    {
        while ((info->audio->n < requested_bytes) && (info->audio->eos==0))
        {
            result = FLAC__stream_decoder_process_single(info->audio->flac);

            if (result==0)
                EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Fatal error decoding FLAC file\n")

            if (FLAC__stream_decoder_get_state(info->audio->flac) == FLAC__STREAM_DECODER_END_OF_STREAM)
            {
                info->audio->eos=1;
            }
        }

        if (info->audio->n >= requested_bytes)
        {
            buffer_increment = requested_bytes;
            uint32_t request = (*bytesinbuffer + offset + buffer_increment < AUDIO_BUFFER_SIZE) ? buffer_increment  : AUDIO_BUFFER_SIZE - (*bytesinbuffer + offset);
            memcpy(buf + offset, info->audio->buf, request);
            memmove(info->audio->buf, &(info->audio->buf[requested_bytes]), info-> audio->n - request);
            info->audio->n -= request;
            buffer_increment = request;
        }
        else
        {
            buffer_increment = info->audio->n;
            uint32_t request = (*bytesinbuffer + offset + buffer_increment < AUDIO_BUFFER_SIZE) ? buffer_increment  : AUDIO_BUFFER_SIZE - (*bytesinbuffer + offset);
            memcpy(buf + offset, info->audio->buf, request);

            info->audio->n = 0;
            buffer_increment = request;
        }

        info->audio->eos = 0;
    }
#endif


    // PATCH: reinstating Lee Feldkamp's 2009 sampleunitsize rounding
    // Note: will add extra zeros on decoding!

    uint16_t rmdr = buffer_increment % info->sampleunitsize;
    rounded_buffer_increment = buffer_increment - rmdr;

    if (globals.padding == 0 && ! globals.lossy_rounding)
    {
            // buffer_increment may not be a multiple of info->sampleunitsize only if at end of file, with remaining bytes < size of audio buffer
            offset = 0;

            if (rmdr)
            {
                // normally at end of file

                if (info->contin_track)
                {
                    offset = rmdr;

                    memcpy(fbuf, buf + rounded_buffer_increment, rmdr);
                    buffer_increment = rounded_buffer_increment;

                    if (globals.debugging)
                       foutput(WAR "File: %s. Shifting %d bytes from offset %d to offset %d to next packet for gapless processing...\n", info->filename, rmdr, rounded_buffer_increment, buffer_increment);
                }
                else
                {
                    uint16_t padbytes = info->sampleunitsize - rmdr;
                    if (padbytes + buffer_increment > AUDIO_BUFFER_SIZE)
                        padbytes = AUDIO_BUFFER_SIZE - buffer_increment;

                    memset(buf + buffer_increment, 0, padbytes);
                    buffer_increment += padbytes;
                    foutput(WAR "Padding track with %d bytes (ultimate packet).\n", padbytes);
                }
            }
    }
    else
    {
        if (rmdr)
        {
            // normally at end of file

            if (globals.lossy_rounding)
            {
               // audio loss at end of audio file may result in a 'blip'

               buffer_increment = rounded_buffer_increment;
               if (globals.debugging)
                   foutput("%s %s %s %d %s %d.\n", WAR "Cutting audio file", info->filename, "by", rmdr, "bytes out of", buffer_increment);
            }
            else
            {
                uint16_t padbytes = info->sampleunitsize - rmdr;
                if (padbytes + buffer_increment > AUDIO_BUFFER_SIZE)
                    padbytes = AUDIO_BUFFER_SIZE - buffer_increment;

                uint8_t padding_byte = 0;

                if (globals.padding_continuous && buffer_increment)
                {
                   padding_byte = buf[buffer_increment - 1];
                }

                memset(buf + buffer_increment, padding_byte, padbytes);
                buffer_increment += padbytes;

                if (globals.debugging)
                {
                    foutput(WAR "Padding track with %d bytes", padbytes);
                    if (globals.padding_continuous && padbytes) foutput("%s", " continuously.");
                    foutput("%s", "\n");
                }
            }
       }
    }

    // End of patch

    // Convert little-endian WAV samples to big-endian MPEG LPCM samples

    if ((info->channels > 6) || (info->channels < 1))
    {
        foutput(ERR "problem in audio.c ! %d channels \n",info->channels);
        EXIT_ON_RUNTIME_ERROR
    }

    switch (info->bitspersample)
    {
        case 24:

            // Processing 24-bit audio
            interleave_24_bit_sample_extended(info->channels, buffer_increment, buf);
            break;

        case 16:

            // Processing 16-bit audio
            interleave_sample_extended(info->channels, buffer_increment, buf);
            break;

        default:

            /* 20-bit stereo samples are packed as follows:
            Packet 0: 1980 bytes
            Packets 1-: 2000 bytes

            Stored similarly to 24-bit:

            4 samples, most significant 16 bits of each sample first, in big-endian
            order, followed by 2 bytes containing the least-significant 4 bits of
            each sample.

            I'm guessing  that  20-bits are stored in the most-significant
            20-bits of the 24.
            */

            // FIX: Handle 20-bit audio and maybe convert other formats.
            foutput(ERR "%d bit audio is not supported\n",info->bitspersample);
            EXIT_ON_RUNTIME_ERROR
    }


    *bytesinbuffer += buffer_increment;

    return(buffer_increment);
}

int audio_close(fileinfo_t* info)
{
    if (globals.debugging) foutput("%s %s\n", INF "Closing audio file", info->filename);
    if (info->type==AFMT_WAVE)
    {
        fclose(info->audio->fp);
    }
#ifndef WITHOUT_FLAC
    else if ((info->type==AFMT_FLAC) || (info->type == AFMT_OGG_FLAC))
    {
        FLAC__stream_decoder_delete(info->audio->flac);
    }
#endif
    // info->audio[i][j] will be freed before exit in free_memory.
    return(0);
}

