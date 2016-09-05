/*

File:    ats.c
Purpose: Create an Audio Titleset

dvda-author  - Author a DVD-Audio DVD

(C) Dave Chapman <dave@dchapman.com> 2005

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
#include <math.h>
#include <errno.h>
#include "structures.h"
#include "audio2.h"
#include "c_utils.h"
#include "auxiliary.h"
#include "commonvars.h"
#include "ats.h"


extern globalData globals;
extern uint8_t channels[21];
static uint8_t pack_start_code[4]={0x00,0x00,0x01,0xBA};
static uint8_t system_header_start_code[4]={0x00,0x00,0x01,0xBB};
static uint8_t program_mux_rate_bytes[3]={0x01,0x89,0xc3};
static uint8_t pack_stuffing_length_byte[1]={0xf8};


/* pack_scr was taken from mplex (part of the mjpegtools) */
#define MARKER_MPEG2_SCR 1
inline void pack_scr(uint8_t scr_bytes[6],uint64_t SCR_base, uint16_t SCR_ext)
{

    uint8_t temp;
    uint64_t msb, lsb;

    msb = (SCR_base>> 32) & 1;
    lsb = SCR_base & (0xFFFFFFFF);

    temp = (MARKER_MPEG2_SCR << 6) | (msb << 5) | ((lsb >> 27) & 0x18) | 0x4 | ((lsb >> 28) & 0x3);
    scr_bytes[0]=temp;
    temp = (lsb & 0x0ff00000) >> 20;
    scr_bytes[1]=temp;
    temp = ((lsb & 0x000f8000) >> 12) | 0x4 | ((lsb & 0x00006000) >> 13);
    scr_bytes[2]=temp;
    temp = (lsb & 0x00001fe0) >> 5;
    scr_bytes[3]=temp;
    temp = ((lsb & 0x0000001f) << 3) | 0x4 | ((SCR_ext & 0x00000180) >> 7);
    scr_bytes[4]=temp;
    temp = ((SCR_ext & 0x0000007F) << 1) | 1;
    scr_bytes[5]=temp;
}

inline static void decode_SCR(uint8_t* scr_bytes, uint64_t* SCRint_ptr)
{
    uint64_t SCR_base = (scr_bytes[0] >> 3 & 0x7) << 30 | (scr_bytes[0] & 0x3) << 28 |
                                                                                  scr_bytes[1] << 20 | (scr_bytes[2] >> 3 & 0x1F) << 15 | (scr_bytes[2] & 0x3) << 13 |
                                                                                                                                                                  scr_bytes[3] << 5  | scr_bytes[4] >> 3;

    uint64_t SCR_ext = (scr_bytes[4] & 0x3) << 7 | scr_bytes[5] >> 1;

    *SCRint_ptr = SCR_base * 300 + SCR_ext;

    if (globals.logdecode) fprintf(aob_log, "NA;SCR_base;%" PRIu64 ";SCR_ext;%" PRIu64 ";SCR_int;%" PRIu64 "\n", SCR_base, SCR_ext, *SCRint_ptr);
}

#if 0 // not used
inline void pack_pts_dts(uint8_t PTS_DTS_data[10],uint32_t pts, uint32_t dts)
{
    uint8_t p3,p2,p1,p0,d3,d2,d1,d0;

    p3=(pts&0xff000000)>>24;
    p2=(pts&0x00ff0000)>>16;
    p1=(pts&0x0000ff00)>>8;
    p0=pts&0xff;
    d3=(dts&0xff000000)>>24;
    d2=(dts&0x00ff0000)>>16;
    d1=(dts&0x0000ff00)>>8;
    d0=dts&0xff;

    PTS_DTS_data[0]=0x31|((p3&0xc0)>>5);              // [32],31,30
    PTS_DTS_data[1]=((p3&0x3f)<<2)|((p2&0xc0)>>6);    // 29,28,27,26,25,24 ,23,22
    PTS_DTS_data[2]=((p2&0x3f)<<2)|((p1&0x80)>>6)|1;  // 21,20,19,18,17,16, 15
    PTS_DTS_data[3]=((p1&0x7f)<<1)|((p0&0x80)>>7);    // 14,13,12,11,10,9,8, 7
    PTS_DTS_data[4]=((p0&0x7f)<<1)|1;                 // 6,5,4,3,2,1,0
    PTS_DTS_data[5]=0x11|((d3&0xc0)>>5);              // [32],31,30
    PTS_DTS_data[6]=((d3&0x3f)<<2)|((d2&0xc0)>>6);    // 29,28,27,26,25,24 ,23,22
    PTS_DTS_data[7]=((d2&0x3f)<<2)|((d1&0x80)>>6)|1;  // 21,20,19,18,17,16, 15
    PTS_DTS_data[8]=((d1&0x7f)<<1)|((d0&0x80)>>7);    // 14,13,12,11,10,9,8, 7
    PTS_DTS_data[9]=((d0&0x7f)<<1)|1;                 // 6,5,4,3,2,1,0
}
#endif

inline static void pack_pts(uint8_t PTS_DTS_data[5], uint32_t pts)
{
    uint8_t p3,p2,p1,p0;

    p3=(pts&0xff000000)>>24;
    p2=(pts&0x00ff0000)>>16;
    p1=(pts&0x0000ff00)>>8;
    p0=pts&0xff;

    PTS_DTS_data[0]=0x21|((p3&0xc0)>>5);              // [32],31,30
    PTS_DTS_data[1]=((p3&0x3f)<<2)|((p2&0xc0)>>6);    // 29,28,27,26,25,24 ,23,22
    PTS_DTS_data[2]=((p2&0x3f)<<2)|((p1&0x80)>>6)|1;  // 21,20,19,18,17,16, 15
    PTS_DTS_data[3]=((p1&0x7f)<<1)|((p0&0x80)>>7);    // 14,13,12,11,10,9,8, 7
    PTS_DTS_data[4]=((p0&0x7f)<<1)|1;                 // 6,5,4,3,2,1,0
}

// P[5] is PTS_DTS_data

