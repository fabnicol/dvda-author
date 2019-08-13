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
#include <stdint.h>
#include <math.h>
#include <errno.h>

#include "audio2.h"
#include "c_utils.h"
#include "auxiliary.h"
#include "commonvars.h"
#include "ats.h"

#include "libavcodec/mlplayout.h"
#include "decode.h"
#include "c_utils.h"
#include "structures.h"

extern globalData globals;
extern uint8_t channels[21];
static uint8_t default_cga[]={0,  1,  7,  3,   16,   17};

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

    if (globals.logdecode)
        fprintf(aob_log, "INF;SCR_base: ;%" PRIu64 ";SCR_ext: ;%" PRIu64 ";SCR_int: ;%" PRIu64 ";\n", SCR_base, SCR_ext, *SCRint_ptr);
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

inline static void decode_pts(uint8_t P[5], uint64_t* pts_ptr)
{

    *pts_ptr = (P[0] >> 1 & 0x3) << 30 | P[1] << 22 | (P[2] >> 1) << 15 | P[3] << 7 | P[4] >> 1;

    if (globals.logdecode)
        fprintf(aob_log, "INF;PTS décode;%08lX ;;;;\n", *pts_ptr);
}

uint8_t pack_start_code[4]={0x00,0x00,0x01,0xBA};
uint8_t system_header_start_code[4]={0x00,0x00,0x01,0xBB};
uint8_t program_mux_rate_bytes[3]={0x01,0x89,0xc3};
uint8_t pack_stuffing_length_byte[1]={0xf8};

inline static void write_pack_header(FILE* fp,  uint64_t SCRint)
{

    uint8_t scr_bytes[6];

    /* offset_count += 4 */ fwrite(pack_start_code,4,1,fp);           // PCM or MLP
    pack_scr(scr_bytes,(SCRint/300),(SCRint%300));
    /* offset_count += 6 */ fwrite(scr_bytes,6,1,fp);
    /* offset_count += 3 */ fwrite(program_mux_rate_bytes,3,1,fp);    // PCM or MLP
    /* offset_count += */   fwrite(pack_stuffing_length_byte,1,1,fp); // PCM or MLP
}


inline static void read_pack_header(FILE* fp,  uint64_t* SCRint_ptr)
{
    uint8_t scr_bytes[6]={0};
    open_aob_log();
    CHECK_FIELD(pack_start_code)
    RW_FIELD(scr_bytes)
    decode_SCR(scr_bytes, SCRint_ptr);
    CHECK_FIELD(program_mux_rate_bytes)
    CHECK_FIELD(pack_stuffing_length_byte)
    close_aob_log();
}

uint8_t header_length[2]={0x00,0x0c};
uint8_t rate_bound[3]={0x80,0xc4,0xe1};
uint8_t audio_bound[1]={0x04};
uint8_t video_bound[1]={0xa0};
uint8_t packet_rate_restriction_flag[1]={0x7f};
uint8_t stream_info1[3]={0xb8, 0xc0, 0x40};
uint8_t stream_info2[3]={0xbd, 0xe0, 0x0a};

inline static void write_system_header(FILE* fp)
{

    /* offset_count += 4 */ fwrite(system_header_start_code,4,1,fp);  // PCM or MLP
    /* offset_count += 2 */ fwrite(header_length,2,1,fp);             // PCM or MLP
    /* offset_count += 3 */ fwrite(rate_bound,3,1,fp);                // PCM or MLP
    /* offset_count += */   fwrite(audio_bound,1,1,fp);               // PCM or MLP
    /* offset_count += */   fwrite(video_bound,1,1,fp);               // PCM or MLP
    /* offset_count += */   fwrite(packet_rate_restriction_flag,1,1,fp); // PCM or MLP
    /* offset_count += 3 */ fwrite(stream_info1,3,1,fp);              // PCM or MLP
    /* offset_count += 3 */ fwrite(stream_info2,3,1,fp);              // PCM or MLP
}

