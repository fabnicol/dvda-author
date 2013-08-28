/* adapted and stripped down from dvda-author original code by Dave Chapman (revised version Fabrice Nicol) */

#include "flac_metadata_processing.h"


/* spurious *dec argument is not an anomaly */

void Flac::metadata_callback(const FLAC__StreamDecoder *dec, const FLAC__StreamMetadata *meta, void *data)
{

    QAudioFormat *info = dynamic_cast<QAudioFormat*>(data);

    if (meta->type==FLAC__METADATA_TYPE_STREAMINFO)
    {
        info->setSampleSize((int) meta->data.stream_info.bits_per_sample);
        info->setSampleRate((int) meta->data.stream_info.sample_rate);
        info->setChannelCount(meta->data.stream_info.channels);
    }
}

int Flac::getinfo(QAudioFormat* info, char* filename)
{
    FLAC__StreamDecoder* flac;
    FLAC__StreamDecoderInitStatus  result=0;

    flac=FLAC__stream_decoder_new();

    if (flac!=NULL)
    {

        if (info->type == AFMT_FLAC )

            result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_file  	(
                        /*FLAC__StreamDecoder *  */ 	 flac,
                        /*FILE * */ filename,
                        /*FLAC__StreamDecoderWriteCallback */ 	flac_null_write_callback,
                        /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                        /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                        (void *) info
                    );

        else
        {
            result=/*FLAC__StreamDecoderInitStatus*/ FLAC__stream_decoder_init_ogg_file  	(
                        /*FLAC__StreamDecoder *  */ 	 flac,
                        /*FILE * */ filename,
                        /*FLAC__StreamDecoderWriteCallback */ 	flac_null_write_callback,
                        /*FLAC__StreamDecoderMetadataCallback */ 	flac_metadata_callback,
                        /*FLAC__StreamDecoderErrorCallback  */	flac_error_callback,
                        (void *) info
                    );
        }

    }
    else
    {
        parent->outputTextEdit->append(tr(ERROR_HTML_TAG  " Fatal - could not create FLAC OR OGG FLAC decoder\n"));
    }


    if (result!=FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {

        FLAC__stream_decoder_delete(flac);
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Failed to initialise FLAC decoder\n");
    }

    if (!FLAC__stream_decoder_process_until_end_of_metadata(flac))
    {

        FLAC__stream_decoder_delete(flac);
        EXIT_ON_RUNTIME_ERROR_VERBOSE("[ERR]  Failed to read metadata from FLAC file\n");
    }
    FLAC__stream_decoder_finish(flac);

    if (((info->bitspersample!=16) && (info->bitspersample!=24)) || (info->channels > 2))
    {
        return(1);
    }

    info->numbytes=info->numsamples*info->channels*(info->bitspersample/8);
    FLAC__stream_decoder_delete(flac);
    return(0);
}
