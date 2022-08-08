#ifndef REPAIR_H_INCLUDED
#define REPAIR_H_INCLUDED

#include "fixwav_manager.h"

int launch_repair(WaveData *info, WaveHeader *header, globalData*);
int dvda_write_header(WaveData *info, WaveHeader* header, globalData*);

#endif
// REPAIR_H_INCLUDED