inline static void read_system_header(FILE* fp)
{
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

uint8_t packet_start_code_prefix[3]={0x00,0x00,0x01};
uint8_t stream_id_padding[1]={0xbe};
uint8_t length_bytes[2];

inline static void write_pes_padding(FILE* fp, uint16_t length)
{
    if (length == 0)
    {
      uint64_t offset = ftello(fp) % 2048;
      if (offset)
      {
        int8_t zero[2048-offset];
        memset(zero, 0, 2048-offset);
        fwrite(zero, 2048-offset, 1, fp);
      }
      return;
    }

    if (length > 6)
    {
      length-=6; // We have 6 bytes of PES header.
      if (globals.maxverbose)
          foutput("%s %d %s\n", INF " Padding with ", length, " bytes.");
    }
    else
    {
        foutput("%s\n", ERR "pes_padding length must be higher than 6;");
        return;
    }


    length_bytes[0] = (length & 0xFF00) >> 8;

    /* Take number of bytes to pad in sector (=2048-length)
     * */

    length_bytes[1] = length & 0xFF;

    uint8_t ff_buf[length];

    memset(ff_buf, 0xff, length);

    /* offset_count += 3 */ fwrite(packet_start_code_prefix, 3, 1, fp);
    /* offset_count += 1 */ fwrite(stream_id_padding, 1, 1, fp);
    /* offset_count += 2 */ fwrite(length_bytes, 2, 1, fp);
    /* offset_count += length */ fwrite(ff_buf, length, 1, fp);
    
}

inline static void read_pes_padding(FILE* fp, uint16_t length)
{
    if (length == 0)
    {
        uint64_t offset = ftello(fp) % 2048;
        if (offset)
        {
          uint8_t zero[2048 - offset];
          fread(zero, 2048 - offset, 1, fp);
        }
        return;
    }

    if (length > 6)
    {
        length-=6; // We have 6 bytes of PES header.
    }
    else
    {
        foutput("%s\n", ERR "  pes_padding length must be higher than 6;");
        return;
    }

    uint8_t ff_buf[length];

    memset(ff_buf, 0xff,length);

    open_aob_log();

    /* offset_count += 3 */ CHECK_FIELD(packet_start_code_prefix)
    /* offset_count += 1 */ CHECK_FIELD(stream_id_padding);

    length_bytes[0] = (length & 0xFF00) >> 8;
    length_bytes[1] = length & 0xFF;

    /* offset_count += 2 */ CHECK_FIELD(length_bytes);

    /* offset_count += length */ CHECK_FIELD_NOWRITE(ff_buf)

    close_aob_log();
}

uint8_t stream_id[1]={0xbd};
uint8_t PES_packet_len_bytes[2];
uint8_t flags1[1]={0x81};  // various flags - original_or_copy=1
uint8_t flags2[1]={0};  // various flags - contains pts_dts_flags and extension_flag
uint8_t PES_header_data_length[1]={0};
uint8_t PES_extension_flags[1]={0x1e};  // PSTD_buffer_flag=1
uint8_t PTS_data[5];
uint8_t DTS_data[5];
uint8_t PSTD_buffer_scalesize[2];
const uint16_t PSTD = 10;

inline static void write_audio_pes_header(FILE* fp, uint16_t PES_packet_len, uint8_t mlp_flag, uint8_t extension_flag, uint32_t PTS, uint32_t DTS)
{

    PES_header_data_length[0] = mlp_flag  == 0xC0 ? 0xA : 5;       // PTS only (and PSTD)

    if (extension_flag)
    {
        PES_header_data_length[0] += 3; // 8 (uncompressed) or 0xD (mlp)
    }

    flags2[0] = mlp_flag | extension_flag;  // PES_extension_flag = 1 marks pack_in_title = 0

    PES_packet_len_bytes[0] = (PES_packet_len & 0xff00) >> 8;
    PES_packet_len_bytes[1] = PES_packet_len & 0xff;

    /* offset_count += 3 */ fwrite(packet_start_code_prefix, 3, 1, fp);
    /* offset_count += */   fwrite(stream_id, 1, 1, fp);

    if (globals.maxverbose) fprintf(stderr, DBG "Writing PES_plb at offset: %" PRIu64 ", with value PES_packet_len = %d (%d, %d)\n",
                                    ftello(fp),
                                    PES_packet_len,
                                    PES_packet_len_bytes[0],
                                    PES_packet_len_bytes[1]);

    /* offset_count += 2 */ fwrite(PES_packet_len_bytes, 2, 1, fp);  //PCM and also works for MLP (!)
    /* offset_count += */   fwrite(flags1, 1, 1, fp); // PCM or MLP

    // yields 0xC10D (first pack)  then 0xC00A (following packs) for MLP
    // yields 0x8108 (first pack)  then 0x8005 (following packs) for MLP

    /* offset_count += */   fwrite(flags2, 1, 1, fp); // MLP change  as above
    /* offset_count += */   fwrite(PES_header_data_length, 1, 1, fp);

    pack_pts(PTS_data, PTS);

    /* PATCH LOCATION 1: +0x17=23 */

    /* offset_count += 5 */ fwrite(PTS_data, 5, 1, fp);

    if (DTS)  // will select MLP case as DTS is null for PCM
    {
         pack_pts(DTS_data, DTS);
        /* offset_count += 5 */ fwrite(DTS_data, 5, 1, fp);
    }

    if (extension_flag)  // only there at start of first pack. Looks correct for both MLP and PCM
    {
        PSTD_buffer_scalesize[0] = 0x60 | ((PSTD & 0x1f00) >>8);
        PSTD_buffer_scalesize[1] = PSTD & 0xff;

        /* offset_count += */   fwrite(PES_extension_flags,1,1,fp);   //0x1E
        /* offset_count += 2 */ fwrite(PSTD_buffer_scalesize,2,1,fp); // 0x600A
    }
}

inline static uint16_t read_audio_pes_header(FILE* fp, uint8_t mlp_flag, uint8_t extension_flag, uint32_t* PTS_ptr, uint32_t* DTS_ptr)
{
    uint16_t PES_packet_len = 0;

    PES_header_data_length[0] = mlp_flag  == 0xC0 ? 0xA : 5;

    if (extension_flag)
    {
        PES_header_data_length[0] += 3;
    }

    flags2[0] = mlp_flag | extension_flag;

    open_aob_log();

    CHECK_FIELD(packet_start_code_prefix);
    CHECK_FIELD(stream_id);
    RW_FIELD(PES_packet_len_bytes);

    PES_packet_len = PES_packet_len_bytes[0] << 8 | PES_packet_len_bytes[1];

    CHECK_FIELD(flags1);
    CHECK_FIELD(flags2);
    CHECK_FIELD(PES_header_data_length);

    RW_FIELD(PTS_data);

    decode_pts(PTS_data, PTS_ptr);

    if (DTS_ptr)
    {
        RW_FIELD(DTS_data);

        decode_pts(DTS_data, DTS_ptr);
    }

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

uint8_t sub_stream_id[1]      = {0xa0};
uint8_t continuity_counter[1] = {0x00};
uint8_t LPCM_header_length[2];
uint8_t first_access_unit_pointer[2];
uint8_t downmix_index[1]      = {0x00};   // 0x10 for stereo or mono, 0x00 for surround or rank of downmix table
uint8_t sample_size[1]        = {0x0f};   // 0x0f=16-bit, 0x1f=20-bit, 0x2f=24-bit
uint8_t sample_rate[1]        = {0x00};   // 0x0f=48KHz, 0x1f=96KHz, 0x2f=192KHz,0x8f=44.1KHz, 0x9f=88.2KHz, 0xaf=176.4KHz
uint8_t unknown2[1]           = {0x00};
uint8_t channel_assignment[1] = {0};      // The channel assignment - 0=C; 1=L,R; 17=L,R,C,lfe,Ls,Rs
uint8_t unknown3[1]           = {0x80};
uint8_t zero[16]              = {0};

inline static void write_lpcm_header(FILE* fp, uint8_t header_length, fileinfo_t* info, int64_t pack_in_title, uint8_t counter, struct MLP_LAYOUT* m)
{
// Except for first MLP "AOB" headers are 43 bytes long
// Examples

//    00 00 01 BA 44 00 04 B8 75 83 01 89 C3 F8 00 00 01 BD 07 EC 81 C0 0A 31 00 01 2F 61 11 00 01 2E BF A1 01 00 06 00 17 07 FF 00 00
//...
//    00 00 01 BA 44 00 DD 56 85 83 01 89 C3 F8 00 00 01 BD 07 EC 81 C0 0A 31 00 37 56 E9 11 00 37 56 47 A1 07 00 06 00 33 FF 09 00 00
//                                                                               +5224           +5224
//    00 00 01 BA 44 00 DD A8 34 AB 01 89 C3 F8 00 00 01 BD 05 D0 81 C0 0A 31 00 37 6B 51 11 00 37 6A AF A1 08 00 06 00 1C FF 09 00 00
//    Offsets 25-30 (0 based) differ by 162
//    Offsets 25 and 30 increase by 5224 each sector
// Rest of sector is just MLP bytes without any permutation involved.

    int frame_offset;
    uint64_t bytes_written;
    uint64_t frames_written;

    continuity_counter[0]=counter;
    uint8_t high_nibble = 0;

    if (info->type != AFMT_MLP)
    {

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
            downmix_index[0]=0x10;

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
            // rank of downmix table if there is one: 0 for 1st table, 1 for 2nd etc. up to 15 for the 16th table (0x0F)

            downmix_index[0]= info->downmix_table_rank;

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
    }
    else sample_size[0] = 0xFF;

    // TODO: assign real channel !!
       channel_assignment[0] = info->cga;  // defaults to default_cga[info->channels -1];
    // TODO

    bytes_written = pack_in_title == 0 ? 0 : (pack_in_title * info->lpcm_payload) - info->firstpackdecrement;

    // e.g., for 4ch, First pack is 1984, rest are 2000
    // for MLP header_lengh looks like constant 6 and is adjusted in audio.c

    if (info->type == AFMT_MLP)
    {
        frame_offset = m[pack_in_title].pkt_pos - (pack_in_title ? ((pack_in_title - 1) * 2005 + 1984) : 0) + header_length - 1;
    }
    else
    {
        frames_written = bytes_written / info->bytesperframe;

        if (frames_written * info->bytesperframe < bytes_written)
        {
            frames_written++;
        }

        frame_offset = (frames_written * info->bytesperframe) - bytes_written + header_length - 1;
    }

    LPCM_header_length[0] = (header_length & 0xff00) >> 8;
    LPCM_header_length[1] = header_length & 0xff;

    first_access_unit_pointer[0]=(uint8_t)((frame_offset & 0xff00) >> 8);
    first_access_unit_pointer[1]=(uint8_t) frame_offset & 0xff;

    if (info->type == AFMT_MLP) sub_stream_id[0] = 0xa1;

    /* offset_count += */   fwrite(sub_stream_id, 1, 1, fp);  // MLP change: PCM 0xa0 -> 0xa1 (offset: 0x31 / 33)
    /* offset_count += */   fwrite(continuity_counter, 1, 1, fp);  // PCM MLP
    /* offset_count += 2 */ fwrite(LPCM_header_length, 2, 1, fp);  // MLP looks constant for all packs at 0x06, tested for 2/16-24/44 and 3/16/96(offset 0x39 at  first pack and 0x24 at following packs).
    /* offset_count += 2 */ fwrite(first_access_unit_pointer, 2, 1, fp); // PCM: uses PCM frames as defined in audio.c
                                                                         // MLP: very different, uses pkt_pos of first packet after cumulative sector payload,
                                                                         // exactly as in calc_PCM_DTS_MLP() with m[sector].pkt_pos from get_mlp_layout


    /* offset_count += */   fwrite(downmix_index, 1, 1,fp); // PCM or MLP
    /* offset_count += */   fwrite(sample_size,1,1,fp); // Apparently 0xFF for all MLP ? TOO: check this.
    /* offset_count += */   fwrite(sample_rate,1,1,fp); // PCM, no meaning for MLP (0)
    /* offset_count += */   fwrite(unknown2,1,1,fp);

    // MLP pack headers stp here
    if (info->type == AFMT_MLP) return;

    /* offset_count += */   fwrite(channel_assignment,1,1,fp);
    /* offset_count += */   fwrite(unknown3,1,1,fp);
    /* offset_count += (header_length-8) */
    fwrite(zero,header_length-8,1,fp);

}

inline static int read_lpcm_header(FILE* fp, fileinfo_t* info, uint32_t pack_in_title, uint8_t position)
{

    int frame_offset = 0;
    uint64_t bytes_written = 0;
    uint64_t frames_written = 0;

    open_aob_log();

    /* offset_count += */   CHECK_FIELD(sub_stream_id);
    /* offset_count += */   RW_FIELD(continuity_counter);
    /* offset_count += 2 */ RW_FIELD(LPCM_header_length);

    uint8_t header_length =  /* LPCM_header_length[0] << 8 | */ LPCM_header_length[1];

    /* offset_count += 2 */ RW_FIELD(first_access_unit_pointer);

    /* offset_count += */   RW_FIELD(downmix_index);
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

        if ((sample_rate[0] & 0xf) != 0xf)
        {
            if (globals.logdecode)
                fprintf(aob_log, "%s", "ERR;coherence_test;sample_rate and sample_size are incoherent (no 0xf lower nibble);;;;;\n");
        }

    }
    else
    if (sample_size[0] == 0x00 || sample_size[0] == 0x22)
    {
        if ((sample_rate[0] & 0xf) != high_nibble)
        {
            if (globals.logdecode) fprintf(aob_log, "%s", "ERR;coherence_test;sample_rate and sample_size are incoherent (lower nibble != higher nibble);;;;;\n");
        }
    }

    if (sample_size[0] == 0x2f || sample_size[0] == 0x22)
        info->bitspersample = 24;
    else
        info->bitspersample = 16;

    /* offset_count += */   CHECK_FIELD(unknown2);
    /* offset_count += */   RW_FIELD(channel_assignment);

    info->cga = channel_assignment[0];

    info->channels = (channel_assignment[0] < 21) ? channels[channel_assignment[0]] : 0;

    if (globals.logdecode)
    {
        if (info->channels <= 2)
        {
            if (downmix_index[0] != 0x10)
               fprintf(aob_log, "%s%d%s", "ERR;incorrect unknown1 lpcm header value: ;", downmix_index[0], "; instead of expected 0x10;;;;\n");
        }
    else

        if (downmix_index[0] > 0xF)
               fprintf(aob_log, "%s%d%s", "ERR;incorrect downmix index lpcm header value: ;", downmix_index[0], "; instead of expected 0-0xF;;;;\n");

    }

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

            if (header_length != info->firstpack_lpcm_headerquantity)
            {
                if (globals.logdecode) fprintf(aob_log, "WAR;info->firstpack_lpcm_headerquantity;Disk: ; %d; Computed: ; %d;;\n", header_length, info->firstpack_lpcm_headerquantity);
            }
            break;

        case  LAST_PACK:
            if (header_length != info->lastpack_lpcm_headerquantity)
            {
                if (globals.logdecode) fprintf(aob_log, "WAR;info->lastpack_lpcm_headerquantity;Disk: ;%d; Computed: ; %d;;\n", header_length, info->lastpack_lpcm_headerquantity);
            }
            break;

        case  MIDDLE_PACK:
            if (header_length != info->midpack_lpcm_headerquantity)
            {
                if (globals.logdecode) fprintf(aob_log, "WAR;info->midpack_lpcm_headerquantity;Disk: ; %d; Computed: ; %d;;\n", header_length, info->midpack_lpcm_headerquantity);
            }
    }

    bytes_written = pack_in_title == 0 ? 0 : (pack_in_title*info->lpcm_payload)-info->firstpackdecrement;
    // e.g., for 4ch, First pack is 1984, rest are 2000
    frames_written = bytes_written/info->bytesperframe;
    if (frames_written*info->bytesperframe < bytes_written)
    {
        frames_written++;
    }

    frame_offset=(frames_written*info->bytesperframe)-bytes_written+header_length-1;

    if (globals.veryverbose)
        foutput("titlepack: %ld bytes written: %ld\nsample rate: %d bit rate: %d channel asignment: %d\n",
                pack_in_title, bytes_written, info->samplerate, info->bitspersample, info->cga);

    if (first_access_unit_pointer[0] != (frame_offset & 0xff00) >> 8)
    {
        foutput("%s\n", "first_access_unit_pointer: inaccurate bit 0");
        if (globals.logdecode) fprintf(aob_log, "ERR;first_access_unit_pointer byte 0 inaccurate; %d ; instead of;  %d;;;\n", first_access_unit_pointer[0], (frame_offset & 0xff00) >> 8);
    }

    if (first_access_unit_pointer[1] != (frame_offset & 0xff))
    {
        foutput("%s\n", "first_access_unit_pointer: inaccurate bit 1");
        if (globals.logdecode) fprintf(aob_log, "ERR;first_access_unit_pointer byte 1 inaccurate; %d ; instead of; %d;;;\n", first_access_unit_pointer[1], frame_offset & 0xff);
    }

    /* offset_count += */   CHECK_FIELD(unknown3);

    if (header_length < 8) return 0;
    uint8_t zero[header_length-8];
    uint8_t zero__[header_length-8];
    memset(zero, 0, header_length-8);
    memset(zero__, 0, header_length-8);

    /* offset_count += (header_length-8) */
    CHECK_FIELD_NOWRITE(zero);

    close_aob_log();
    return header_length;
}


