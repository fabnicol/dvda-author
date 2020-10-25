#ifndef CHECKPARAMETERS_H_INCLUDED
#define CHECKPARAMETERS_H_INCLUDED
#include "fixwav_manager.h"

int user_control(WaveData *info, WaveHeader *header, globalData*);
int auto_control(WaveData *info, WaveHeader *header, globalData*);
void regular_test(WaveHeader *head, int* regular, globalData*);
#endif // CHECKPARAMETERS_H_INCLUDED
