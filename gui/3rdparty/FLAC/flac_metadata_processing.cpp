/* adapted and stripped down from dvda-author original code by Dave Chapman (revised version Fabrice Nicol) */
#ifndef WITHOUT_FLAC
#include "flac_metadata_processing.h"


/* spurious *dec argument is not an anomaly */
//extern "C" {


#include "format.h"
#include "stream_decoder.h"
#include "ordinals.h"
//#include "dvda.h"

#include "dvda.h"
#include "common.h"

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

void init_file(const char* filepath, void* f)
{
   FLAC__StreamDecoder* flac;
   flac=FLAC__stream_decoder_new();
    FLAC__StreamDecoderInitStatus result= /*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_file  	(
    /*FLAC__StreamDecoder *  */ 	 flac,
    /*FILE * */ filepath,
    /*FLAC__StreamDecoderWriteCallback */ 	flac__null_write_callback,
    /*FLAC__StreamDecoderMetadataCallback */ 	flac__metadata_callback,
    /*FLAC__StreamDecoderErrorCallback  */	 flac__error_callback,
   (void*) f
);

    if (result!=FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {

        FLAC__stream_decoder_delete(flac);
        //parent->outputTextEdit->append(QObject::tr(ERROR_HTML_TAG  "Failed to initialise FLAC decoder\n"));
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(flac))
    {

        FLAC__stream_decoder_delete(flac);
        //parent->outputTextEdit->append(QObject::tr(ERROR_HTML_TAG "Failed to read metadata from FLAC file\n"));
    }
    FLAC__stream_decoder_finish(flac);
    FLAC__stream_decoder_delete(flac);

}

void  dvda::getAudioFormat(const char* filepath)
{



//    if (flac!=NULL)
//    {

     //   if (audioFormat->type == AFMT_FLAC )
afmt=&(this->audioFormat);
init_file(filepath, nullptr);
int a=this->audioFormat.channelCount();
q(a)

//        else
//        {
//            result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_ogg_file  	(
//                        /*FLAC__StreamDecoder *  */ 	 flac,
//                        /*FILE * */ filepath,
//                        /*FLAC__StreamDecoderWriteCallback */ 	null_write_callback,
//                        /*FLAC__StreamDecoderMetadataCallback */ 	metadata_callback,
//                        /*FLAC__StreamDecoderErrorCallback  */	error_callback,
//                        (void *) audioFormat
//                    );
//        }

//    }
//    else
//    {
//        //parent->outputTextEdit->append(QObject::tr(ERROR_HTML_TAG  " Fatal - could not create FLAC OR OGG FLAC decoder\n"));
//    }


   // return audioFormat;
}
//}
#endif
