/*
  
ats2wav - extract uncompressed LPCM audio from a DVD-Audio disc

Copyright Dave Chapman <dave@dchapman.com> 2005,
revised by Fabrice Nicol <fabnicol@users.sourceforge.net> 2008
for Windows compatibility, directory output and dvda-author integration.

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


#include "ats2wav.h"
#include "ats.h"
#include "c_utils.h"

#include "winport.h"
#include "multichannel.h"
#include "auxiliary.h"


extern globalData globals;
extern uint8_t channels[21];
#if 0
static unsigned char wav_header[80]= {'R','I','F','F',   //  0 - ChunkID
                               0,0,0,0,            //  4 - ChunkSize (filesize-8)
                               'W','A','V','E',    //  8 - Format
                               'f','m','t',' ',    // 12 - SubChunkID
                               40,0,0,0,           // 16 - SubChunk1ID  // 40 for WAVE_FORMAT_EXTENSIBLE
                               0xFE, 0xFF,         // 20 - AudioFormat (1=16-bit)
                               2,0,                // 22 - NumChannels
                               0,0,0,0,            // 24 - SampleRate in Hz
                               0,0,0,0,            // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
                               4,0,                // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
                               0x10,0,             // 34 - BitsPerSample (16)
                               0x16,0,             // 36 - cbSize Size of extension (22)
                               0x10,0,             // 38 - wValidBitsPerSample (16 bits)
                               0,0,0,0,            // 40 - dwChannelMask
                               0x01,0x00,          // 44 - subFormat 0x1 for PCM
                               0,0,0,0,0x10,0,0x80,0,0,0xaa,0,0x38,0x9b,0x71,                    // 46 - GUID (fixed string)
                               'f','a','c','t',    // 60 - "fact"
                               4,0,0,0,            // 64 - ckSize (4)
                               0,0,0,0,            // 68 - dwSampleLength = NumChannels * ckSize/(BitsPerSample/8)
                               'd','a','t','a',    // 72 - ckID
                               0,0,0,0             // 76 - ckSize -> // 79
                              };
#endif
// Reverse table (to be used to convert AOBs to WAVs

static const uint8_t  T[2][6][36]=
     {{ {0}, // 4
        {0}, // 8
        {5, 4, 7, 6, 1, 0, 9, 8, 11, 10, 3, 2}, 
        {9, 8, 11, 10, 1, 0, 3, 2, 13, 12, 15, 14, 5, 4 ,7, 6}, 
        {9, 8, 11, 10, 13, 12, 1, 0, 3, 2, 15, 14, 17, 16, 19, 18, 5, 4, 7, 6}, //20, rev
        {13, 12, 15, 14, 17, 16, 1, 0, 3, 2, 5, 4, 19, 18, 21, 20, 23, 22, 7, 6, 9, 8, 11, 10}}, //rev
      {{4,  1,  0,  5,  3,  2},
        {8, 1, 0, 9, 3, 2, 10, 5, 4, 11, 7, 6},
        {14, 7, 6, 15, 9, 8, 4, 1, 0, 16, 11, 10, 17, 13, 12, 5, 3, 2},
        {20, 13, 12, 21, 15, 14, 8, 1, 0, 9, 3, 2, 22, 17, 16, 23, 19, 18, 10, 5, 4, 11, 7, 6},
        {24, 13, 12, 25, 15, 14, 26, 17, 16, 8, 1, 0, 9, 3,  2,  27, 19, 18, 28, 21, 20, 29,  23, 22,  10, 5, 4, 11, 7, 6}, //30, rev
        {28, 13, 12, 29, 15, 14,  8, 1, 0,  9, 3, 2, 30, 17, 16, 31, 19, 18, 32, 21, 20, 33, 23, 22, 10, 5, 4, 11, 7,  6, 34, 25, 24, 35, 27, 26 }}
    };

// sizes of preceding table


static void deinterleave_24_bit_sample_extended(uint8_t channels, int count, uint8_t *buf)
{
    // Processing 16-bit case
    int i, size=channels*6;
    // Requires C99
    uint8_t _buf[size];

    for (i=0; i < count ; i += size)
        permutation(buf+i, _buf, 1, channels, T, size);

}

static void deinterleave_sample_extended(uint8_t channels, int count, uint8_t *buf)
{

    int x,i, size = channels * 4;

    uint8_t _buf[size];

    switch (channels)
    {
    case 1:
    case 2:
        for (i = 0; i < count; i += 2)
        {
            x = buf[i];
            buf[i] = buf[i + 1];
            buf[i + 1] = x;
        }
        break;

    default:
        for (i =0; i < count ; i += size)
            permutation(buf+i, _buf, 0, channels, T, size);
    }
}

static void convert_buffer(WaveHeader* header, uint8_t *buf, int count)
{
    switch (header->wBitsPerSample)
    {
        case 24:

            deinterleave_24_bit_sample_extended(header->channels, count, buf);
            break;

        case 16:
            deinterleave_sample_extended(header->channels, count, buf);
            break;

        default:
            // FIX: Handle 20-bit audio and maybe convert other formats.
            printf("[ERR]  %d bit %d channel audio is not supported\n", header->wBitsPerSample, header->channels);
            return;
            //exit(EXIT_FAILURE);
    }
}


inline static void  aob_open(WaveData *info)
{

    if (file_exists(info->infile.filename))
        info->infile.filesize = stat_file_size(info->infile.filename);

    info->infile.fp = fopen(info->infile.filename, "rb")  ;

    if (info->infile.fp != NULL)
    {
        info->infile.isopen = true;
    }
    else
    {
        foutput("File: %s\n", info->infile.filename);
        EXIT_ON_RUNTIME_ERROR_VERBOSE("INFILE open issue.")
    }
}

inline static void wav_output_path_create(const char* dirpath, WaveData *info, int track, int title)
{

    char Title[23] = {0};
    sprintf(Title, "track_%02d_title_%02d_.wav", track + 1, title + 1);

    info->outfile.filename = filepath(dirpath, Title);
}

inline static void wav_output_open(WaveData *info)
{
    if (globals.veryverbose)
    {
        foutput(INF "Opening file %s ...\n", info->outfile.filename);
    }
    
    info->outfile.fp = fopen(info->outfile.filename, "ab");
    if (info->outfile.fp != NULL)
    {
        info->outfile.isopen = true;
    }
    else
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE("OUTFILE open issue.")
    }
}


inline static int calc_position(FILE* fileptr, const uint64_t offset0)
{
    uint8_t buf[4];
    int position;

    fseeko(fileptr, offset0 + 14, SEEK_SET);
    int result = fread(buf, 4, 1, fileptr);
    
    if (result != 1)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting packet position.")
    }
    

    /* got to system header and read first 4 bytes to detect whether pack is start or not */

    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB)
    {
        position = FIRST_PACK;
    }
    else
    {
        /* go to end of sector : if end of file, then last pack, idem if new pack detected ; otherwise middle pack */

        int res = fseeko(fileptr, 2044, SEEK_CUR);

        if (res != 0)
        {
            position = LAST_PACK;
        }
        else
        {
            int n = fread(buf, 1, 4, fileptr);

            if (n != 4 || (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB))
            {
                position = LAST_PACK;
            }
            else
            {
                position = MIDDLE_PACK;
            }
        }
    }
    
    return(position);
}

