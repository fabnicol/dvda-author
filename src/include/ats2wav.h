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
#include "fixwav_manager.h"

#define BUFFER_SIZE 3*2048

int ats2wav(short ngroups_scan, const char* audiots_dir, const char* outdir, const extractlist* extract);

int get_ats_audio_i(int, fileinfo_t* [9][99], WaveData *info);
int get_ats_audio(bool use_ifo_files);

#endif
