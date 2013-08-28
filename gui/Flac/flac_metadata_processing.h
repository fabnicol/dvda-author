#ifndef FLAC_METADATA_PROCESSING_H
#define FLAC_METADATA_PROCESSING_H
#include "format.h"
#include "stream_decoder.h"
#include "dvda.h"
#include "wavfile.h"

class Flac
{
   public :
    Flac(dvda* p=0) {parent=p;}

    void metadata_callback(const FLAC__StreamDecoder *dec, const FLAC__StreamMetadata *meta, void *data);
    int getinfo(fileinfo_t* info);

private:
    dvda* parent;
}


#endif // FLAC_METADATA_PROCESSING_H