inline static int peek_pes_packet_audio(WaveData *info, WaveHeader* header, bool *status, uint8_t* continuity)
{
    if (! info->infile.isopen) aob_open(info);
    
    uint64_t offset0 = ftello(info->infile.fp);
     
    int position = calc_position(info->infile.fp, offset0);
    
    fseeko(info->infile.fp, offset0 + 29 + (position == FIRST_PACK ? 21 : 0), SEEK_SET);

    uint8_t sample_size[1] = {0};
    uint8_t sample_rate[1] = {0};
    uint8_t continuity_counter[1] = {0};

    int result = fread(continuity_counter, 1, 1, info->infile.fp);

    if (result != 1)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting continuity counter.")
    }

    *continuity = continuity_counter[0];

    fseeko(info->infile.fp, 5, SEEK_CUR);  // +6

    result = fread(sample_size, 1, 1, info->infile.fp);


    if (result != 1)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting sample size.")
    }
    
    result = fread(sample_rate, 1, 1, info->infile.fp);

    if (result != 1)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting sample rate.")
    }
    
    uint8_t high_nibble = (sample_rate[0] & 0xf0) >> 4;

    switch(high_nibble)
    {
        case 0:
            header->dwSamplesPerSec = 48000;
            break;
        case 0x1:
            header->dwSamplesPerSec = 96000;
            break;
        case 0x2:
            header->dwSamplesPerSec = 192000;
            break;
        case 0x8:
            header->dwSamplesPerSec = 44100;
            break;
        case 0x9:
            header->dwSamplesPerSec = 88200;
            break;
        case 0xa:
            header->dwSamplesPerSec = 176400;
            break;
        default:
            *status = INVALID;
            break;
    }

    if (sample_size[0] == 0x0f || sample_size[0] == 0x2f)
    {
        if ((sample_rate[0] & 0xf) != 0xf)
        {
            foutput("%s", "[ERR]  Coherence_test (peek) : sample_rate and sample_size are incoherent (no 0xf lower nibble).\n");
            fflush(NULL);
            *status = INVALID;
        }
    }
    else
    if (sample_size[0] == 0x00 || sample_size[0] == 0x22)
    {
        if ((sample_rate[0] & 0xf) != high_nibble)
        {
            foutput("%s", "[ERR]  Coherence_test (peek) : sample_rate and sample_size are incoherent (lower nibble != higher nibble).\n");
            *status = INVALID;
            fflush(NULL);
        }
    }

    header->wBitsPerSample = (sample_size[0] == 0x2f || sample_size[0] == 0x22) ? 24 : ((sample_size[0] == 0x0f || sample_size[0] == 0x00) ? 16 : 0) ;

    if (! header->wBitsPerSample) *status = INVALID;

    fseeko(info->infile.fp, 1, SEEK_CUR);

    uint8_t channel_assignment[1] = {0};

    result = fread(channel_assignment, 1, 1, info->infile.fp);

    if (result != 1)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting channel.")
    }
    
    if (channel_assignment[0] > 20) *status = INVALID;

    header->channels = (channel_assignment[0] < 21) ? channels[channel_assignment[0]] : 0;

    header->nBlockAlign =  header->wBitsPerSample / 8 * header->channels ;
    header->nAvgBytesPerSec = header->nBlockAlign * header->dwSamplesPerSec;

    uint32_t cga2wav_channels[21] = {0x4, 0x3,   0x103, 0x33,  0xB,  0x10B, 0x3B,  0x7,  0x107, 0x37,
                                     0xF, 0x10F, 0x3F,  0x107, 0x37, 0xF,   0x10F, 0x3F, 0x3B,  0x37, 0x3B};

    if (channel_assignment[0] < 21)
    {
      header->dwChannelMask = cga2wav_channels[channel_assignment[0]];
    }

   
    fseeko(info->infile.fp, offset0, SEEK_SET);

    return position;
}