inline static void decode_pts(uint8_t P[5], uint32_t* pts_ptr)
{

    *pts_ptr = (P[0] >> 1 & 0x3) << 30 | P[1] << 22 | (P[2] >> 1) << 15 | P[3] << 7 | P[4] >> 1;

    if (globals.logdecode)
        fprintf(aob_log, "PTS;%" PRIu32 "\n", *pts_ptr);
}

inline static void write_pack_header(FILE* fp,  uint64_t SCRint)
{

    uint8_t scr_bytes[6];


    /*200806  patch Fabrice Nicol <fabnicol@users.sourceforge.net>
    * floor returns a double (type conversion error).
    * SCRint=floor(SCR);
    */

    if (globals.maxverbose)
    {
        EXPLAIN("%s%d\n", " PACK HEADER into file, size is: ", 4+6+3+1);
    }

    /* offset_count += 4 */ fwrite(pack_start_code,4,1,fp);
    pack_scr(scr_bytes,(SCRint/300),(SCRint%300));
    /* offset_count += 6 */ fwrite(scr_bytes,6,1,fp);
    /* offset_count += 3 */ fwrite(program_mux_rate_bytes,3,1,fp);
    /* offset_count += */   fwrite(pack_stuffing_length_byte,1,1,fp);
}


inline static void test_field(uint8_t* tab__, uint8_t* tab, int size,const char* label, FILE* fp, _Bool write)
{
    uint64_t offset = ftello(fp);
    /* offset_count += */   fread(tab__, size,1,fp);
    if (globals.logdecode) fprintf(aob_log, "%" PRIu64 ";%s;", offset, label);
    if (! globals.logdecode) return;
    if (memcmp(tab__, tab, size) == 0)
    {
            fprintf(aob_log, "%s", "OK\n");
    }
    else
    {
        if (write) hex2file(aob_log, tab__, size);
        fprintf(aob_log, "%s", "\n");
    }
}

inline static void rw_field(uint8_t* tab, int size,const char* label, FILE* fp)
{
    uint64_t offset = ftello(fp);
    /* offset_count += */   fread(tab, size,1,fp);
    hex2file(aob_log, tab, size);

    if (! globals.logdecode) return;
    fprintf(aob_log, "%" PRIu64 ";%s;", offset, label);
    fprintf(aob_log, "%s", "\n");
}


#define CHECK_FIELD(X) test_field(X##__, X, sizeof(X), #X, fp, true);
#define CHECK_FIELD_NOWRITE(X) test_field(X##__, X, sizeof(X), #X, fp, false);
#define RW_FIELD(X) rw_field(X, sizeof(X), #X, fp);




inline static void read_pack_header(FILE* fp,  uint64_t* SCRint_ptr)
{

    uint8_t scr_bytes[6]={0};
    uint8_t pack_stuffing_length_byte__[1]={0};
    uint8_t pack_start_code__[4]={0};
    uint8_t program_mux_rate_bytes__[3]={0};
    open_aob_log();

    CHECK_FIELD(pack_start_code)

    RW_FIELD(scr_bytes)

    decode_SCR(scr_bytes, SCRint_ptr);

    CHECK_FIELD(program_mux_rate_bytes)
    CHECK_FIELD(pack_stuffing_length_byte)

    close_aob_log();
}

inline static void write_system_header(FILE* fp)
{
    uint8_t header_length[2]={0x00,0x0c};
    uint8_t rate_bound[3]={0x80,0xc4,0xe1};
    uint8_t audio_bound[1]={0x04};
    uint8_t video_bound[1]={0xa0};
    uint8_t packet_rate_restriction_flag[1]={0x7f};
    uint8_t stream_info1[3]={0xb8, 0xc0, 0x40};
    uint8_t stream_info2[3]={0xbd, 0xe0, 0x0a};

    if (globals.maxverbose) EXPLAIN("%s%d\n","WRITE SYSTEM HEADER, size is: ", 4+2+3+1+1+1+3+3)

            /* offset_count += 4 */ fwrite(system_header_start_code,4,1,fp);
    /* offset_count += 2 */ fwrite(header_length,2,1,fp);
    /* offset_count += 3 */ fwrite(rate_bound,3,1,fp);
    /* offset_count += */   fwrite(audio_bound,1,1,fp);
    /* offset_count += */   fwrite(video_bound,1,1,fp);
    /* offset_count += */   fwrite(packet_rate_restriction_flag,1,1,fp);
    /* offset_count += 3 */ fwrite(stream_info1,3,1,fp);
    /* offset_count += 3 */ fwrite(stream_info2,3,1,fp);
}

inline static void read_system_header(FILE* fp)
{
    uint8_t header_length[2]={0x00,0x0c};
    uint8_t rate_bound[3]={0x80,0xc4,0xe1};
    uint8_t audio_bound[1]={0x04};
    uint8_t video_bound[1]={0xa0};
    uint8_t packet_rate_restriction_flag[1]={0x7f};
    uint8_t stream_info1[3]={0xb8, 0xc0, 0x40};
    uint8_t stream_info2[3]={0xbd, 0xe0, 0x0a};

    uint8_t header_length__[2]={0};
    uint8_t rate_bound__[3]={0};
    uint8_t audio_bound__[1]={0};
    uint8_t video_bound__[1]={0};
    uint8_t packet_rate_restriction_flag__[1]={0};
    uint8_t stream_info1__[3]={0};
    uint8_t stream_info2__[3]={0};
    uint8_t system_header_start_code__[4]={0};

    open_aob_log();

    /* offset_count += 4 */ CHECK_FIELD(system_header_start_code)
    /* offset_count += 2 */ CHECK_FIELD(header_length)
    /* offset_count += 3 */ CHECK_FIELD(rate_bound)
    /* offset_count += */   CHECK_FIELD(audio_bound)
    /* offset_count += */   CHECK_FIELD(video_bound)
    /* offset_count += */   CHECK_FIELD(packet_rate_restriction_flag)
    /* offset_count += 3 */ CHECK_FIELD(stream_info1)
    /* offset_count += 3 */ CHECK_FIELD(stream_info2)

    close_aob_log();
}


