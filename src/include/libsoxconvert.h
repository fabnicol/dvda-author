

#ifndef WITHOUT_sox
#include "sox.h"
#include <stdlib.h>
int soxconvert(char * input, char* output, globalData* globals);
int resample(char* in, char* out, unsigned int channels, unsigned int bitrate, unsigned int samplerate);

#endif