inline static uint64_t calc_SCR(fileinfo_t* info, uint64_t pack_in_title)
{
  static double SCR;

  static double SCR1;

  // SCR should be cumulated as floating point
  // It is approriate to filter out rounding errors as they may pile up

  switch (pack_in_title)
  {
    case 0: return 0;
    case 1:
          SCR = (info->lpcm_payload - info->firstpackdecrement) * 300.0 * 90000.0 / info->bytespersecond;
          break;
    case 2:
          SCR1 = info->lpcm_payload * 300.0 * 90000.0 / info->bytespersecond;  // do not compute each time ! Once is enough
          SCR += SCR1;
          break;

    default:
         SCR += SCR1;
         break;
  }

  double SCR_sup = ceil(SCR);

  if (SCR_sup - SCR < 1E-6)
  {
     return (uint64_t) SCR_sup;
  }

  return (uint64_t) floor(SCR);

  // For MLP seems to be : in sector count as many raw (not counting AOB headers) MLP bytes, see corresponding PCM bytes output (using get_mlp_layout)
  // then use PCM bytespersecond so that duration = PCM byte output (sector) / bytespersecond * 90000 * 300
  // SCR starts after 000001BA, normally with 0x44 at lower offsets, on 6 bytes.
}


inline static void calc_PTS_DTS_MLP(fileinfo_t* info, const struct MLP_LAYOUT* m, pts_t* res)
{
    if (info->type != AFMT_MLP)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "MLP requested")
    }

    // now m[i].pkt_pos indicates offset of MLP packet in mlp file that is just after sector m[i].rank in "mlp-type" AOB
    // m[i].nb_samples gives number of corresponding PCM samples after decoding.
    // PTS must be computed using this.

    // Algorithm : at each sector of rank (1-based) N of 2048 B, compute :
    // - sum of header size S before N in B
    // - total byte size = 2048  * (N-1) - S   (0xE is where header starts for PTS)
    // - run parallel count of PKT pos through MLP decoder, gives Rank of packet and POS of packet
    // - take Rank such that POS (0-based) of MLP packet strictly superior to total byte size
    // - Rank * 40 = NPCMsamples		(Rank 0-based, numer of frames of 40 samples)
    // - NPCMsamples * c * br / 8 = Nbyteswritten
    // - Nbyteswritten / (c * sr * br / 8 ) = duration
    // Or : duration = NPCMsamples /sr
    // PTS = ceiling(duration * 90000)		   = ceiling(Rank * 40  / sr * 90000) = ceiling(Rank / sr * 360000)
    // DTS = floor(duration * 90000)
    // Add :
    // PTS = PTS + PTS0
    // DTS = DTS + DTS0
    // PTS0 = floor of PTS tick duration of playing samples in 1 frame = floor(samplesperframe/samplerate x 90000) + 24
    // DTS0 = 24 or 23 (?)
    // Write PTS at offset 23 on 5 bytes in sector header
    // Write DTS at offset 28 on 5 bytes in sector header

    // Variant below that looks slightly closer avoiding rounding errors
    // START_MLP_BYTESHIFT is a constant that may be software-specific (DW is 23 and Sonic is 584). Choosing DW value by default.

    const uint32_t start_byteshift = START_MLP_BYTESHIFT;

    for (int i = 0; i < MAX_AOB_SECTORS && m[i].pkt_pos != 0; ++i)
    {
      double ptsint = ceil((double) (m[i].nb_samples  + info->samplesperframe)/ (double) info->samplerate * 90000.0);
      double dtsint = ceil((double) m[i].nb_samples/ (double) info->samplerate * 90000.0);
      res[i].DTSint = (uint32_t)  dtsint;
      res[i].PTSint = (uint32_t) ptsint;
      res[i].DTSint += start_byteshift;
      res[i].PTSint += start_byteshift;
    }

    ++res[0].PTSint;

    // now for sector 0-ranked i, use PST[i] and DTS[i] in write_pes_header at offset 23/28
    // there may be 1 to 3 remaining useless indexes used for complete audio data description in m[]
    // There seems to be 1 PTS tick less for about 1/3rd of PTS ticks
    // and 1 DTS tick more very occasionnaly (~2%)
    // This looks negligible.
    // Note: these spreads are 1/90000 th of a second ~ 0.5 (1ch 44.1 kHz) to 40 bytes (6 ch. 192 KHz) of pcm at worst

}


