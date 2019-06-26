#ifndef CHECKDATA_H_INCLUDED
#define CHECKDATA_H_INCLUDED


int check_sample_count(WaveData *info, WaveHeader *header);
void check_real_size(WaveData *info, WaveHeader *header);
int prune(WaveData *info, WaveHeader *header);
int pad_end_of_file(WaveData* info);
#endif // CHECKDATA_H_INCLUDED
