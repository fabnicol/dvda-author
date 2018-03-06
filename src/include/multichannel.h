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

    for (j=0; j < size ; j++)
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

    memcpy(buf,_buf, size);
}

/* WAVFORMAT_EXTENSIBLE SPECS at offset 0x29-0x2B in wav headers */

#ifndef SPEAKER_RESERVED

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
#endif

/* DVD-A multichanenl specs (= info->cga at relative offset 0x26 of AOB 2048-byte sectors, except for the first (at 0x3B) */

#if 0

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
20 	L 	R 	Ls 	Rs 	C 	Lfe                 SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT  |  SPEAKER_BACK_LEFT    | SPEAKER_BACK_RIGHT    | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY  0x3B

cga2wav_channels[21] = {0x4, 0x3, 0x103, 0x33, 0xB, 0x10B, 0x3B, 0x7, 0x107, 0x37, 0xF, 0x10F, 0x3F, 0x107, 0x37, 0xF, 0x10F, 0x3F, 0x3B, 0x37, 0x3B };

#endif

/* so we can convert dwChannelMask into cga and vice-versa */

#endif // MULTICHANNEL_H_INCLUDED
