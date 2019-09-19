/* Multichannel reference tables for buffer conversions
 *
 * Copyright Lee and Tim Feldkamp, 2008;
 * Modified by Fabrice Nicol, 20008 <fabrnicol@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdint.h>
#include <string.h>

#ifndef MULTICHANNEL_H_INCLUDED
#define MULTICHANNEL_H_INCLUDED
// used for multichannel encoding/decoding, in audio.c and libats2wav.c

// To enable inlining of the same function in two distinct files, it is necessary to place it in a header
// Performs permutation of buf by replacing buf[j] with associate value, depending on pits per second, channel number,
// and whether one converts to AOB or extracts from AOB.

static inline void permutation(uint8_t *buf,
                               uint8_t *_buf,
                               int bits_per_second_flag,
                               uint8_t channels,
                               const uint8_t reference_table[][6][36],
                               int size)
{
    int j;

    for (j=0; j < size ; ++j)
    {
        _buf[j] = buf[reference_table[bits_per_second_flag][channels-1][j]];
    }

#if 0
    for (j=0; j < size ; j++)
    {
        fprintf(stderr, "%X ", buf[j]);
    }
        fprintf(stderr, "\n");
    for (j=0; j < size ; j++)
    {
        fprintf(stderr, "%X ", _buf[j]);
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "channels = %d, bps = %d\n", channels, bits_per_second_flag);
    exit(0);

#endif

    memcpy(buf,_buf, (size_t) size);
}


static inline void permutation_merged(uint8_t *buf,
                               uint8_t *_buf,
                               int bits_per_second_flag,
                               uint8_t channels,
                               const uint8_t reference_table[][6][36][2],
                               int size)
{
    int j;

    //  buf is double array of channel times 4 bytes (16-bit case) or 6 bytes (24-bit case): buf[channels][bitrate / 4]
    // except for 16-bit ch 1/2

    for (j = 0; j < size ; ++j)
    {
       int  channel     = reference_table[bits_per_second_flag][channels-1][j][0];
       int byte_index   = reference_table[bits_per_second_flag][channels-1][j][1];
       _buf[j] = buf[channel * (bits_per_second_flag ? 6 : 4) + byte_index];
    }

    memcpy(buf, _buf, (size_t) size);
}


/* WAVFORMAT_EXTENSIBLE SPECS at offset 0x29-0x2B in wav headers */


# define SPEAKER_FRONT_LEFT             0x1
# define SPEAKER_FRONT_RIGHT            0x2
# define SPEAKER_FRONT_CENTER           0x4
# define SPEAKER_LOW_FREQUENCY          0x8
# define SPEAKER_BACK_LEFT              0x10
# define SPEAKER_BACK_RIGHT             0x20
# define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
# define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
# define SPEAKER_BACK_CENTER            0x100
# define SPEAKER_SIDE_LEFT              0x200
# define SPEAKER_SIDE_RIGHT             0x400
# define SPEAKER_TOP_CENTER             0x800
# define SPEAKER_TOP_FRONT_LEFT         0x1000
# define SPEAKER_TOP_FRONT_CENTER       0x2000
# define SPEAKER_TOP_FRONT_RIGHT        0x4000
# define SPEAKER_TOP_BACK_LEFT          0x8000
# define SPEAKER_TOP_BACK_CENTER        0x10000
# define SPEAKER_TOP_BACK_RIGHT         0x20000
# define SPEAKER_RESERVED               0x80000000

/* DVD-A multichanenl specs (= info->cga at relative offset 0x26 of AOB 2048-byte sectors, except for the first (at 0x3B) */

#if 0
Wav channels are interleave in the order of the above table from top to bottom.


dwChannelMask is normally at offset 40 (0x28) on 4 bytes in WAV_FORMAT_EXTENSIBLE headers

Issue is that CGA in DVD-Ausio is more flexible with form example cgas 10 and 13 or 12 and 17 corresponding to same dwChannelMask
This is caused by chroup-splitting in the DVD-A norm, which is currently unsupported in dvda-author.
As a result the mapping is not bijective.
Using higer cga values to disambiguate.

