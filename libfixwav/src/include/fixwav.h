#ifndef FIXWAV_H
#define FIXWAV_H


#include <assert.h>
#include <stdint.h>


#define TRUE 1
#define FALSE 0
#define YES 1
#define NO  0

#define FIXBUF_LEN 1024
#define BAD_HEADER  1
#define BAD_DATA    2
#define GOOD_HEADER 0
#define FAIL        10
#define COPY_SUCCESS 100
#define NO_PRUNE    4
#define PRUNED      5

/* real size of header in bytes */
#define HEADER_SIZE 44
#define HEADER_EXTENSIBLE_SIZE 80
#define HAVE_STANDARD_HEADER 44
#define WAV_EXTENSION_LENGTH 36
#define HAVE_NON_STANDARD_HEADER 72

#define FW_VERSION "Version 0.2.0\n\n"\
                    "Copyright Fabrice Nicol 2008, 2009-2016 (revised version) \n<fabnicol@users.sourceforge.net>.\nReleased under GPLv3. This software comes under NO GUARANTEE.\nPlease backup your files before running this utility.\n\n"

/* Definitions for Microsoft WAVE format */

#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

#endif
