#ifndef SOUND_H
#define SOUND_H

#include "structures.h"
#include <stdio.h>


int launch_lplex_soundtrack(pic* img,const char* create_mode);

int launch_lplex_hybridate(const pic* img, const char* create_mode,
                           const char*** trackpath,const uint8_t* ntracks,
                           const char*** slidepath, uint8_t* nslides,
                           int ntitlesets);

int audit_soundtrack(char* path, _Bool strict);
int resample(const char* in, const char* out,const char* bitrate,const char* samplerate);
int standardize_wav_header(char* path);

#endif