inline static uint64_t get_pes_packet_audio(WaveData *info,
                                            WaveHeader *header,
                                            int *position,
                                            uint8_t* continuity,
                                            uint64_t* written_bytes,
                                            uint64_t wav_numbytes,
                                            unsigned long *pack_in_track,
                                            unsigned long *pack_in_title,
                                            unsigned long *pack_in_group)
{
    int audio_bytes;
    uint8_t PES_packet_len_bytes[2];
    uint8_t audio_buf[2048 * 2]; // in case of incomplete buffer shift

    // CAUTION : check coherence of this table and the S table of audio.c, of which it is only a subset.

    const uint16_t T[2][6][6]=     // 16-bit table
    {
         {{ 	2000, 16,   22, 11, 16, 16},
            {	2000, 16,   28, 11, 16, 10},
            { 	2004, 24,   24, 15, 12, 10},
            { 	2000, 16,   28, 11, 16, 10},
            { 	2000, 20,   22, 15, 16, 10},
            { 	1992, 24,   22, 10, 10, 10}},
        // 24-bit table
        {{    	2004, 24,   22, 15, 12, 12},
            { 	2004, 24,   24, 15, 12, 10},
            { 	1998, 18,   28, 15, 10, 10},
            { 	1992, 24,   22, 10, 10, 10},
            { 	1980,  0,   22, 15, 10, 10},
            { 	1980,  0,   22, 15, 16, 16}}
    };

    const short int table_index = header->wBitsPerSample == 24 ? 1 : 0;

#   define X T[table_index][header->channels-1]

    int lpcm_payload = X[0];
    int lpcm_payload_cut = 0;
    const int firstpackdecrement = X[1];
    const int lastpack_audiopesheaderquantity = X[2];
    const int firstpack_lpcm_headerquantity = X[3];
    const int midpack_lpcm_headerquantity   = X[4];
    const int lastpack_lpcm_headerquantity  = X[5];

#   undef X

    //////////////////////////////

    if (! info->infile.isopen) aob_open(info);

    uint64_t offset0 = ftello(info->infile.fp);
    
    int res  = 0, result;    
    static bool gapless_cut;
    static bool gapless_rmdr;
    int lpcm_payload_rmdr = 0;

    if (wav_numbytes > 0 && wav_numbytes - *written_bytes < lpcm_payload)
    {
        lpcm_payload_cut = wav_numbytes - *written_bytes;
        gapless_cut = true;
        lpcm_payload_rmdr = lpcm_payload - lpcm_payload_cut;
    }
    
    do {
            if (! gapless_rmdr) *position = calc_position(info->infile.fp, offset0);
            
            switch(*position)
            {
                case FIRST_PACK :
                    audio_bytes = lpcm_payload - firstpackdecrement;
                    fseeko(info->infile.fp, offset0 + 53 + firstpack_lpcm_headerquantity, SEEK_SET);
                    *pack_in_track = 1;
                    *pack_in_title = 1;
                    ++*pack_in_group;
                    break;
        
                case MIDDLE_PACK :
                    audio_bytes = gapless_cut ? lpcm_payload_cut : (gapless_rmdr ? lpcm_payload_rmdr : lpcm_payload);
                    if (! gapless_rmdr)
                    {
                        fseeko(info->infile.fp, offset0 + 29, SEEK_SET);
                        fread(continuity, 1, 1, info->infile.fp);
                        fseeko(info->infile.fp, midpack_lpcm_headerquantity + 2, SEEK_CUR);
                        ++*pack_in_track;
                        ++*pack_in_title;
                        ++*pack_in_group;
                    }
                    break;
        
                case LAST_PACK :
                    fseeko(info->infile.fp, offset0 + 18, SEEK_SET);
                    result = fread(PES_packet_len_bytes, 1, 2, info->infile.fp);
                    if (result != 2)
                    {
                            EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR " Error detecting last pack.")
                    }
                    audio_bytes = (PES_packet_len_bytes[0] << 8 | PES_packet_len_bytes[1]) - lastpack_audiopesheaderquantity;
                    /* skipping rest of audio_pes_header, i.e 8 bytes + lpcm_header */
                    fseeko(info->infile.fp, offset0 + 32 + lastpack_lpcm_headerquantity, SEEK_SET);
                    ++*pack_in_track;
                    ++*pack_in_title;
                    ++*pack_in_group;
                    break;
    
            }
    } 
    while(0);

    // Here "cut" for a so-called 'gapless' track
    // The cut should respect the size of permutation tables.
           
    res += fread(audio_buf + res, 1, audio_bytes, info->infile.fp);

    if (globals.maxverbose)    
    {
       foutput(MSG_TAG "Audio bytes: %d, res: %d\n", audio_bytes, res);
       if (res != audio_bytes)
           foutput(WAR "Caution : Read %d instead of %d\n", res, audio_bytes);
    }
    
    convert_buffer(header, audio_buf, res);

    int fpout_size_increment = fwrite(audio_buf, 1, res, info->outfile.fp);
    
    if (*position == LAST_PACK)
    {
        S_CLOSE(info->outfile)
    }

    uint64_t offset1 = offset0;

    if (gapless_cut) offset1 = ftello(info->infile.fp);
    else
        if (gapless_rmdr) offset1 = /* got to 2048 */;
    else
            offset1 = offset0 +2048;
        
    if (offset1 >= filesize(info->infile))
        *position = END_OF_AOB;
    else
        fseeko(info->infile.fp, offset1, SEEK_SET);
   
    if (globals.maxverbose)
           foutput(MSG_TAG "Position : %d\n", *position);
    
    *written_bytes += fpout_size_increment;
    return(fpout_size_increment);
}