inline static void write_pes_padding(FILE* fp,uint16_t length)
{
    uint8_t packet_start_code_prefix[3]={0x00,0x00,0x01};
    uint8_t stream_id[1]={0xbe};
    uint8_t length_bytes[2];
    uint8_t ff_buf[2048];

    if (globals.maxverbose) EXPLAIN("%s%d\n", "PES padding, size is: ", 3+1+2+length)

            memset(ff_buf,0xff,sizeof(ff_buf));

    length-=6; // We have 6 bytes of PES header.

    length_bytes[0]=(length&0xff00)>>8;
    length_bytes[1]=(length&0xff);

    /* offset_count += 3 */ fwrite(packet_start_code_prefix,3,1,fp);
    /* offset_count += 1 */ fwrite(&stream_id,1,1,fp);
    /* offset_count += 2 */ fwrite(length_bytes,2,1,fp);
    /* offset_count += length */ fwrite(ff_buf,length,1,fp);
}

inline static void read_pes_padding(FILE* fp,uint16_t length)
{
    uint8_t packet_start_code_prefix[3]={0x00,0x00,0x01};
    uint8_t stream_id[1]={0xbe};
    uint8_t length_bytes[2];
    uint8_t ff_buf[2048];

    uint8_t packet_start_code_prefix__[3]={0};
    uint8_t stream_id__[1]={0};
    uint8_t length_bytes__[2]={0};
    uint8_t ff_buf__[2048]={0};

    memset(ff_buf,0xff,sizeof(ff_buf));

    length-=6; // We have 6 bytes of PES header.

    open_aob_log();

    /* offset_count += 3 */ CHECK_FIELD(packet_start_code_prefix)
            /* offset_count += 1 */ CHECK_FIELD(stream_id);

    length_bytes[0]=(length&0xff00)>>8;
    length_bytes[1]=(length&0xff);

    /* offset_count += 2 */ CHECK_FIELD(length_bytes);
    /* offset_count += length */ CHECK_FIELD_NOWRITE(ff_buf)

            close_aob_log();
}


inline static void write_audio_pes_header(FILE* fp, uint16_t PES_packet_len, uint8_t extension_flag, uint64_t PTS)
{
    uint8_t packet_start_code_prefix[3]={0x00,0x00,0x01};
    uint8_t stream_id[1]={0xbd}; // private_stream_1
    uint8_t PES_packet_len_bytes[2];
    uint8_t flags1[1]={0x81};  // various flags - original_or_copy=1
    uint8_t flags2[1]={0};  // various flags - contains pts_dts_flags and extension_flav
    uint8_t PES_header_data_length[1]={0};
    uint8_t PES_extension_flags[1]={0x1e};  // PSTD_buffer_flag=1
    uint8_t PTS_DTS_data[5];
    uint8_t PSTD_buffer_scalesize[2];
    uint16_t PSTD=10240/1024;

    if (globals.maxverbose) EXPLAIN("%s%d%s%d%s%d\n","WRITE AUDIO PES HEADER, PES_packet_len: ", PES_packet_len, " extension_flag: ", extension_flag, " size is: ", 3+1+2+1+1+1+5+(extension_flag)*3)

            /* Set PTS_DTS_flags in flags2 to 2 (top 2 bits) - PTS only*/
            flags2[0]=(2<<6);

    if (extension_flag)
    {
        PES_header_data_length[0]+=3;
    }

    PES_header_data_length[0]+=5;        // PTS only (and PSTD)

    flags2[0]|=extension_flag;  // PES_extension_flag=1

    PES_packet_len_bytes[0]=(PES_packet_len&0xff00)>>8;
    PES_packet_len_bytes[1]=PES_packet_len&0xff;

    /* offset_count += 3 */ fwrite(packet_start_code_prefix,3,1,fp);
    /* offset_count += */   fwrite(stream_id,1,1,fp);
    /* offset_count += 2 */ fwrite(PES_packet_len_bytes,2,1,fp);
    /* offset_count += */   fwrite(flags1,1,1,fp);
    /* offset_count += */   fwrite(flags2,1,1,fp);
    /* offset_count += */   fwrite(PES_header_data_length,1,1,fp);

    pack_pts(PTS_DTS_data, PTS);
    /* PATCH LOCATION 1: +0x18=24 */
    /* offset_count += 5 */ fwrite(PTS_DTS_data,5,1,fp);

    if (extension_flag)
    {
        PSTD_buffer_scalesize[0]=0x60|((PSTD&0x1f00)>>8);
        PSTD_buffer_scalesize[1]=PSTD&0xff;

        /* offset_count += */   fwrite(PES_extension_flags,1,1,fp);
        /* offset_count += 2 */ fwrite(PSTD_buffer_scalesize,2,1,fp);
    }
}

inline static uint16_t read_audio_pes_header(FILE* fp, uint8_t extension_flag, uint32_t* PTS_ptr)
{
    uint8_t packet_start_code_prefix[3]={0x00,0x00,0x01};
    uint8_t stream_id[1]={0xbd}; // private_stream_1
    uint8_t PES_packet_len_bytes[2] ={0};
    uint8_t flags1[1]={0x81};  // various flags - original_or_copy=1
    uint8_t flags2[1]={0};  // various flags - contains pts_dts_flags and extension_flav
    uint8_t PES_header_data_length[1]={0};
    uint8_t PES_extension_flags[1]={0x1e};  // PSTD_buffer_flag=1
    uint8_t PTS_DTS_data[5];
    uint8_t PSTD_buffer_scalesize[2];

    uint8_t packet_start_code_prefix__[3]={0};
    uint8_t stream_id__[1]={0}; // private_stream_1
    uint8_t flags1__[1]={0};  // various flags - original_or_copy=1
    uint8_t flags2__[1]={0};  // various flags - contains pts_dts_flags and extension_flav
    uint8_t PES_header_data_length__[1]={0};
    uint8_t PES_extension_flags__[1]={0};  // PSTD_buffer_flag=1
    uint8_t PSTD_buffer_scalesize__[2]={0};

    uint16_t PES_packet_len = 0;

    uint16_t PSTD=10; // =10240/1024;

    /* Set PTS_DTS_flags in flags2 to 2 (top 2 bits) - PTS only*/
    flags2[0]=(2<<6);

    if (extension_flag)
    {
        PES_header_data_length[0]+=3;
    }

    PES_header_data_length[0]+=5;        // PTS only (and PSTD)

    flags2[0]|=extension_flag;  // PES_extension_flag=1

    open_aob_log();

    CHECK_FIELD(packet_start_code_prefix);
    CHECK_FIELD(stream_id);
    RW_FIELD(PES_packet_len_bytes);

    PES_packet_len = PES_packet_len_bytes[0] << 8 | PES_packet_len_bytes[1];

    CHECK_FIELD(flags1);
    CHECK_FIELD(flags2);
    CHECK_FIELD(PES_header_data_length);

    RW_FIELD(PTS_DTS_data);

    decode_pts(PTS_DTS_data, PTS_ptr);

    if (extension_flag)
    {
        PSTD_buffer_scalesize[0]=0x60|((PSTD&0x1f00)>>8);
        PSTD_buffer_scalesize[1]=PSTD&0xff;

        CHECK_FIELD(PES_extension_flags);
        CHECK_FIELD(PSTD_buffer_scalesize);
    }

    close_aob_log();

    return(PES_packet_len);
}


