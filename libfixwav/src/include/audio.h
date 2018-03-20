#ifndef _AUDIO_H
#define _AUDIO_H


#define AFMT_WAVE 1
#define NO_AFMT_FOUND 4
#define AFMT_WAVE_GOOD_HEADER 10
#define AFMT_WAVE_FIXED 11



int fixwav_getinfo(char* filename);

int launch_sox(char** filename);
int extract_audio_info(fileinfo_t *info, uint8_t * header);

#endif
