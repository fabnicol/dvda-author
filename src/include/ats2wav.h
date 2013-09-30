#ifndef LIBATS2WAV_H
#define LIBATS2WAV_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef __WIN32__
#include <fcntl.h>
#include <unistd.h>
#endif
#include <inttypes.h>
#include <stdint.h>
#include "structures.h"
#include "libiberty.h"

typedef struct
{
    FILE* fpout;
    const char* filename;
    int samplerate;
    uint8_t channels;
    uint8_t bitspersample;
    int ntracks;
    int started;
    uint64_t last_sector;
    uint64_t first_sector;
    uint64_t numsamples;
    uint64_t numbytes;
    uint64_t byteswritten;
    uint64_t pts_length;

} _fileinfo_t;

#define BUFFER_SIZE 3*2048
int ats2wav(const char* filename, const char* outdir, extractlist *extract, char* player);


#endif
