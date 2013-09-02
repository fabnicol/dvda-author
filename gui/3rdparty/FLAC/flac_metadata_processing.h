#ifndef FLAC_METADATA_PROCESSING_H
#define FLAC_METADATA_PROCESSING_H


#include "format.h"
#include "stream_decoder.h"
#include "ordinals.h"
#include "dvda.h"
#include "common.h"

void flac_init_file(const QString &filepath, void* f);
void oggflac_init_file(const QString& filepath, void* f);


#endif // FLAC_METADATA_PROCESSING_H