#define FIRST_PACK   0
#define LAST_PACK    1
#define MIDDLE_PACK  2

inline static void write_lpcm_header(FILE* fp, int header_length,fileinfo_t* info, int64_t pack_in_title, uint8_t counter, uint8_t position)
{
    uint8_t sub_stream_id[1]={0xa0};
    uint8_t continuity_counter[1]={0x00};
    uint8_t LPCM_header_length[2];
    uint8_t first_access_unit_pointer[2];
    uint8_t unknown1[1]={0x10};   // e.g. 0x10 for stereo, 0x00 for surround
    uint8_t sample_size[1]={0x0f};  // 0x0f=16-bit, 0x1f=20-bit, 0x2f=24-bit
    uint8_t sample_rate[1]={0x0f};  // 0x0f=48KHz, 0x1f=96KHz, 0x2f=192KHz,0x8f=44.1KHz, 0x9f=88.2KHz, 0xaf=176.4KHz
    uint8_t unknown2[1]={0x00};
    uint8_t channel_assignment;  // The channel assignment - 0=C; 1=L,R; 17=L,R,C,lfe,Ls,Rs
    uint8_t unknown3[1]={0x80};

    uint8_t zero[16]={0};

    int frame_offset;
    uint64_t bytes_written;
    uint64_t frames_written;

    continuity_counter[0]=counter;
    uint8_t high_nibble = 0;

    switch (info->samplerate)
    {
    case 48000:
        high_nibble=0;
        break;
    case 96000:
        high_nibble=0x1;
        break;
    case 192000:
        high_nibble=0x2;
        break;
    case 44100:
        high_nibble=0x8;
        break;
    case 88200:
        high_nibble=0x9;
        break;
    case 176400:
        high_nibble=0xa;
        break;
    default:
        break;
    }

    if (info->channels < 3)
    {
        unknown1[0]=0x10;

        switch (info->bitspersample)
        {
        case 16:
            sample_size[0]=0x0f;
            break;

        case 24:
            sample_size[0]=0x2f;
            break;
        default:
            break;
        }

        sample_rate[0] = high_nibble << 4 | 0xf;
    }
    else
    {
        unknown1[0]=0x00;
        switch (info->bitspersample)
        {
        case 16:
            sample_size[0]=0x00;
            break;

        case 24:
            sample_size[0]=0x22;
            break;
        default:
            break;
        }

        sample_rate[0] = high_nibble << 4 | high_nibble;
    }

    channel_assignment = info->cga;

    switch (position)
    {
    case  FIRST_PACK:
        frames_written=0;
        bytes_written=0;
        frame_offset=header_length-1;
        break;

    case  LAST_PACK:
        if (info->bitspersample == 16)
        {
            frame_offset=0;
            break;
        }
        // else no break;

    case  MIDDLE_PACK:
        bytes_written=(pack_in_title * info->lpcm_payload) - info->firstpackdecrement;
        // e.g., for 4ch, First pack is 1984, rest are 2000
        frames_written = bytes_written / info->bytesperframe;
        if (frames_written * info->bytesperframe < bytes_written)
        {
            frames_written++;
        }

        frame_offset=(frames_written * info->bytesperframe) - bytes_written + header_length - 1;
    }

    LPCM_header_length[0] = (header_length & 0xff00) >> 8;
    LPCM_header_length[1] = header_length & 0xff;

    first_access_unit_pointer[0]=(frame_offset&0xff00)>>8;
    first_access_unit_pointer[1]=frame_offset&0xff;

    /* offset_count += */   fwrite(sub_stream_id,1,1,fp);
    /* offset_count += */   fwrite(continuity_counter,1,1,fp);
    /* offset_count += 2 */ fwrite(LPCM_header_length,2,1,fp);
    /* offset_count += 2 */ fwrite(first_access_unit_pointer,2,1,fp);
#ifdef DEBUG
    fprintf(stderr, "ftell=%lu\n", ftell(fp));
#endif
    /* offset_count += */   fwrite(unknown1,1,1,fp);
    /* offset_count += */   fwrite(sample_size,1,1,fp);
    /* offset_count += */   fwrite(sample_rate,1,1,fp);
    /* offset_count += */   fwrite(unknown2,1,1,fp);
    /* offset_count += */   fwrite(&channel_assignment,1,1,fp);
    /* offset_count += */   fwrite(unknown3,1,1,fp);
    /* offset_count += (header_length-8) */  fwrite(zero,header_length-8,1,fp);
    if (globals.maxverbose) EXPLAIN("%s%d%s\n","WRITE LPCM HEADER for ",info->bitspersample, " bits")
}

