/*
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

#ifndef __AUDIOLIB_H__
#define __AUDIOLIB_H__

#include <mjpeg_types.h>

void audio_shutdown(void);

int audio_init(int a_read, int use_read_write, int a_stereo, int a_size, int a_rate);
long audio_get_buffer_size(void);
void audio_get_output_status(struct timeval *tmstmp, unsigned int *nb_out, unsigned int *nb_err);

int audio_read( uint8_t *buf, int size, int swap, 
			    struct timeval *tmstmp, int *status);
int audio_write( uint8_t *buf, int size, int swap);

void audio_start(void);

char *audio_strerror(void);


#endif /* __AUDIOLIB_H__ */
