#ifndef COMMON_H
#define COMMON_H

#include <QtWidgets>

#include "fwidgets.h"
//#include "fwidgets_global.h"



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
#define VERSION "12.12"
#endif

#define VIDEO 1
#define AUDIO 0

/* Build with autoconf DATADIR set by autoconf/qmake or define DATADIR directly in DEFINES=-DDATADIR="..."  or just with DEFINES=-DLOCAL_BUILD */


#ifdef Q_OS_WIN32
   #ifdef LOCAL_BUILD
     #define PREFIX ".."
   #else
     #define PREFIX "C:/Program Files"
   #endif
#else
  #ifdef Q_OS_UNIX
     #ifdef LOCAL_BUILD
       #define PREFIX ".."
     #else
       #define PREFIX "/usr/local"
     #endif
  #endif
#endif

#ifndef DATADIR
#ifdef PREFIX
  #define DATADIR  PREFIX "/share/dvda-author-gui-" VERSION
#else
  #error  "Build using command line: make DEFINES=-DPREFIX='\"any-old-path\"'"
#endif
#endif

#define Max(X,Y) ((X>Y)? X : Y)
#define Q(X) QMessageBox::about(NULL, "", X);
#define q(X) QMessageBox::about(NULL, "", QString::number(X));



class common : public QDialog, public flags
{
  Q_OBJECT

 private:
    QString whatsThisPath;

protected slots:
    void assignAudioCharacterisics(int exitcode, QProcess::ExitStatus status);

public:
  common();
  static QString tempdir;
  QString generateDatadirPath(const char* path);
  QTextEdit *outputTextEdit;

protected :
  static FString    htmlLogPath;
  static QStringList extraAudioFilters;
  enum audioCharacteristics   { isWav=AFMT_WAVE, isFlac=AFMT_FLAC, isOggFlac=AFMT_OGG_FLAC, isDVDAudioCompliant,  isDVDVideoCompliant, isNonCompliant};
  struct fileinfo_t
  {
     u_int8_t header_size;
     u_int8_t type;
     u_int8_t bitspersample;
     u_int8_t channels;
     audioCharacteristics compliance;
     u_int32_t samplerate;
     u_int64_t numsamples;
     u_int64_t numbytes; // theoretical file size
     u_int64_t file_size; // file size on disc
     QByteArray filename;
  };

  fileinfo_t *fileinfo;

  bool checkVideoStandardCompliance(QString &filename);
  bool checkAudioStandardCompliance(QString &filename);
  void getAudioCharacteristics(QString &filename);

  qint64 recursiveDirectorySize(const QString &path, const QString &extension);

  bool removeDirectory(const QString &path);
  bool remove(const QString &path);

  int readFile(QString &path, QStringList &list, int start=0, int stop=0, int width=0);
  int readFile(const char* path, QStringList &list, int start=0, int stop=0, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, list, start, stop, width);
  }
  QString readFile(QString &path,  int start=0, int stop=0, int width=0);
  QString readFile(const char* path,  int start=0, int stop=0, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, start, stop, width);
  }
void setWhatsThisText(QWidget* widget, int start, int stop);
void openDir(QString path);

};

#endif // COMMON_H