inline static int read_lpcm_header(FILE* fp, fileinfo_t* info, int64_t pack_in_title, uint8_t position)
{
    uint8_t sub_stream_id[1]={0xa0};
    uint8_t continuity_counter[1]={0x00};
    uint8_t LPCM_header_length[2];
    uint8_t first_access_unit_pointer[2];
    uint8_t unknown1[1]={0x10};   // e.g. 0x10 for stereo, 0x00 for surround
    uint8_t sample_size[1]={0x0f};  // 0x0f=16-bit, 0x1f=20-bit, 0x2f=24-bit
    uint8_t sample_rate[1]={0x0f};  // 0x0f=48KHz, 0x1f=96KHz, 0x2f=192KHz,0x8f=44.1KHz, 0x9f=88.2KHz, 0xaf=176.4KHz
    uint8_t unknown2[1]={0x00};
    uint8_t channel_assignment[1];  // The channel assignment - 0=C; 1=L,R; 17=L,R,C,lfe,Ls,Rs
    uint8_t unknown3[1]={0x80};

    uint8_t sub_stream_id__[1]={0};
    uint8_t unknown1__[1]={0};   // e.g. 0x10 for stereo, 0x00 for surround
    uint8_t unknown2__[1]={0};
    uint8_t unknown3__[1]={0};

    int frame_offset = 0;
    uint64_t bytes_written = 0;
    uint64_t frames_written = 0;

    open_aob_log();

    /* offset_count += */   CHECK_FIELD(sub_stream_id);
    /* offset_count += */   RW_FIELD(continuity_counter);
    /* offset_count += 2 */ RW_FIELD(LPCM_header_length);

    uint header_length =  LPCM_header_length[0] << 8 | LPCM_header_length[1];

    /* offset_count += 2 */ RW_FIELD(first_access_unit_pointer);

    /* offset_count += */   CHECK_FIELD(unknown1);
    /* offset_count += */   RW_FIELD(sample_size);
    /* offset_count += */   RW_FIELD(sample_rate);

    uint8_t high_nibble = (sample_rate[0] & 0xf0) >> 4;

    switch(high_nibble)
    {
    case 0:
        info->samplerate = 48000;
        break;
    case 0x1:
        info->samplerate = 96000;
        break;
    case 0x2:
        info->samplerate = 192000;
        break;
    case 0x8:
        info->samplerate = 44100;
        break;
    case 0x9:
        info->samplerate = 88200;
        break;
    case 0xa:
        info->samplerate = 176400;
        break;
    default:
        break;
    }

    if (sample_size[0] == 0x0f || sample_size[0] == 0x2f)
    {
        unknown1[0] = 0x10;

        if ((sample_rate[0] & 0xf) != 0xf)
        {
            if (globals.logdecode) fprintf(aob_log, "%s", "NA;coherence_test;sample_rate and sample_size are incoherent (no 0xf lower nibble)\n");
        }

    }
    else
        if (sample_size[0] == 0x00 || sample_size[0] == 0x22)
        {
            unknown1[0] = 0x00;

            if ((sample_rate[0] & 0xf) != high_nibble)
            {
                if (globals.logdecode) fprintf(aob_log, "%s", "NA;coherence_test;sample_rate and sample_size are incoherent (lower nibble != higher nibble)\n");
            }
        }

    info->bitspersample = 16 + (sample_size[0] & 0x10) * 4;

    /* offset_count += */   CHECK_FIELD(unknown2);
    /* offset_count += */   RW_FIELD(channel_assignment);

    info->cga = channel_assignment[0];

    info->channels = channels[channel_assignment[0]];

    /* info->PTS_length, info->numsamples and info->numbytes will be unusable, other info fileds OK */

    calc_info(info);

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
        Group 1 	        Group 2
*/

    switch (position)
    {
    case  FIRST_PACK:
        frames_written=0;
        bytes_written=0;
        frame_offset=header_length-1;

        if (header_length != info->firstpack_lpcm_headerquantity)
        {
            if (globals.logdecode) fprintf(aob_log, "NA;info->firstpack_lpcm_headerquantity;%d;%d", info->firstpack_lpcm_headerquantity, header_length);
        }
        break;

    case  LAST_PACK:
        if (header_length != info->lastpack_lpcm_headerquantity)
        {
            if (globals.logdecode) fprintf(aob_log, "NA;info->lastpack_lpcm_headerquantity;%d;%d", info->lastpack_lpcm_headerquantity, header_length);
        }

        if (info->bitspersample == 16)
        {
            frame_offset=0;
            break;
        }
        // else no break;

    case  MIDDLE_PACK:
        if (header_length != info->midpack_lpcm_headerquantity)
        {
            if (globals.logdecode) fprintf(aob_log, "NA;info->midpack_lpcm_headerquantity;%d;%d", info->midpack_lpcm_headerquantity, header_length);
        }
        bytes_written=(pack_in_title*info->lpcm_payload)-info->firstpackdecrement;
        // e.g., for 4ch, First pack is 1984, rest are 2000
        frames_written=bytes_written/info->bytesperframe;
        if (frames_written*info->bytesperframe < bytes_written)
        {
            frames_written++;
        }
        frame_offset=(frames_written*info->bytesperframe)-bytes_written+header_length-1;
    }

    if (globals.veryverbose)
        foutput("titlepack: %ld bytes written: %ld\nsample rate: %d bit rate: %d channel asignment: %d\n",
                pack_in_title, bytes_written, info->samplerate, info->bitspersample, info->cga);

    if (first_access_unit_pointer[0] != (frame_offset & 0xff00) >> 8)
    {
        foutput("%s\n", "first_access_unit_pointer: inaccurate bit 0");
        if (globals.logdecode) fprintf(aob_log, "NA;first_access_unit_pointer bit 0 inacurate; %d instead of %d;", first_access_unit_pointer[0], (frame_offset & 0xff00) >> 8);
    }

    if (first_access_unit_pointer[1] != (frame_offset & 0xff))
    {
        foutput("%s\n", "first_access_unit_pointer: inaccurate bit 1");
        if (globals.logdecode) fprintf(aob_log, "NA;first_access_unit_pointer bit 0 inacurate; %d instead of %d;", first_access_unit_pointer[1], (frame_offset & 0xff00) >> 8);
    }

    /* offset_count += */   CHECK_FIELD(unknown3);

    uint8_t zero[header_length-8];
    uint8_t zero__[header_length-8];
    memset(zero, 0, header_length-8);
    memset(zero__, 0, header_length-8);

    /* offset_count += (header_length-8) */ CHECK_FIELD_NOWRITE(zero);

    close_aob_log();
    return header_length;
}

