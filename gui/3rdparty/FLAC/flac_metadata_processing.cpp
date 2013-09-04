/* adapted and stripped down from dvda-author original code by Dave Chapman (revised version Fabrice Nicol) */
#ifndef WITHOUT_FLAC
#include "flac_metadata_processing.h"


/* spurious *dec argument is not an anomaly */
//extern "C" {



QAudioFormat *afmt;

void flac__metadata_callback(const FLAC__StreamDecoder *dec, const FLAC__StreamMetadata *meta, void *data)
{

    if (meta->type==FLAC__METADATA_TYPE_STREAMINFO)
    {
        afmt->setSampleSize((int) meta->data.stream_info.bits_per_sample);
        afmt->setSampleRate((int) meta->data.stream_info.sample_rate);
        afmt->setChannelCount(meta->data.stream_info.channels);
    }
}

FLAC__StreamDecoderWriteStatus flac__null_write_callback(const FLAC__StreamDecoder *dec,
        const FLAC__Frame *frame,
        const FLAC__int32 * const buf[],
        void *data)
{
    return (FLAC__StreamDecoderWriteStatus) FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



void flac__error_callback(const FLAC__StreamDecoder *dec,
                         FLAC__StreamDecoderErrorStatus status, void *data=nullptr)
{
    //parent->outputTextEdit->append(QObject::tr(ERROR_HTML_TAG  " FLAC error callback called."));

}

FLAC__StreamDecoder* flac;

void flac_close(int result)
{
    if ((result == FLAC__STREAM_DECODER_INIT_STATUS_OK) &&  (FLAC__stream_decoder_process_until_end_of_metadata(flac)))
    {
         FLAC__stream_decoder_finish(flac);
    }
        FLAC__stream_decoder_delete(flac);
}

void flac_init_file(const QString & filepath, void* f)
{
   flac=FLAC__stream_decoder_new();
    FLAC__StreamDecoderInitStatus result= /*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_file  	(
    /*FLAC__StreamDecoder *  */ 	 flac,
    /*FILE * */ (const char*) filepath.toLocal8Bit(),
    /*FLAC__StreamDecoderWriteCallback */ 	flac__null_write_callback,
    /*FLAC__StreamDecoderMetadataCallback */ 	flac__metadata_callback,
    /*FLAC__StreamDecoderErrorCallback  */	 flac__error_callback,
   (void*) f
);

    flac_close(result);
}


void oggflac_init_file(const QString & filepath, void* f)
{
   flac=FLAC__stream_decoder_new();
    FLAC__StreamDecoderInitStatus    result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_ogg_file  	(
                /*FLAC__StreamDecoder *  */ 	 flac,
                /*FILE * */ (const char*) filepath.toLocal8Bit(),
                /*FLAC__StreamDecoderWriteCallback */ 	flac__null_write_callback,
                /*FLAC__StreamDecoderMetadataCallback */ 	flac__metadata_callback,
                /*FLAC__StreamDecoderErrorCallback  */	flac__error_callback,
                (void *) f
            );
    flac_close(result);
}
#endif