static inline void filestat_clean(filestat_t* f)
{
    free(f->filename);
    f->filename = NULL;
    if (f->fp != NULL) fclose(f->fp);
    f->isopen = false;
}

static inline filestat_t filestat_copy(filestat_t f)
{
    filestat_t out;
    out.filename = f.filename ? strdup(f.filename) : NULL;
    out.isopen = f.isopen;
    if (out.isopen)
    {
        out.fp = fopen(f.filename, "rb+");  // beware of concurrency
    }
    else out.fp = NULL;
    out.filesize = f.filesize;
    return out;
}

static inline WaveData* wavedata_copy(const WaveData* w)
{
    WaveData* out = calloc(1, sizeof (WaveData));
    if (out == NULL) return NULL;

    out->database  = w->database ? strdup(w->database) : NULL;
    out->filetitle = w->filetitle ? strdup(w->filetitle) : NULL;
    out->automatic = w->automatic;
    out->prepend   = w->prepend;
    out->in_place  = w->in_place;
    out->cautious  = w->cautious;
    out->interactive = w->interactive;
    out->padding     = w->padding;
    out->prune       = w->prune;
    out->virtual     = w->virtual;
    out->repair      = w->repair;
    out->padbytes    = w->padbytes;
    out->prunedbytes = w->prunedbytes;

    out->infile  = filestat_copy(w->infile);
    out->outfile = filestat_copy(w->outfile);
    return out;
}

static inline void wavedata_clean(WaveData* w)
{
    free(w->database);
    w->database = NULL;
    free(w->filetitle);
    w->filetitle = NULL;
    filestat_clean(&w->infile);
    if (! w->in_place) filestat_clean(&w->outfile);
    free(w);
    w = NULL;
}