inline static uint64_t calc_PTS_start(fileinfo_t* info, uint64_t pack_in_title)
{
    double PTS;
    uint64_t PTSint;
    uint64_t bytes_written;

    if (pack_in_title==0)
    {
        PTS=585.0;
    }
    else
    {
        bytes_written=(pack_in_title*info->lpcm_payload)-info->firstpackdecrement;
        // e.g., for 4ch, First pack is 1984, rest are 2000
        PTS=((bytes_written*90000.0)/(info->bytespersecond*1.0))+585.0;
    }

    PTSint=floor(PTS);
    return(PTSint);
}



inline static uint64_t calc_SCR(fileinfo_t* info, uint64_t pack_in_title)
{
    double SCR;
    uint64_t SCRint;


    if (info->bytespersecond == 0)
    {
        foutput(""ANSI_COLOR_RED"[WAR]"ANSI_COLOR_RESET"  file %s has bytes per second=0\n", info->filename);
        return 0;
    }


    SCR=(pack_in_title <= 4 ? pack_in_title : 4)*(2048.0*8.0*90000.0*300.0)/10080000.0;

    if (pack_in_title>=4)
    {
        SCR+=((info->SCRquantity*300.0*90000.0)/info->bytespersecond)-42.85;
    }
    if (pack_in_title>=5)
    {
        SCR+=(pack_in_title-4.0)*((info->lpcm_payload*300.0*90000.0)/info->bytespersecond);
    }

    SCRint=floor(SCR);

    return(SCRint);
}


inline static uint32_t calc_PTS(fileinfo_t* info, uint64_t pack_in_title)
{
    double PTS;
    uint32_t PTSint;
    uint64_t bytes_written;
    uint64_t frames_written;


    if (pack_in_title==0)
    {
        PTS=585.0;
    }
    else
    {
        bytes_written=(pack_in_title*info->lpcm_payload)-info->firstpackdecrement;
        // e.g., for 4ch, First pack is 1984, rest are 2000
        frames_written=bytes_written/info->bytesperframe;
        if (frames_written*info->bytesperframe < bytes_written)
        {
            frames_written++;
        }
        // 1 48Khz-based frame is 1200Hz, 1 44.1KHz frame is 1102.5Hz
        PTS=((frames_written*info->bytesperframe*90000.0)/(info->bytespersecond*1.0))+585.0;
    }

    PTSint=(uint32_t) floor(PTS);
    return(PTSint);
}


inline static int write_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf, uint32_t bytesinbuffer, uint64_t pack_in_title, _Bool start_of_file)
{
    uint64_t PTS;
    uint64_t SCR;
    int audio_bytes;
    static int cc;  // Continuity counter - reset to 0 when pack_in_title=0


    PTS=calc_PTS(info,pack_in_title);
    SCR=calc_SCR(info,pack_in_title);

    if (pack_in_title==0) // 53  + info->firstpack_lpcm_headerquantity + (info->firstpack_pes_padding != 0)? info->firstpack_pes_padding + 6 + info->lpcm_payload - info->firstpackdecrement
                         //  = 59 + 1995 = 2054 if info->firstpack_pes_padding != 0
                         //  = 2048 everywhere except for 16/6ch or 24/4ch.
    {
        cc=0;            // First packet in title
        foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Writing first packet - pack=%"PRIu64", bytesinbuffer=%d\n",pack_in_title,bytesinbuffer);
        write_pack_header(fp,SCR); //+14
        write_system_header(fp); //+18

        write_audio_pes_header(fp,info->firstpack_audiopesheaderquantity,1,PTS); //+17
        audio_bytes = info->lpcm_payload - info->firstpackdecrement;
        write_lpcm_header(fp,info->firstpack_lpcm_headerquantity,info,pack_in_title,cc,FIRST_PACK);//info->firstpack_lpcm_headerquantity+4
        /* offset_count+= */ fwrite(audio_buf,1,audio_bytes,fp);

        if (info->firstpack_pes_padding > 0)
        {
            write_pes_padding(fp,info->firstpack_pes_padding);//+6+info->firstpack_pes_padding
        }
    }
    else if (bytesinbuffer < info->lpcm_payload)   // Last packet in title 2038+info->lastpack_lpcm_headerquantity
    {
        // 2048,2048,2044,2048,2048,2042 | 2048,2048,2052,2046,2052,2052 : faulty

        foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Writing last packet - pack=%lu, bytesinbuffer=%d\n", pack_in_title, bytesinbuffer);
        audio_bytes=bytesinbuffer;
        write_pack_header(fp,SCR); //+14
        write_audio_pes_header(fp,info->lastpack_audiopesheaderquantity+audio_bytes,0,PTS);  // +14
        write_lpcm_header(fp,info->lastpack_lpcm_headerquantity,info,pack_in_title,cc,LAST_PACK); // +info->lastpack_lpcm_headerquantity +4
        /* offset_count+= */ fwrite(audio_buf,1,audio_bytes,fp);
        // PATCH Dec. 2013
        // Lee Feldkamp corrected formula write_pes_padding(fp,2048-28-info->midpack_lpcm_headerquantity-4-audio_bytes+gamma);
        // reverting to Old-style Dave code:
        write_pes_padding(fp,2006-audio_bytes);//2012-audio_bytes

    }
    else   			// A middle packet in the title: 38 + (info->midpack_lpcm_headerquantity+info->midpack_pes_padding+info->lpcm_payload)
                    // = 32 + 2016 = 2048 or [faulty?] 2054 if (info->midpack_pes_padding > 0) ie 24b Ch 3+ and 16b 6 Ch.
    {
        audio_bytes=info->lpcm_payload;
        write_pack_header(fp,SCR); //+14
        write_audio_pes_header(fp,info->midpack_audiopesheaderquantity,0,PTS);//+14

        //PATCH Dec. 2013: reset continuity counters at start of file
        if (start_of_file) cc=0;

        write_lpcm_header(fp,info->midpack_lpcm_headerquantity,info,pack_in_title,cc,MIDDLE_PACK); //info->midpack_lpcm_headerquantity+4
        /* offset_count+= */ fwrite(audio_buf,1,audio_bytes,fp);
        if (info->midpack_pes_padding > 0) write_pes_padding(fp,info->midpack_pes_padding);//info->midpack_pes_padding +6
    }
/*
    {{   	2000, 16,  1984,  2010,	2028, 22, 11, 16, 10, 0, 0 },
       {	2000, 16,  1984,  2010,	2028, 22, 11, 16, 10, 0, 0 },
       { 	2004, 24,  1980,  2010,	2028, 22, 15, 12,  6, 0, 0 },
       { 	2000, 16,  1980,  2010,	2028, 22, 11, 16, 10, 0, 0 },
       { 	2000, 20,  1980,  2010, 2028, 22, 15, 16, 10, 0, 0 },
       { 	1992, 24,  1992, 1993,  2014, 22, 10, 10,  4,17,14}},
   // 24-bit table
   {{    	2004, 24,  1980,  2010,	2028, 22, 15, 12, 10, 0, 0 },
       { 	2004, 24,  1980,  2010,	2028, 22, 15, 12, 10, 0, 0 },
       { 	1998, 18,  1980,  2010,	2026, 22, 15, 16, 14, 0, 2 },
       { 	1992, 24,  1968,  1993,	2014, 22, 10, 10,  8,17, 14 },
       { 	1980,  0,  1980,  2010, 2008, 22, 15, 16, 14, 0, 20 },
       { 	1980,  0,  1980,  2010, 2008, 22, 15, 16, 14, 0, 20 }}


#define X T[table_index][info->channels-1]

info->sampleunitsize=
       (table_index==1)? info->channels*6 :
                         ((info->channels > 2)? info->channels*4 :
                                                info->channels*2);
info->lpcm_payload=X[0];
info->firstpackdecrement=X[1];
info->SCRquantity=X[2];
info->firstpack_audiopesheaderquantity=X[3];
info->midpack_audiopesheaderquantity=X[4];
info->lastpack_audiopesheaderquantity=X[5];
info->firstpack_lpcm_headerquantity=X[6];
info->midpack_lpcm_headerquantity=X[7];
info->lastpack_lpcm_headerquantity=X[8];
info->firstpack_pes_padding=X[9];
info->midpack_pes_padding=X[10];
*/

    if (cc == 0x1f)
    {
        cc=0;
    }
    else
    {
        cc++;
    }

    return(audio_bytes);
}