// SCR for MLP is poorly understood and very software-specific.
// Sonic has a different start slope for the first four sectors then the steep one thereafter at 90000 * 300 / samplerate
// wrt PCM (nb_samples) - PCM (nb_samples) samples of first 4 sectors
// But this is approximative so the nb_samples used may be quite different.
//
// DW has an closer system to ffmpeg nb_samples yet only SCR_int can be approximately guessed with more or less confidence.
// I do not know if these differences arise from flexibility of specifications, hardware concerns or sheer code / software obfuscation.
// one would prefer a simpler way of computing SCR as for calc_SCR above (PCM case):
//
// inline static uint64_t calc_SCR_MLP_v2(fileinfo_t* info, const struct MLP_LAYOUT* m, uint64_t* res)
// {
//   if (info->type != AFMT_MLP)
//   {
//      EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "MLP requested")
//   }
//
//   for (int i = 1; i < MAX_AOB_SECTORS && m[i].pkt_pos != 0; ++i)
//   {
//    double temp = (double) (m[i].nb_samples - 40.0) / (double) info->samplerate * 90000.0 * 300.0
//
//    double res = ceil(temp);
//
//    if (res - temp < 1E-6)
//    {
//       res[i] =  temp;
//    }
//    else
//    {
//       res[i] = floor(temp);
//    }
// }
//
// To be tested