int get_ats_audio_i(int i, fileinfo_t* files[9][99], WaveData *info)
{

    int title = 0;
    int track = 0;
    unsigned long pack_in_group = 0;
    int position = FIRST_PACK;
    uint8_t continuity = 0;

    WaveHeader header;

    info->infile.isopen = false;
    info->infile.filename = globals.aobpath[i];
    info->infile.filesize = stat_file_size(info->infile.filename);

    char g_i[3];
    sprintf(g_i, "g%d", i + 1);

    char* dir_g_i = filepath(globals.settings.outdir, g_i);

    if (! s_dir_exists(dir_g_i))
    {
        secure_mkdir(dir_g_i, globals.access_rights);
        if (globals.veryverbose)
                foutput("%s |%s|\n", INF "Creating directory", dir_g_i);
    }

    const int lpcm_payload[2][6] = {{2000,	2000, 2004,	2000,	2000,	1992}, // 16-bit table
                                    { 2004, 	2004, 	1998, 	1992, 	1980, 	1980}}; //24-bit table

    // Start of title loop title

    while (position != END_OF_AOB)
    {
        /* First pass to get basic audio characteristics (sample rate, bit rate, cga */
        
        bool status = VALID;
        errno = 0;
        bool new_track  = false;
        uint8_t continuity_save = 0;
        unsigned long pack_in_title = 1;

        do
        {
            position = peek_pes_packet_audio(info, &header, &status, &continuity);
            
            if (status == VALID) break;
        }
        while (position == FIRST_PACK 
               || position == MIDDLE_PACK);
        
        if (continuity != 0)
        {
            if (globals.maxverbose)
                foutput("%s %d\n", MSG_TAG "Continuity counter has wrong value (should be 0): ", continuity);
        }


        // Track loop
           do {

                files[i][track]->bitspersample = header.wBitsPerSample;
                files[i][track]->samplerate    = header.dwSamplesPerSec;
                files[i][track]->channels      = header.channels;

                uint64_t x = 0, numsamples = 0, numbytes = 0, wav_numbytes = 0;
                int bitrank = header.wBitsPerSample == 24 ? 1 : 0;

                if (files[i][track]->PTS_length)     // Use IFO files
                {
                    numsamples = (files[i][track]->PTS_length * files[i][track]->samplerate) / 90000;

                    files[i][track]->lpcm_payload = lpcm_payload[bitrank][header.channels - 1];

                    if (numsamples)
                        x = 90000 * numsamples;
                    else
                    {
                        foutput("%s", ERR "Found null samplerate or PTS length. Continuing in forensic mode...\n");
                        files[i][track]->PTS_length = 0;
                    }

                    // Adjust for rounding errors:

                    if (x < files[i][track]->PTS_length * files[i][track]->samplerate)
                    {
                        ++numsamples;
                    }

                    numbytes = (numsamples * files[i][track]->channels * files[i][track]->bitspersample) / 8;

                    int sampleunitsize = header.wBitsPerSample / 8 * header.channels * 2;

                    // Taking modulo

                    int rmdr = numbytes % sampleunitsize ;

                    wav_numbytes = numbytes +  (rmdr ? sampleunitsize - rmdr : 0);

                }

                if (errno)
                {
                   foutput(ERR "Error while trying to recover audio characteristics of file %s.\n        Exiting...\n",
                           filename(info->infile));
                   exit(-7);
                }

                if (status == INVALID)
                {
                   foutput(ERR "Error while trying to recover audio characteristics : invalid status or input file %s.\n        Exiting...\n",
                           filename(info->infile));
                   exit(-8);
                }

               bool debug;

                unsigned long pack_in_track = 1;

                wav_output_path_create(dir_g_i, info, track, title);

                files[i][track]->filename = strdup(info->outfile.filename);

                wav_output_open(info);

                if (globals.fixwav_prepend)  // --aob2wav rather than --aob-extract
                {
                    /* generate header in empty file. We must allow prepend and in_place for empty files */

                    debug = globals.debugging;

                    /* Hush it up as there will be spurious error mmsg */
                    globals.debugging = false;

                    info->outfile.filesize = 0;  // necessary to reset so that header can be generated in empty file

                    WaveData* info2 = wavedata_copy(info);

                    info2->prepend = true;
                    info2->in_place = true;
                    info2->infile = info2->outfile;

                    fixwav(info2, &header);

                    wavedata_clean(info2);

                    errno = 0;

                    if (globals.veryverbose)
                        foutput(MSG_TAG "%s\n", "Header prepended.");
                }

                /* second pass to get the audio */

                position = FIRST_PACK;
                uint64_t written_bytes = 0;
                unsigned long rmdr_payload = 0;

                do
                {
                    continuity_save = continuity;
                    get_pes_packet_audio(info,
                                         &header,
                                         &position,
                                         &continuity,
                                         &written_bytes,
                                         wav_numbytes,
                                         &pack_in_track,
                                         &pack_in_title,
                                         &pack_in_group);

                    if (globals.maxverbose)
                    {
                        if (wav_numbytes)
                        {
                                foutput(MSG_TAG "Pack %lu, Number of written bytes: %lu / File bytes: %lu\n",
                                        pack_in_track,
                                        written_bytes,
                                        wav_numbytes);

                        }
                        else
                        {
                                 foutput(MSG_TAG "Pack %lu, Number of written bytes: %lu\n",
                                        pack_in_track,
                                        written_bytes);
                        }
                    }

                    if (wav_numbytes == 0)
                    {
                        if (continuity_save != 0 && continuity_save != 0x1F && continuity == 0)
                        {

                           files[i][track]->wav_numbytes = 0;
                           files[i][track]->numbytes  = written_bytes;
                           ++track;
                           if (globals.veryverbose)
                              foutput("%s %d %s %d %s %d  %s %lu\n", MSG_TAG "Group ", i + 1, "Title ", title + 1, "Track ", track, "Written bytes: ", written_bytes);
                           // written bytes are in excess in case of gapless tracks

                           break;
                        }
                    }
                    else
                    {

                        if (wav_numbytes > written_bytes)
                        {
                           if (wav_numbytes - written_bytes <= files[i][track]->lpcm_payload)
                           {
                              rmdr_payload = (unsigned long) (wav_numbytes - written_bytes);
                              if (globals.veryverbose) foutput("%s %lu %s\n", INF "Cutting next sector using ", rmdr_payload, " bytes.");
                              if (globals.maxverbose)
                               {
                                  foutput("%s %lu\n", INF "Looping next sector...", pack_in_group - 1);
                               }
                           }
                        }
                        else
                        if (wav_numbytes == written_bytes)
                        {

                            files[i][track]->wav_numbytes = written_bytes;
                            files[i][track]->numbytes  = written_bytes;

                            ++track;
                            if (globals.veryverbose)
                            {
                                  foutput("%s %d %s %d %s %d %s %lu\n", MSG_TAG "Group ", i + 1, "Title ", title + 1, "Track ", track, "File bytes: ", wav_numbytes);

                                  foutput(INF "Track %d Pack %lu | Title %d Pack %lu | Group %d Pack %lu - Cutting tracks at offset: %" PRIu64
                                                " to complete file from: %" PRIu64
                                                " to: %" PRIu64 " audio bytes.\n",
                                                track,             // 1-based
                                                pack_in_track - 1, // 1-based
                                                title + 1,         // 0-based
                                                pack_in_title - 1, // 1-based
                                                i + 1,         // 1-based
                                                pack_in_group - 1, // 1-based
                                                rmdr_payload,
                                                wav_numbytes - rmdr_payload,
                                                wav_numbytes);

                                  if (files[i][track-1]->last_sector != pack_in_group - 1)  // 0-based v. 1-based
                                  {
                                      foutput(WAR "IFO last sector incorrect: %lu against current %lu\n", files[i][track-1]->last_sector, pack_in_group - 1);
                                  }

                                  if (position != LAST_PACK && position != END_OF_AOB)
                                  {
                                      if (continuity != 0)
                                      {
                                          foutput(WAR "sector continuity counter not null, value : %d, previous value %d\n", continuity, continuity_save);
                                      }
                                      else
                                      {
                                          foutput(WAR "sector continuity counter null as expected, previous value %d\n",  continuity_save);
                                      }
                                  }
                            }

                            break;
                       }
                       else
                       if (wav_numbytes < written_bytes)
                       {
                            foutput(INF "Track %d Pack %lu | Title %d Pack %lu | Group %d Pack %lu - issue "
                                          " File bytes: %" PRIu64
                                          " Written bytes:: %" PRIu64 " audio bytes.\n",
                                          track,             // 1-based
                                          pack_in_track - 1, // 1-based
                                          title + 1,         // 0-based
                                          pack_in_title - 1, // 1-based
                                          i + 1,         // 0-based
                                          pack_in_group - 1,
                                          wav_numbytes,
                                          written_bytes);

                            EXIT_ON_RUNTIME_ERROR_VERBOSE("Incoherent byte counts.")
                        }
                    }
                }
                while (position == FIRST_PACK
                       || position == MIDDLE_PACK);

                new_track = false;

                if (wav_numbytes - written_bytes > files[i][track]->lpcm_payload)
                {
                  foutput(WAR "Remaining bytes %lu in excess of payload %d \n", wav_numbytes - written_bytes, files[i][track]->lpcm_payload);

                  if (files[i][track]->last_sector != pack_in_group - 1)
                  {
                      foutput(WAR "IFO last sector incorrect: %lu against current %lu\n", files[i][track]->last_sector, pack_in_group - 1);
                  }

                  files[i][track]->numbytes  = written_bytes;
                }

                info->outfile.filesize = written_bytes + header.header_size_out;

//                if (files[i][k]->PTS_length && wav_numbytes < numbytes)
//                    {
//                        int delta = numbytes - wav_numbytes;
//                        info->outfile.filesize -= delta;
//                        if (globals.veryverbose)
//                        foutput(INF "Number of bytes in file %s not aligned with sampling units by %d bytes.\n", info->outfile.filename, delta);
//                        int res = truncate(info->outfile.filename, info->outfile.filesize );
//                        files[i][k]->wav_numbytes = res == 0 ? wav_numbytes : numbytes;
//                        if (globals.veryverbose)
//                        {
//                           if (res == 0)
//                               foutput(MSG_TAG "Truncated %d bytes at end of file %s.\n", delta, info->outfile.filename);
//                           else
//                           {
//                               foutput(ERR "Failed to truncate %d bytes at end of file %s.\n", delta, info->outfile.filename);
//                           }
//                        }
//                    }

                foutput(MSG_TAG "Wrote %s, %" PRIu64 " bytes, %.2f MB.\n",
                                        info->outfile.filename,
                                        info->outfile.filesize,
                                        (double) info->outfile.filesize / (double) (1024 * 1024));

                if (globals.fixwav_prepend)
                {
                    /* WAV output is now OK except for the wav file size-based header data.
                     * ckSize, data_ckSize and nBlockAlign must be readjusted by computing
                     * the exact audio content bytesize. Also we no longer prepend the header
                     * but overwrite the existing one */

                    WaveData* info2 = wavedata_copy(info);
                    info2->prepend = false;
                    info2->in_place = true;
                    info2->infile = info->outfile;

                    fixwav(info2, &header);

                    wavedata_clean(info2);

                    if (track == 99)
                    {
                        EXIT_ON_RUNTIME_ERROR_VERBOSE("DVD-Audio specifications only allow 99 tracks per group.")
                    }

                    // Possible adjustment here

                    files[i][track]->wav_numbytes = header.data_cksize;

                    globals.debugging = debug;
                    errno = 0;
                }

                S_CLOSE(info->outfile)

                if (position == LAST_PACK)
                {
                    if (globals.veryverbose)
                            foutput("%s\n", INF "Closing track and opening new one.");
                }
                else
                if (position == END_OF_AOB)
                {
                    if (globals.veryverbose)
                            foutput("%s\n", INF "Closing last track of AOB.");
                }

            }   while (position != END_OF_AOB && position != LAST_PACK);

           // End of track loop

        ++title;  // Title loop
    }

    free(dir_g_i);

    return(errno);
}

