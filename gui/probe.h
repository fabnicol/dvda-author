#ifndef PROBE_H
#define PROBE_H
#include "common.h"
#include "wavfile.h"

#include "sox.h"

void flac_init_file(const QString &filepath, void* f);
void oggflac_init_file(const QString& filepath, void* f);

class StandardComplianceProbe : public QObject
{
    Q_OBJECT

private:

    QStringList SoXFormatList ={
        "8svx",        "aif",        "aifc",        "aiff",        "aiffc",        "al",        "amb",        "au",        "avr",        "caf",        "cdda",        "cdr",        "cvs",
        "cvsd",        "cvu",        "dat",        "dvms",        "f32",        "f4",        "f64",        "f8",        "fap",        "flac",        "fssd",        "gsm",        "gsrt",
        "hcom",        "htk",        "ima",        "ircam",      "la",        "lpc",        "lpc10",        "lu",        "mat",        "mat4",        "mat5",        "maud",        "mp2",
        "mp3",        "nist",        "ogg",        "paf",        "prc",        "pvf",        "raw",        "s1",        "s16",        "s2",        "s24",        "s3",        "s32",        "s4",
        "s8",        "sb",        "sd2",        "sds",        "sf",        "sl",        "sln",        "smp",        "snd",        "sndfile",        "sndr",        "sndt",        "sou",        "sox",
        "sph",        "sw",        "txw",        "u1",        "u16",        "u2",        "u24",        "u3",        "u32",        "u4",        "u8",        "ub",        "ul",        "uw",        "vms",
        "voc",        "vorbis",        "vox",        "w64",        "wav",        "wavpcm",        "wv",        "wve",        "xa",        "xi"};

    WavFile wavDecoder;
    QAudioFormat audioFileFormat;
    uint audioZone;
    QString codec;

    void getAudioCharacteristics(const QString &filename);
#ifndef WITHOUT_FLAC
   void  getFlacAudioFormat(const QString &filepath);
   void getOggFlacAudioFormat(const QString& filepath);
#endif

#ifndef WITHOUT_SOX
   void getSoxAudioFormat(const QString& filename);
#endif

    void checkStandardCompliance();
    enum audioCharacteristics   { isWav=AFMT_WAVE, isFlac=AFMT_FLAC, isOggFlac=AFMT_OGG_FLAC, isStrictlyDVDAudioCompliant=0x10,  isStrictlyDVDVideoCompliant=0x100, isNonCompliant=0x1000};
    audioCharacteristics decoderCompliance;
    int sampleRate;
    int sampleSize;
    int channelCount;

 public:
    StandardComplianceProbe(QString &filename, uint zone=0)
    {
        audioZone=zone;
        getAudioCharacteristics(filename);
    }
    bool isStandardCompliant();
    QString getSampleRate() {return QString::number(sampleRate);}
    QString getSampleSize() {return QString::number(sampleSize);}
    QString getChannelCount() {return QString::number(channelCount);}
    QString getCodec() {return codec;}
};


#endif // PROBE_H