inline static void calc_SCR_MLP(fileinfo_t* info, const struct MLP_LAYOUT* m, uint64_t* res)
{
    if (info->type != AFMT_MLP)
    {
        EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "MLP requested")
    }

   // struct MLP_LAYOUT already retrieved

    double sectorbytes = 1984;

    for (int i = 1; i < MAX_AOB_SECTORS && m[i].pkt_pos != 0; ++i)
    {
      double temp = (double) (m[i].nb_samples - 40.0) / (double) info->samplerate * 90000;  // PTS ticks first
      sectorbytes += 2005;
      double byteshift = m[i].pkt_pos - sectorbytes;  // very wild guess through regression
      temp += 26.0 - byteshift / 14.0;                     // that looks like close at a handful PTS ticks off
      res[i] = ((uint64_t) round(temp)) * 300 ;  // giving up on SCR_ext ticks
    }

    return;
}

inline static uint32_t calc_PTS(fileinfo_t* info, uint64_t pack_in_title, uint32_t *totpayload)
{
    double PTS;
    uint32_t PTSint;

    if (info->bytespersecond == 0)
    {
        foutput(WAR "file %s has 0 bytes per second\n", info->filename);
        return 0;
    }

    if (pack_in_title == 0) return 0;

    if (pack_in_title == 1)
    {
       *totpayload = info->lpcm_payload - info->firstpackdecrement;
    }
    else
    {
       *totpayload += info->lpcm_payload;
    }

   PTS = *totpayload / info->bytesperframe;
   PTS = ceil(PTS) * info->bytesperframe;  // ceiling round
   PTS /= info->bytespersecond;
   PTS *= 90000.0; // 1 s = 90,000 PTS ticks

   PTSint = (uint32_t) round(PTS);  // normal round
   return(PTSint);
}