static void audio_extraction_layout(fileinfo_t* files[9][99])
{
    foutput("\n%s", "DVD Layout\n");
    foutput("%s\n",ANSI_COLOR_BLUE "Group" ANSI_COLOR_GREEN "  Track    " ANSI_COLOR_YELLOW "Rate" ANSI_COLOR_RED " Bits" ANSI_COLOR_RESET "  Ch  Input audio (B)   Output wav (B) 1st sector  last sect. PTS length  Filename\n");

    for (int i = 0; i < 81; ++i)
        for (int j = 0; j < 99 && files[i][j]->filename != NULL; ++j)
        {
           foutput("  "ANSI_COLOR_BLUE "%d     " ANSI_COLOR_GREEN "%02d"ANSI_COLOR_YELLOW "  %6" PRIu32 "   " ANSI_COLOR_RED "%02d" ANSI_COLOR_RESET "   %d       %10" PRIu64 "   %10" PRIu64"   %10" PRIu64"   %10" PRIu64"   %10" PRIu64"   ",
                     i+1, j+1, files[i][j]->samplerate, files[i][j]->bitspersample, files[i][j]->channels, files[i][j]->wav_numbytes, files[i][j]->numbytes, files[i][j]->first_sector, files[i][j]->last_sector, files[i][j]->PTS_length);
           foutput("%s\n",files[i][j]->filename);
        }

    foutput("\n\n%s\n\n", MSG_TAG "The Audio bytes column gives the exact audio size of extracted files,\n" MSG_TAG "excluding the header (44 bytes for mono/stereo or 80 bytes for multichannel.)");
}


