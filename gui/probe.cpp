#include "probe.h"
#ifndef WITHOUT_SOX
#include "sox.h"
#include "flac_metadata_processing.h"


void StandardComplianceProbe::getSoxAudioFormat(const QString& filename)
{
sox_format_t * ft = sox_open_read((const char*) filename.toLocal8Bit(), NULL, NULL, NULL);
if (ft != nullptr)
{
    audioFileFormat.setCodec(QString(ft->filetype));
    audioFileFormat.setSampleRate((int) ft->signal.rate);
    audioFileFormat.setChannelCount((int) ft->signal.channels);
    audioFileFormat.setSampleSize(ft->signal.precision);
    sox_close(ft);
}
else
{
    audioFileFormat.setCodec("");
    audioFileFormat.setSampleRate(0);
    audioFileFormat.setChannelCount(0);
    audioFileFormat.setSampleSize(0);
}
}
#endif

#ifndef WITHOUT_FLAC
#include "flac_metadata_processing.h"

extern QAudioFormat *afmt;  /* awkward yet unavoidable as using audioFormat (dvda::)member brings about intricacies as to C/C++ linking and type casts. */

/* C++ wrappers of C functions (yet undeclared as being C */

void  StandardComplianceProbe::getFlacAudioFormat(const QString& filepath)
{
    afmt=&(this->audioFileFormat);
    flac_init_file(filepath, nullptr);
}

void  StandardComplianceProbe::getOggFlacAudioFormat(const QString& filepath)
{
    afmt=&(this->audioFileFormat);
    oggflac_init_file(filepath, nullptr);
}

#endif

void StandardComplianceProbe::checkStandardCompliance()
{

sampleSize=audioFileFormat.sampleSize();
sampleRate=audioFileFormat.sampleRate();
channelCount=audioFileFormat.channelCount();


if (((channelCount ==0) || ( sampleRate == 0) || (sampleSize == 0)) ||
    (channelCount > 6) ||
    ((sampleSize != 16) && (sampleSize != 24)))
  {
    decoderCompliance=isNonCompliant;
    return;
  }

switch (audioFileFormat.sampleRate())
{
case 96000:
case 48000: decoderCompliance=isStrictlyDVDVideoCompliant;
                      break;
case 44100:
case 88200:
case 176400:
case 192000:
                         decoderCompliance = isStrictlyDVDAudioCompliant;
                         break;
default:

  decoderCompliance=isNonCompliant;
}

return;
}

bool StandardComplianceProbe::isStandardCompliant()
{
    if (audioZone == VIDEO)
       return (decoderCompliance == isStrictlyDVDVideoCompliant) ;

    return (decoderCompliance == isStrictlyDVDAudioCompliant ||  decoderCompliance == isStrictlyDVDVideoCompliant);
}

void StandardComplianceProbe::getAudioCharacteristics(const QString &filename)
{
    QString sox_guessed_codec=QString();

    decoderCompliance=isNonCompliant;   //default

    if (filename.isEmpty()  || audioZone > 1)
                       return;

      QString extension=filename.split(".").last();

      if (extension == "flac")
      {
#ifndef WITHOUT_FLAC
         getFlacAudioFormat(filename);
#endif
      }
      else if (extension == "oga")
      {
    #ifndef WITHOUT_FLAC
             getOggFlacAudioFormat(filename);
    #endif
      }

 #ifndef WITHOUT_SOX
     else if (SoXFormatList.contains(extension))
    {
       getSoxAudioFormat(filename);
    }
    else                                                 // last-ditch attempt
     {
               getSoxAudioFormat(filename);
               sox_guessed_codec=audioFileFormat.codec();
      }
 #endif

      codec= (sox_guessed_codec.isEmpty())?
                     ((audioFileFormat.codec().isEmpty())? filename:  audioFileFormat.codec()):
                      sox_guessed_codec + " (guessed format)";

      checkStandardCompliance();
}

