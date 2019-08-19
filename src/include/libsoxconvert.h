

#ifndef WITHOUT_sox
#include "sox.h"
int soxconvert(char * input, char* output);
int resample(char* in, char* out, unsigned int channels, unsigned int bitrate, unsigned int samplerate);

#endif