static inline int scan_ats_ifo(fileinfo_t **files, uint8_t *buf)
{
    int i, title, track;
    static int group;

    i = 2048;

    int numtitles = uint16_read(buf + i);
    int ntracks = 0;

    uint8_t ntitletracks[numtitles];
    uint32_t titlelength;

    i += 8;
    i += 8 * numtitles;

    for (title = 0; title < numtitles; ++title)
    {
        i += 2;

        ntitletracks[title] = buf[i];

        i += 2;

        titlelength = uint32_read(buf + i);

        i += 12;

        for (track = 0; track < ntitletracks[title]; ++track)
        {
            i += 6;

            files[ntracks + track]->first_PTS  = uint32_read(buf + i);

            i += 4;

            files[ntracks + track]->PTS_length = uint32_read(buf + i);

            i += 10;
        }


        for (track = 0; track < ntitletracks[title]; ++track)
        {
            i += 4;

            files[ntracks + track]->first_sector = uint32_read(buf +i);
            i += 4;

            files[ntracks + track]->last_sector = uint32_read(buf +i);
            i += 4;
        }

        ntracks += ntitletracks[title];

    }

    if (globals.debugging)
    {
        ++group;

        printf(ANSI_COLOR_BLUE "Group " ANSI_COLOR_GREEN "   Track/N  " ANSI_COLOR_YELLOW  "first sector" ANSI_COLOR_RED "  last sector"
               ANSI_COLOR_YELLOW "  first pts" ANSI_COLOR_RED "    pts length\n");

        for (track = 0; track < ntracks; ++track)
        {
              printf(ANSI_COLOR_BLUE "%02d  " ANSI_COLOR_GREEN "      %02d/%02d" ANSI_COLOR_YELLOW " %12u" ANSI_COLOR_RED "%12u"
                     ANSI_COLOR_YELLOW "%12" PRIu64 ANSI_COLOR_RED "%12" PRIu64 "\n",
                   group, track+1, ntracks, files[track]->first_sector, files[track]->last_sector,
                     files[track]->first_PTS, files[track]->PTS_length);
        }
        printf("\n");
    }

    return(ntracks);
}