inline static int read_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf)
{
    int position;
    int title = -1;
    //static int cc;  // Continuity counter - reset to 0 when pack_in_title=0
    char* POS;
    uint32_t PTS;
    uint64_t SCR, pack_in_title = 0;
    int audio_bytes;

    read_pack_header(fp, &SCR); // +14

    uint8_t buf[4];

    uint64_t offset = ftello(fp);

    fread(buf, 4, 1, fp);

    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB)
    {
        position = FIRST_PACK;
        pack_in_title = 0;
        POS = "first";
        ++title;
    }
    else
    {
        if (fseek(fp, 2044, SEEK_CUR) != 0)
        {
            position = LAST_PACK;
            POS = "last";
        }
        else
        {
            fread(buf, 4, 1, fp);

            if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB)
            {
                position = LAST_PACK;
                POS = "last";
            }
            else
            {
                     position = MIDDLE_PACK;
                     POS = "middle";
            }
        }

        ++pack_in_title;
     }

    fseek(fp, offset, SEEK_SET);

    foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Reading %s packet - pack=%"PRIu64"\n",
            POS,
            pack_in_title);

    if (position == FIRST_PACK) read_system_header(fp); // +18

    /* skipping read_audio_pes_header to identify info */
    offset = ftello(fp);

    fseek(fp, 14 + (position == FIRST_PACK ? 3 : 0), SEEK_CUR);

    uint header_length = read_lpcm_header(fp, info, pack_in_title, position); //  info->first/mid/last/pack_lpcm_headerquantity + 4

    uint64_t offset1 = ftello(fp);

    fseek(fp, offset, SEEK_SET);

    /* AFTER read_lpcm_header, which is used to identify info members */

    uint16_t PES_packet_len = 0;

    switch(position)
    {
      case FIRST_PACK :

        audio_bytes = info->lpcm_payload - info->firstpackdecrement;

        PES_packet_len = read_audio_pes_header(fp, 1, &PTS); //+17
        if (info->firstpack_audiopesheaderquantity != PES_packet_len)
        {
            if (globals.logdecode)
            {
                open_aob_log();
                fprintf(aob_log, "NA;firstpack_audiopesheaderquantity issue in read_audio_pes_header at pack %lu, title %d, position %d\n", pack_in_title, title, position);
                close_aob_log();
            }
        }
        break;

      case MIDDLE_PACK :

        audio_bytes=info->lpcm_payload;

        PES_packet_len = read_audio_pes_header(fp, 0, &PTS); //+14

        if (info->midpack_audiopesheaderquantity != PES_packet_len)
        {
            if (globals.logdecode)
            {
                open_aob_log();
                fprintf(aob_log, "NA;midpack_audiopesheaderquantity issue in read_audio_pes_header at pack %lu, title %d, position %d\n", pack_in_title, title, position);
                close_aob_log();
            }
        }

        break;

      case LAST_PACK :

        PES_packet_len = read_audio_pes_header(fp, 0, &PTS); //+14

        audio_bytes = PES_packet_len - info->lastpack_audiopesheaderquantity;

        break;
    }


    /* 14 + (3) + 4 + header_length = 18 + (3) + header_length = offset1-offset */

    if (18 + (position == FIRST_PACK ? 3 : 0) + header_length != offset1 - offset)
    {
        if (globals.logdecode)
        {
            open_aob_log();
            fprintf(aob_log, "NA;header_length issue in read_lpcm_header at pack %lu, position %d\n", pack_in_title, position);
            close_aob_log();
        }
    }

    uint32_t PTS_calc;

    if (PTS != (PTS_calc = calc_PTS(info, pack_in_title)))
    {
        if (globals.logdecode)
        {
            open_aob_log();
            fprintf(aob_log, "NA;PTS issue at pack %lu, position %d. Disc/Computed:;%d;%d\n",
                    pack_in_title,
                    position,
                    PTS,
                    PTS_calc);

            close_aob_log();
        }
    }

    uint64_t SCR_calc;
    if (SCR != (SCR_calc = calc_SCR(info, pack_in_title)))
    {
        if (globals.logdecode)
        {
            open_aob_log();
            fprintf(aob_log, "NA;SCR issue at pack %lu, position %d. Disc/Computed:;%lu;%lu\n",
                                           pack_in_title,
                                           position,
                                           SCR,
                                           SCR_calc);
            close_aob_log();
        }
    }

    fseek(fp, offset1, SEEK_SET);

    /* offset_count+= audio_bytes */ fread(audio_buf, 1, audio_bytes,fp);

    switch(position)
    {
      case FIRST_PACK :
        if (info->firstpack_pes_padding > 0)
        {
           read_pes_padding(fp,info->firstpack_pes_padding); // 0 or 6 + info->firstpack_pes_padding
        }
        break;

      case MIDDLE_PACK :
        if (info->midpack_pes_padding > 0)
        {
           read_pes_padding(fp,info->midpack_pes_padding); // 0 or 6 + info->midpack_pes_padding
        }

        break;

      case LAST_PACK :
         read_pes_padding(fp,2006-audio_bytes);  // 2012 - audio_bytes
        break;
    }

    /*
    if (cc == 0x1f)
    {
        cc=0;
    }
    else
    {
        cc++;
    }
    ??
    */


    return(audio_bytes);
}

