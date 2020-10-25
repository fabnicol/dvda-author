#ifndef SOUND_H
#define SOUND_H
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "structures.h"
#include "fixwav.h"
#include "fixwav_manager.h"
#include "c_utils.h"
#include "auxiliary.h"
#include "launch_manager.h"

int launch_lplex_soundtrack(pic* img,const char* create_mode, globalData*);

int launch_lplex_hybridate(const pic* img, const char* create_mode,
                           const char*** trackpath,const uint8_t* ntracks,
                           const char*** slidepath, uint8_t* nslides,
                           int ntitlesets, globalData*);

int audit_soundtrack(char* path, bool strict, globalData*);
WaveData* wavedata_copy(const WaveData* w);
void wavedata_clean(WaveData* w);
WaveData* wavedata_init(void);
void filestat_clean(filestat_t* f);
filestat_t filestat_copy(filestat_t f);
#endif
