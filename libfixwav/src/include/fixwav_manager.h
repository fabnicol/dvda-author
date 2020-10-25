#ifndef MANAGER_H
#define MANAGER_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include "c_utils.h"

#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE   64
#endif

#define LITTLE_ENDIAN_WRITE_2_bytes(Y, X)  uint16_copy_reverse(Y, X);

#define _LITTLE_ENDIAN_WRITE_2_bytes(Y, X) LITTLE_ENDIAN_WRITE_2_bytes(Y, X)\
                                           Y+=2;

#define LITTLE_ENDIAN_WRITE_4_bytes(Y, X)  uint32_copy_reverse(Y, X);

#define _LITTLE_ENDIAN_WRITE_4_bytes(Y, X) LITTLE_ENDIAN_WRITE_4_bytes(Y, X)\
                                           Y+=4;



/* Function declarations */


WaveHeader    *fixwav(WaveData *info, WaveHeader *header, globalData*);
int repair_wav(WaveData *info, WaveHeader *header, globalData*);
void print_fixwav_help();


#endif
