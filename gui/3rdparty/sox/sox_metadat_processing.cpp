 
#ifndef WITHOUT_SOX

#include "common.h"
#include "sox.h"
#include "dvda.h"


int dvda::getSoxAudioFormat(const char* filename)
{
sox_format_t * ft = sox_open_read(filename, NULL, NULL, NULL);

audioFormat.setCodec(QString(ft->filetype));
audioFormat.setSampleRate((int) ft->signal.rate);
audioFormat.setChannelCount((int) ft->signal.channels);
audioFormat.setSampleSize(ft->encoding.bits_per_sample);
}
#endif
