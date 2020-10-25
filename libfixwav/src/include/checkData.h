#ifndef CHECKDATA_H_INCLUDED
#define CHECKDATA_H_INCLUDED


int check_sample_count(WaveData *info, WaveHeader *header);
void check_real_size(WaveData *info, WaveHeader *header, globalData*);
int prune(WaveData *info, WaveHeader *header, globalData*);
int pad_end_of_file(WaveData* info, globalData*);
#endif // CHECKDATA_H_INCLUDED
