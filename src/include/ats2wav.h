#ifndef LIBATS2WAV_H
#define LIBATS2WAV_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <dirent.h>
#include <inttypes.h>
#include <stdint.h>
#include "structures.h"
#include "libiberty.h"
#include "fixwav_manager.h"

#define BUFFER_SIZE 3*2048

int get_ats_audio_i(int, fileinfo_t* [9][99], WaveData *info);
int get_ats_audio(bool use_ifo_files, const extractlist* extract);

#endif
