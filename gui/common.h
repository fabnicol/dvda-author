#ifndef COMMON_H
#define COMMON_H

#include <QtWidgets>
#include <QAudioDecoder>

#include "fwidgets.h"


#define AFMT_WAVE 1
#define AFMT_FLAC 2
#define AFMT_OGG_FLAC 3
#define NO_AFMT_FOUND 4
#define AFMT_WAVE_GOOD_HEADER 10
#define AFMT_WAVE_FIXED 11

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifndef VERSION
#define VERSION "Crystal echo"
#endif

#define VIDEO 1
#define AUDIO 0


#define Max(X,Y) ((X>Y)? X : Y)
#define Q(X) QMessageBox::about(NULL, "", X);
#define q(X) QMessageBox::about(NULL, "", QString::number(X));
#define v(X) *FString(#X)


class StandardComplianceProbe
{
private:
    QAudioDecoder decoder;
    QAudioFormat audioFileFormat;
    uint audioZone;
    void getAudioCharacteristics(QString &filename);
    enum audioCharacteristics   { isWav=AFMT_WAVE, isFlac=AFMT_FLAC, isOggFlac=AFMT_OGG_FLAC, isStrictlyDVDAudioCompliant=0x10,  isStrictlyDVDVideoCompliant=0x100, isNonCompliant=0x1000};
    audioCharacteristics decoderCompliance;
    int sampleRate;
    int sampleSize;
    int channelCount;

 public:
    StandardComplianceProbe(QString &filename, uint zone)
    {
        audioZone=zone;
        getAudioCharacteristics(filename);
    }
    bool isStandardCompliant();
    QString getSampleRate() {return QString::number(sampleRate);}
    QString getSampleSize() {return QString::number(sampleSize);}
    QString getChannelCount() {return QString::number(channelCount);}
};


class common : public QDialog, public flags
{
  Q_OBJECT

 private:
    QString whatsThisPath;

public:

  common()   {    whatsThisPath=generateDatadirPath("whatsthis.info");  }

  static QString tempdir;
  static QString generateDatadirPath(const char* path);
  static QString generateDatadirPath(QString &path);

  qint64 recursiveDirectorySize(const QString &path, const QString &extension);

  bool removeDirectory(const QString &path);
  bool remove(const QString &path);

  int readFile(QString &path, QStringList &list, int start=0, int stop=-1, int width=0);
  int readFile(const char* path, QStringList &list, int start=0, int stop=-1, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, list, start, stop, width);
  }
  QString readFile(QString &path,  int start=0, int stop=-1, int width=0);
  QString readFile(const char* path,  int start=0, int stop=-1, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, start, stop, width);
  }
static void writeFile(QString & path, const QStringList &list, QFlags<QIODevice::OpenModeFlag> flag= QFile::WriteOnly | QFile::Truncate) ;
void setWhatsThisText(QWidget* widget, int start, int stop);
void openDir(QString path);


protected :
  QString  videoFilePath;
  static FString    htmlLogPath;
  static QStringList extraAudioFilters;


};

#endif // COMMON_H