int decode_ats(char* aob_file)
{

    FILE* fp;

    int pack=0, pack_in_file=0, fileno=1;
    uint32_t bytesinbuf=2048, n=0;
    uint8_t audio_buf[2048];
    uint64_t pack_in_title=0;
    fileinfo_t files;

    fp=fopen(aob_file,"rb");
    if (globals.logdecode)
        unlink("/home/fab/aob_log");

    if (fp == NULL)
    {
        foutput("%s\n", "[ERR]   Could not open AOB file.");
    }

    do
    {
        n=read_pes_packet(fp, &files, audio_buf);

        if (n == bytesinbuf)
        {
            ++pack;
            ++pack_in_title;
            ++pack_in_file;
        }
        else
            bytesinbuf = 0;


    } while (bytesinbuf);

    return(1-fileno);
}


int create_ats(char* audiotsdir,int titleset,fileinfo_t* files, int ntracks)
{

    FILE* fpout;
    char outfile[CHAR_BUFSIZ+13+1];
    int i=0, pack=0, pack_in_file=0, fileno=1;
    uint32_t bytesinbuf=0, n=0, lpcm_payload=0;
    uint8_t audio_buf[AUDIO_BUFFER_SIZE];
    uint64_t pack_in_title=0;


    STRING_WRITE_CHAR_BUFSIZ(outfile, "%s/ATS_%02d_%d.AOB",audiotsdir,titleset,fileno)
            fpout=fopen(outfile,"wb+");



    //    /* Open the first file and initialise the input audio buffer */
    //    if (files[i].mergeflag)
    //    {

    //    }
    //    else
    if (audio_open(&files[i])!=0)
    {
        foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not open %s\n", files[i].filename);
        EXIT_ON_RUNTIME_ERROR
    }

    n=audio_read(&files[i], audio_buf, sizeof(audio_buf));
    bytesinbuf=n;

    lpcm_payload = files[i].lpcm_payload;

    files[i].first_sector=0;
    files[i].first_PTS=calc_PTS_start(&files[i],pack_in_title);

    foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Processing %s\n",files[i].filename);

    while (bytesinbuf)
    {
        if (bytesinbuf >= lpcm_payload)
        {
            //pack_in_file is not used in write_pes_packet

            n=write_pes_packet(fpout,&files[i],audio_buf,bytesinbuf,pack_in_title,(pack_in_file == 0));

            memmove(audio_buf,&audio_buf[n],bytesinbuf-n);
            bytesinbuf -= n;

            pack++;
            pack_in_title++;
            pack_in_file++;
        }

        if ((pack > 0) && ((pack%(512*1024))==0))
        {
            fclose(fpout);
            fileno++;
            STRING_WRITE_CHAR_BUFSIZ(outfile, "%s/ATS_%02d_%d.AOB",audiotsdir,titleset,fileno)
                    fpout=fopen(outfile,"wb+");
        }

        if (bytesinbuf < lpcm_payload)
        {
            n=audio_read(&files[i],&audio_buf[bytesinbuf],sizeof(audio_buf)-bytesinbuf);
            bytesinbuf+=n;
            if (n==0)   /* We have reached the end of the input file */
            {
                files[i].last_sector=pack;
                audio_close(&files[i]);
                i++;
                pack_in_file=-1;

                if (i<ntracks)
                {
                    /* If the current track is a different audio format, we must start a new title. */
                    if (files[i].newtitle)
                    {

                        n=write_pes_packet(fpout,&files[i-1],audio_buf,bytesinbuf,pack_in_title,(pack_in_file == 0)); // Empty audio buffer.
                        pack++;
                        bytesinbuf=0;
                        pack_in_title=0;
                        pack_in_file=0;

                        files[i].first_PTS=calc_PTS_start(&files[i],pack_in_title);
                    }
                    else
                    {
                        files[i].first_PTS=calc_PTS_start(&files[i],pack_in_title+1);
                    }

                    files[i].first_sector=files[i-1].last_sector+1;
                    if (audio_open(&files[i])!=0)
                    {
                        foutput(ANSI_COLOR_RED"\n[ERR]"ANSI_COLOR_RESET"  Could not open %s\n",files[i].filename);
                        EXIT_ON_RUNTIME_ERROR
                    }

                    n=audio_read(&files[i],&audio_buf[bytesinbuf],sizeof(audio_buf)-bytesinbuf);
                    bytesinbuf+=n;
                    foutput(ANSI_COLOR_BLUE"[INF]"ANSI_COLOR_RESET"  Processing %s\n",files[i].filename);
                }
                else
                {
                    /* We have reached the last packet of the last file */
                    if (bytesinbuf==0)
                    {
                        files[i-1].last_sector=pack-1;
                    }
                    else
                    {
                        n=write_pes_packet(fpout,&files[i-1],audio_buf,bytesinbuf,pack_in_title,(pack_in_file == 0)); // Empty audio buffer.
                        bytesinbuf=0;
                        pack++;
                        pack_in_title++;
                    }
                }
            }
        }
    }

    if (files[0].single_track) files[0].last_sector=files[ntracks-1].last_sector;
    return(1-fileno);
}
