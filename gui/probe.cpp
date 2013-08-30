#include "probe.h"
#ifndef WITHOUT_SOX
#include "sox.h"

void StandardComplianceProbe::getSoxAudioFormat(const QString& filename)
{
sox_format_t * ft = sox_open_read((const char*) filename.toLocal8Bit(), NULL, NULL, NULL);
if (ft != nullptr)
{
    audioFileFormat.setCodec(QString(ft->filetype));
    audioFileFormat.setSampleRate((int) ft->signal.rate);
    audioFileFormat.setChannelCount((int) ft->signal.channels);
    audioFileFormat.setSampleSize(ft->encoding.bits_per_sample);
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
     else if (extension == "wav")
      {
              if (wavDecoder.open(filename))
                audioFileFormat=wavDecoder.fileFormat();
     }
     else if (SoXFormatList.contains(extension))
    {
 #ifndef WITHOUT_SOX
       getSoxAudioFormat(filename);
  #endif
    }

      checkStandardCompliance();  // will override default if headers allow it

       if (isStandardCompliant())
           codec= QString(extension);
   #ifndef WITHOUT_SOX
       else                                                 // last-ditch attempt
        {
                  getSoxAudioFormat(filename);
                  codec=QString(extension)+" (guessed format)";
        }
    #endif

}

