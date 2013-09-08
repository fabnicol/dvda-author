#ifndef COMMON_H
#define COMMON_H

#include <QtWidgets>


#include "fwidgets.h"


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
#define v(X) (*FString(#X))


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

  qint64 getDirectorySize(const QString &path, const QString &extension="");

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
qint64 getFileSize(const QString &, const QString& ="");


protected :
  QString  videoFilePath;
  static FString    htmlLogPath;
  static QStringList extraAudioFilters;

#ifdef CDRECORD_PATH_WIN32
 /* insert executable at root of windows package */
  QString cdRecordCommandStr=QDir::toNativeSeparators(QDir::currentPath ()+"/bindir/"+ QString("cdrecord.exe"));
#else
 #ifndef CDRECORD_PATH
  #define CDRECORD_PATH "/opt/schily/bin"
 #endif
   QString cdRecordCommandStr=QString(CDRECORD_PATH) + QString("/cdrecord");
#endif

};

#endif // COMMON_H
