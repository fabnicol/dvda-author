#ifndef COMMON_H
#define COMMON_H

#include <QtWidgets>

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
#define VERSION "12.12"
#endif

#define VIDEO 1
#define AUDIO 0


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
  static QString generateDatadirPath(const char* path);
  static QString generateDatadirPath(QString &path);
  QTextEdit *outputTextEdit;

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
  enum audioCharacteristics   { isWav=AFMT_WAVE, isFlac=AFMT_FLAC, isOggFlac=AFMT_OGG_FLAC, isDVDAudioCompliant,  isDVDVideoCompliant, isNonCompliant};
  struct fileinfo_t
  {
     quint8 header_size;
     quint8 type;
     quint8 bitspersample;
     quint8 channels;
     audioCharacteristics compliance;
     quint32 samplerate;
     quint64 numsamples;
     quint64 numbytes; // theoretical file size
     quint64 file_size; // file size on disc
     QByteArray filename;
  };

  fileinfo_t *fileinfo;

  bool checkVideoStandardCompliance(QString &filename);
  bool checkAudioStandardCompliance(QString &filename);
  void getAudioCharacteristics(QString &filename);



};

#endif // COMMON_H