inline static int write_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf, uint32_t bytesinbuffer, uint64_t pack_in_title, bool start_of_file)
{
    int audio_bytes;
    static int cc;  // Continuity counter - reset to 0 when pack_in_title=0

    uint32_t PTS;
    uint32_t DTS = 0;
    uint64_t SCR = 0;
    struct MLP_LAYOUT *m = NULL;
    uint64_t res_scr[MAX_AOB_SECTORS] = {0};
    static uint32_t cumbytes;
    uint8_t  mlp_flag = 0;
    pts_t* res_mlp;

    if (info->type == AFMT_MLP)
    {
        mlp_flag = 0xC0;

        if (pack_in_title == 0)  // once and for all for file info->filename, assuming a file has just one title in it (normally the case)
        {
            res_mlp = calloc(MAX_AOB_SECTORS, sizeof (pts_t));
            if (res_mlp == NULL) EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Could not allocate res_mlp.")
            calc_PTS_DTS_MLP(info, m, &res_mlp[0]);
            calc_SCR_MLP(info, m, &res_scr[0]);
        }

        PTS = res_mlp[pack_in_title].PTSint;
        DTS = res_mlp[pack_in_title].DTSint;
        SCR = res_scr[pack_in_title];
    }
    else
    {
        mlp_flag = 0x80;
        if (pack_in_title == 0) cumbytes = 0;
        PTS = calc_PTS(info, pack_in_title, &cumbytes);
        SCR = calc_SCR(info, pack_in_title);
    }

    static bool wait_for_next_pack;

    // Two cases arise: if new file is new title, then pack_in_title == 0
    // and catch PTS into first_PS
    // Else if new file is same title (gapless case), then pack_in_title != 0
    // and wait for next loop to assign first_PTS. The "shifted" bytes from
    // new file to previous file's last pack are not taken into consideration
    // for first_PTS assignment.

    if (start_of_file)
    {
        if(pack_in_title == 0 || wait_for_next_pack)
        {
            info->first_PTS = PTS;
            wait_for_next_pack = false;
        }
        else
            wait_for_next_pack = true;
    }

    if (pack_in_title == 0)
    {
        cc=0;            // First packet in title
        foutput(INF "Writing first packet - pack=%"PRIu64", bytesinbuffer=%d\n",pack_in_title,bytesinbuffer);

        write_pack_header(fp,SCR); //+14

        write_system_header(fp); //+18

        write_audio_pes_header(fp, info->firstpack_audiopesheaderquantity, mlp_flag, 1, PTS, DTS); //+17

        audio_bytes = info->lpcm_payload - info->firstpackdecrement;

        write_lpcm_header(fp, info->firstpack_lpcm_headerquantity, info, pack_in_title, cc, m);
        //info->firstpack_lpcm_headerquantity+4 and MLP always 64

        /* offset_count+= */

        uint64_t offset = ftello(fp);
 
        int res = fwrite(audio_buf, 1, audio_bytes, fp);
        if (globals.maxverbose) fprintf(stderr, DBG "\n%" PRIu64 ": Writing %d bytes\n", offset, res);

        // Apparenlty no padding for MLP files

        if (info->type != AFMT_MLP)
            write_pes_padding(fp, info->firstpack_pes_padding);//+6+info->firstpack_pes_padding
    }
    else if (bytesinbuffer < info->lpcm_payload)
    {

        foutput(INF "Writing last packet - pack=%" PRIu64 ", bytesinbuffer=%d\n", pack_in_title, bytesinbuffer);
        audio_bytes=bytesinbuffer;

        write_pack_header(fp,SCR); //+14

        if (globals.maxverbose)
            fprintf(stderr, DBG "LAST PACK: audio_bytes: %d, info->lastpack_audiopesheaderquantity %d \n",
                    audio_bytes, info->lastpack_audiopesheaderquantity);

        write_audio_pes_header(fp, info->lastpack_audiopesheaderquantity + audio_bytes, mlp_flag, 0, PTS, DTS);  // +14 for PCM or +19 for MLP

        write_lpcm_header(fp, info->lastpack_lpcm_headerquantity, info, pack_in_title,cc, m);

        // +info->lastpack_lpcm_headerquantity +4 (PCM) or +10 (MLP), headers always 43 B
        /* offset_count+= */

        uint64_t offset = ftello(fp);
   
        int res = fwrite(audio_buf, 1, audio_bytes, fp);
        if (globals.maxverbose) fprintf(stderr, DBG "%" PRIu64 ": Writing %d bytes\n", offset, res);

        if (info->type != AFMT_MLP)
        {
            // PATCH Dec. 2013
            // PATCH Sept 2016

            int16_t padding_quantity = 2016 - info->lastpack_lpcm_headerquantity - audio_bytes;
            if (padding_quantity < 0)
            {
                foutput(ERR "Padding quantity error (last title packet): lastpack_lpcm_headerquantity %d; audio_bytes %d\n", info->lastpack_lpcm_headerquantity, audio_bytes);
                return audio_bytes;
            }
            write_pes_padding(fp, (uint16_t) padding_quantity);
        }
        else
        {
          free(res_mlp);
        }

    }
    else
    {
        audio_bytes=info->lpcm_payload;

        write_pack_header(fp,SCR); //+14

        write_audio_pes_header(fp,info->midpack_audiopesheaderquantity, mlp_flag, 0,PTS, DTS);//+14

        write_lpcm_header(fp,info->midpack_lpcm_headerquantity,info,pack_in_title,cc, m);
        //info->midpack_lpcm_headerquantity+4 (PCM) or again 43 (MLP)

        //PATCH May. 2018: reset continuity counters at start of file

        if (start_of_file) cc=0x1f;

        // offset_count+=

        uint64_t offset = ftello(fp);
   
        int res = fwrite(audio_buf,1,audio_bytes,fp);
        if (globals.maxverbose) fprintf(stderr, DBG "%" PRIu64 ": Writing %d bytes\n", offset, res);


        if (info->type != AFMT_MLP)
           write_pes_padding(fp,info->midpack_pes_padding);//info->midpack_pes_padding +6
    }

    if (info->type == AFMT_MLP && pack_in_title == 0) free(m);

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

int read_pes_packet(FILE* fp, fileinfo_t* info, uint8_t* audio_buf)
{
    uint8_t position;
    //static int cc;  // Continuity counter - reset to 0 when pack_in_title=0
    char* POS;
    uint32_t PTS = 0;
    uint32_t DTS = 0;
    uint64_t SCR;
    uint8_t  mlp_flag = 0;
    static uint32_t cumbytes;
    static uint32_t pack_in_title;
    static int title;
    unsigned int audio_bytes = 0;

    if (fp == NULL) return 0;
    uint32_t offset0 = (uint32_t) ftello(fp);

    if (pack_in_title == 0)
    {
        cumbytes = 0;
        fseek(fp, 0x44, SEEK_SET);
        uint8_t buf[3];
        fread(buf, 3, 1, fp);
        if (buf[0] == 0xF8 && buf[1] == 0x72 && buf[2] == 0x6f)
        {
            if (globals.maxverbose)
            {
                foutput("%s %s.\n", MSG_TAG "Detected MLP format for", info->filename);
            }

            info->type = AFMT_MLP;
        }
    }

    if (info->type == AFMT_MLP)
    {
        mlp_flag = 0xC0;
    }

    fseek(fp, offset0 +14, SEEK_SET);

    uint8_t buf[4];
    fread(buf, 4, 1, fp);

    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB)
    {
        position = FIRST_PACK;
        pack_in_title = 0;
        cumbytes = 0;
        POS = "first";
        ++title;
    }
    else
    {
        int res = fseek(fp, 2044, SEEK_CUR);

        if (res != 0)
        {
            position = LAST_PACK;
            POS = "last";
        }
        else
        {
            int n = (int) fread(buf, 1, 4, fp);

            if (n != 4 || (n == 4 && buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xBB))
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

    if (globals.logdecode)
    {
        open_aob_log();
        fprintf(aob_log, ";;;;;;;\nNEW;Reading; %s ; packet - pack;%" PRIu32 ";title;%d;\n",
                POS,
                pack_in_title,
                title);
        close_aob_log();
    }

    fseek(fp, offset0, SEEK_SET);

    /***       +14     ***/
    read_pack_header(fp, &SCR);

    /***       +18  if first  ***/
    if (position == FIRST_PACK) read_system_header(fp);

    int header_length = 0;

    uint32_t offset1 = 0;

    if (info->type != AFMT_MLP)
    {
            /* skipping read_audio_pes_header to identify info */

            uint32_t offset = (uint32_t) ftello(fp);

            fseek(fp, 14 + (position == FIRST_PACK ? 3 : 0), SEEK_CUR);

            /***       info->first/mid/last/pack_lpcm_headerquantity + 4    ***/
            header_length = read_lpcm_header(fp, info, pack_in_title, position);

            offset1 =  (uint32_t) ftello(fp);

            fseek(fp, offset, SEEK_SET);

            /* 14 + (3) + 4 + header_length = 18 + (3) + header_length = offset1-offset */

            if ((uint32_t) 18 + (position == FIRST_PACK ? 3 : 0) + (uint32_t) header_length != offset1 - offset)
            {
                if (globals.logdecode)
                {
                    open_aob_log();
                    fprintf(aob_log, "ERR;header_length issue in read_lpcm_header at pack: ; %" PRIu32 "; position %d;;;;\n", pack_in_title, position);
                    fprintf(aob_log, "ERR;header_length: ; %d; expected: %" PRIu32 ";;;;\n", header_length, (uint32_t) offset1 - offset - 18 - (position == FIRST_PACK ? 3 : 0));
                    close_aob_log();
                }
            }
    }
    else
    {
        header_length = position == FIRST_PACK ? 64 : 43;
    }

    /* AFTER read_lpcm_header, which is used to identify info members */

    uint16_t PES_packet_len = 0;
    uint32_t pts_calc = 0;

    if (info->type != AFMT_MLP)
    {
        switch(position)
        {
            case FIRST_PACK :

                audio_bytes = info->lpcm_payload - info->firstpackdecrement;

                PES_packet_len = read_audio_pes_header(fp, mlp_flag, 1, &PTS, &DTS); //+17
                if (info->firstpack_audiopesheaderquantity != PES_packet_len)
                {
                    if (globals.logdecode)
                    {
                        open_aob_log();
                        fprintf(aob_log,
                                "ERR;firstpack_audiopesheaderquantity issue in read_audio_pes_header at pack: ; %" PRIu32 "; title: ;%d; position: ;%d;\n",
                                pack_in_title,
                                title,
                                position);
                        close_aob_log();
                    }
                }
                break;

            case MIDDLE_PACK :

                audio_bytes = info->lpcm_payload;

                /***   +14   ***/

                PES_packet_len = read_audio_pes_header(fp, mlp_flag, 0, &PTS, &DTS);

                if (info->midpack_audiopesheaderquantity != PES_packet_len)
                {
                    if (globals.logdecode)
                    {
                        open_aob_log();
                        fprintf(aob_log, "NA;midpack_audiopesheaderquantity issue in read_audio_pes_header at pack %" PRIu32 ";Disk/computed;%d;%d\n",
                                pack_in_title,
                                PES_packet_len,
                                info->midpack_audiopesheaderquantity);
                        close_aob_log();
                    }
                }

                break;

            case LAST_PACK :

                PES_packet_len = read_audio_pes_header(fp, mlp_flag, 0, &PTS, &DTS); //+14

                audio_bytes = PES_packet_len - info->lastpack_audiopesheaderquantity;

                break;
        }

        pts_calc = calc_PTS(info, pack_in_title, &cumbytes);
    }
    else
    {
        if (header_length > 2040)
        {
            EXIT_ON_RUNTIME_ERROR_VERBOSE(ERR "Could not compute reasonable header length (< 2040)")
        }

        audio_bytes = 2048 - (uint32_t) header_length;  // sureley false for last pack but well

        read_audio_pes_header(fp, mlp_flag, position == FIRST_PACK, &PTS, &DTS);  // extension for first pack only


        if (globals.logdecode)
        {
            open_aob_log();
            fprintf(aob_log, "%s\n",  "WAR;MLP case, PTS not calculated / supported in --log-decoded, just read from input.");
            fprintf(aob_log, "INF;bitspersample %d;samplerate %d\n",
                    info->bitspersample,
                    info->samplerate);
            close_aob_log();
        }
    }

    if (PTS != pts_calc)
    {
        if (globals.logdecode)
        {
            open_aob_log();
            if (DTS == 0)
              fprintf(aob_log, "ERR;PTS issue at pack: ; %" PRIu32 "; position: %d; Disc: ;%" PRIu32 "; Computed: ;%" PRIu32 "\n",
                      pack_in_title,
                      position,
                      PTS,
                      pts_calc);

            else
            {
                fprintf(aob_log, "ERR;PTS issue at pack: ; %" PRIu32 "; position: %d; PTS: ;%" PRIu32 "; DTS: ;%" PRIu32 "\n",
                        pack_in_title,
                        position,
                        PTS,
                        DTS);

            }
            close_aob_log();
        }
    }

    uint64_t SCR_calc;

    if (SCR != (SCR_calc = calc_SCR(info, pack_in_title)))
    {
        if (globals.logdecode)
        {
            open_aob_log();
            fprintf(aob_log, "ERR; SCR issue at pack: ; %" PRIu32 "; position: %d; Disc: ;%" PRIu64 "; Computed: ;%" PRIu64 "\n",
                    pack_in_title,
                    position,
                    SCR,
                    SCR_calc);
            close_aob_log();
        }
    }

    if (info->type != AFMT_MLP)
    {
       fseek(fp, offset1, SEEK_SET);
    }
    else
    {
        // just lazy to do more for now
       fseek(fp, offset0 + 2048, SEEK_SET);
       return(position);
    }

    /* offset_count+= audio_bytes */

    size_t result = fread(audio_buf, 1, audio_bytes,fp);

    if (globals.logdecode)
    {
        open_aob_log();
        fprintf(aob_log, "INF;Read %d audio_bytes;;;;;;\n", (int) result);
        //hex2file_csv(aob_log, audio_buf, result > 16 ? 16 : result);
        //fprintf(aob_log, "%s", ";;;;;;;\n");
        close_aob_log();
    }

    // info->midpack_pes_padding + info->lpcm_payload + 38 + info->first/mid/last/pack_lpcm_headerquantity

    int16_t padding_quantity;

    switch(position)
    {
        case FIRST_PACK :
            if (info->firstpack_pes_padding > 0)
            {
              read_pes_padding(fp,info->firstpack_pes_padding); // 0 or  info->firstpack_pes_padding
            }
            break;

        case MIDDLE_PACK :
            read_pes_padding(fp,info->midpack_pes_padding); // 0 or info->midpack_pes_padding
            break;

        case LAST_PACK :
            // 2016- info->lastpack_lpcm_headerquantity - audio_bytes
            padding_quantity = (int16_t) (2016 - info->lastpack_lpcm_headerquantity - audio_bytes);

            if (padding_quantity > 0)
                read_pes_padding(fp, (uint16_t) padding_quantity);
            else
            {
                uint64_t offset = ftello(fp) % 2048;
                if (offset)
                {
                  uint8_t zero[2048-offset];
                  memset(zero, 0, 2048-offset);
                  fwrite(zero, 2048-offset, 1, fp);
                }
            }
            break;
    }

    uint32_t sector_length = (uint32_t) ftello(fp) - offset0;

    if (globals.logdecode)
    {
        open_aob_log();

        fprintf(aob_log, "INF;midpack_lpcm_headerquantity;%d;midpack_pes_padding;%d;bitspersample;%d;samplerate;%d\n",
                info->midpack_lpcm_headerquantity,
                info->midpack_pes_padding,
                info->bitspersample,
                info->samplerate);

        fprintf(aob_log, "INF;sector length: ; %u  bytes;------;title pack: ; %u ; title: ;%d\n",
                sector_length,
                pack_in_title,
                title);

        close_aob_log();
    }

    return(position);
}

int decode_ats()
{
    FILE* fp;

    uint8_t audio_buf[2048];
    uint64_t pack = 0;
    fileinfo_t files;
    int result = 0;

    fp = fopen(globals.aobpath[0],"rb");

    if (fp == NULL)
    {
        foutput("%s%s%s", ERR "Could not open AOB file *", globals.aobpath[0],"*\n");
        return(-1);
    }

    do
    {
        result  = read_pes_packet(fp, &files, audio_buf);
        if (result  == LAST_PACK || result  == FIRST_PACK || result  == MIDDLE_PACK)
        {
            ++pack;
        }
    }
    while (result != LAST_PACK);

    if (globals.maxverbose) fprintf(stderr, DBG "Read %" PRIu64 " PES packets.\n", pack);
    return(0);
}

int create_ats(char* audiotsdir,int titleset,fileinfo_t* files, int ntracks)
{
    FILE* fpout;
    char outfile[CHAR_BUFSIZ+13+1];
    int i=0, pack=0, fileno = 1;
    bool start_of_file = true;
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
        foutput(ERR "Could not open %s\n", files[i].filename);
        EXIT_ON_RUNTIME_ERROR
    }

    audio_read(&files[i], audio_buf, &bytesinbuf);
    
    lpcm_payload = files[i].lpcm_payload;

    files[i].first_sector=0;
    uint64_t totpayload = 0;

    foutput(INF "Processing %s\n",files[i].filename);

    while (bytesinbuf)
    {
        if (bytesinbuf >= lpcm_payload)
        {

            n = write_pes_packet(fpout, &files[i], audio_buf, bytesinbuf, pack_in_title, start_of_file);

            memmove(audio_buf, &audio_buf[n], bytesinbuf - n);
            bytesinbuf -= n;

            ++pack;
            ++pack_in_title;
            start_of_file = false;
        }

        if ((pack > 0) && ((pack % (512 * 1024)) == 0))   
        {
            fclose(fpout);
            ++fileno;
            STRING_WRITE_CHAR_BUFSIZ(outfile, "%s/ATS_%02d_%d.AOB", audiotsdir, titleset, fileno)
            fpout = fopen(outfile,"wb+");
            start_of_file = true;
        }

        if (bytesinbuf < lpcm_payload)
        {
            if (globals.maxverbose)
            {
                foutput("%s %d %s %d\n", 
                        INF "Reading", 
                        AUDIO_BUFFER_SIZE - bytesinbuf,
                        "bytes into buffer at offset",
                        bytesinbuf);
            }
            
            n = audio_read(&files[i],
                           audio_buf,
                           &bytesinbuf);
        
            if (n == 0)   /* We have reached the end of the input file */
            {
                files[i].last_sector = pack;
                audio_close(&files[i]);
                if (files[i].type == AFMT_MLP) free(files[i].mlp_layout);
                ++i;

                if (i < ntracks)
                {
                    /* If the current track is a different audio format, we must start a new title. */

                    if (files[i].newtitle)
                    {
                        // Empty audio buffer

                        write_pes_packet(fpout, &files[i-1], audio_buf, bytesinbuf, pack_in_title, start_of_file);

                        ++pack;
                        bytesinbuf = 0;
                        pack_in_title = 0;
                        totpayload = 0;
                    }

                    files[i].first_sector = files[i-1].last_sector+1;

                    if (audio_open(&files[i]) != 0)
                    {
                        foutput(ERR "Could not open %s\n",files[i].filename);
                        EXIT_ON_RUNTIME_ERROR
                    }

                    start_of_file = true;

                    if (globals.veryverbose)
                        foutput("%s %d %s %d\n", INF "Opening audio buffer at offset", bytesinbuf, "for count: ", AUDIO_BUFFER_SIZE - bytesinbuf);
                    
                    audio_read(&files[i], audio_buf, &bytesinbuf);
                    
                    foutput(INF "Processing %s\n", files[i].filename);
                }
                else
                {
                    /* We have reached the last packet of the last file */
                    if (bytesinbuf == 0)
                    {
                        files[i-1].last_sector = pack-1;
                    }
                    else
                    {
                        start_of_file = false;
                        write_pes_packet(fpout, &files[i-1], audio_buf, bytesinbuf, pack_in_title, start_of_file); // Empty audio buffer.
                        bytesinbuf = 0;
                        ++pack;
                        ++pack_in_title;
                    }
                }
            }
        }
    }

    return(fileno);
}