int get_ats_audio(bool use_ifo_files)
{
    fileinfo_t* files[81][99]; // 9 groups but possibly up to 9 AOBs per group (ATS_01_1.AOB...ATS_01_9.AOB)
    for (int i = 0; i < 81; ++i)
        for (int j = 0; j < 99; ++j)
        {
            files[i][j] = (fileinfo_t*) calloc(1, sizeof(fileinfo_t));
            if (files[i][j] == NULL)
                return -1;
        }

    if (! s_dir_exists(globals.settings.outdir)) secure_mkdir(globals.settings.outdir, globals.access_rights);

    change_directory(globals.settings.outdir);

    for (int i = 0; i < 81 && globals.aobpath[i] != NULL; ++i)
    {
      if (globals.veryverbose)
         foutput("%s%d%s\n", INF "Extracting audio for AOB n°", i+1, ".");

      WaveData info;

      errno = 0;

      if (use_ifo_files)
      {
          unsigned long s = strlen(globals.aobpath[i]);

          char* ifo_filename = strdup(globals.aobpath[i]);
          ifo_filename[s - 5] = '0';
          ifo_filename[s - 3] = 'I';
          ifo_filename[s - 2] = 'F';
          ifo_filename[s - 1] = 'O';

          FILE* file = NULL;
          if ((file = fopen(ifo_filename, "rb")) == NULL)
          {
              EXIT_ON_RUNTIME_ERROR_VERBOSE("IFO file could not be opened.")
          }

          if (globals.debugging)
              foutput( INF "Reading IFO file %s\n", ifo_filename);

          uint8_t buf[2048 * 3] = {0};

          int nbytesread = fread(buf, 1, 3 * 2048, file);

          if (globals.veryverbose)
              printf( INF "Read IFO file: %d bytes\n", nbytesread);

          fclose(file);

          if (memcmp(buf, "DVDAUDIO-ATS", 12) != 0)
          {
              foutput(ERR "%s is not an ATSI file (ATS_XX_0.IFO)\n", ifo_filename);
              clean_exit(EXIT_FAILURE);
          }

          printf("%c", '\n');

          /* now scan tracks to be extracted */

          scan_ats_ifo(&files[i][0], &buf[0]);

          free(ifo_filename);
      }

      get_ats_audio_i(i, files, &info);

      if (globals.veryverbose)
              foutput("%s\n", INF "Reached ead of AOB.");
    }
    
    if (globals.fixwav_prepend)
        audio_extraction_layout(files);

    return(errno);
}

int ats2wav(short ngroups_scan, const char* audiots_dir, const char *outdir, const extractlist* extract)
{

    //get_ats;
          


    return(EXIT_SUCCESS);
}
