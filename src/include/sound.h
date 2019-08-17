#ifndef SOUND_H
#define SOUND_H
#include <stdio.h>
#include "structures.h"


int launch_lplex_soundtrack(pic* img,const char* create_mode);

int launch_lplex_hybridate(const pic* img, const char* create_mode,
                           const char*** trackpath,const uint8_t* ntracks,
                           const char*** slidepath, uint8_t* nslides,
                           int ntitlesets);

int audit_soundtrack(char* path, bool strict);

#endif