// Channel group assignment (CGA)
//
//        1    2        3         4        5       6
//0       M
//1       L     R
//2       Lf    Rf      S*
//3       Lf    Rf      Ls*      Rs*
//4       Lf    Rf      Lfe*
//5       Lf    Rf      Lfe*     S*
//6       Lf    Rf      Lfe*     Ls*      Rs*
//7       Lf    Rf      C*
//8       Lf    Rf      C*       S*
//0x9     Lf    Rf      C*       Ls*      Rs*
//0xA-10  Lf    Rf      C*       Lfe*
//0xB-11  Lf    Rf      C*       Lfe*     S*
//0xC-12  Lf    Rf      C*       Lfe*     Ls*      Rs*
//0xD-13  Lf    Rf      C        S*
//0xE-14  Lf    Rf      C        Ls*      Rs*
//0xF-15  Lf    Rf      C        Lfe*
//0x10-16 Lf    Rf      C        Lfe*     S*
//0x11-17 Lf    Rf      C        Lfe*     Ls*      Rs*
//0x12-18 Lf    Rf      Ls       Rs       Lfe*
//0x13-19 Lf    Rf      Ls       Rs       C*
//0x14-20 Lf    Rf      Ls       Rs       C*       Lfe*
// Keys:
// * In Group2
// Each group must have either same sample rate or be even multiples (e.g. 96kHz/48 kHz or 88.2 kHz/44.1 kHz)
// Within groups, bitrate may differ but sample rate cannot.
// M: Mono
// Lf: Left front
// Rf: Right front
// Ls: Left surround (behind)
// Rs: Right front
// C:  Center
// Lfe: Low Frequency Effect (Subwoofer)
// S: Surround (just one behind)
// Ls: Left  surround
// Rs: Right surround

DVD-A                                       WAV_FORMAT_EXTENSIBLE

0 	C                                       SPEAKER_FRONT_CENTER                                                                                                    0x4
1 	L 	R                                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT                                                                                0x3
2 	L 	R 	S                               SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_CENTER                                                         0x103
3 	L 	R 	Ls 	Rs                          SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT                                      0x33
4 	L 	R 	Lfe                             SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY                                                       0xB
5 	L 	R 	Lfe 	S                       SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER                                 0x10B
6 	L 	R 	Lfe 	Ls 	Rs                  SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT              0x3B
7 	L 	R 	C                               SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER                                                        0x7
8 	L 	R 	C 	S                           SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_BACK_CENTER                                 0x107
9 	L 	R 	C 	Ls 	Rs                      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_BACK_LEFT | SPEAKER_RIGHT_LEFT              0x37
10 	L 	R 	C 	Lfe                         SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY                               0xF
11 	L 	R 	C 	Lfe 	S                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER         0x10F
12 	L 	R 	C 	Lfe 	Ls 	Rs              SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT  0x3F
13 	L 	R 	C 	S                           SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_BACK_CENTER                                 0x107
14 	L 	R 	C 	Ls 	Rs                      SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT              0x37
15 	L 	R 	C 	Lfe                         SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY                               0xF
16 	L 	R 	C 	Lfe 	S                   SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER         0x10F
17 	L 	R 	C 	Lfe 	Ls 	Rs              SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  | SPEAKER_FRONT_CENTER  | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT 0x3F
18 	L 	R 	Ls 	Rs 	Lfe                     SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  |  SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT    | SPEAKER_LOW_FREQUENCY       0x3B
19 	L 	R 	Ls 	Rs 	C                       SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  |  SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT    | SPEAKER_FRONT_CENTER        0x37
20 	L 	R 	Ls 	Rs 	C 	Lfe                 SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  |  SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT    | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY  0x3F


#endif

/* so we can convert dwChannelMask into cga and vice-versa */

#endif // MULTICHANNEL_H_INCLUDED
