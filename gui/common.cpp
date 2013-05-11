#include "common.h"


QStringList common::extraAudioFilters=QStringList();
FString common::htmlLogPath;
QString common::tempdir;

common::common()
{
    outputTextEdit=new QTextEdit;
    outputTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    outputTextEdit->setAcceptDrops(false);
    outputTextEdit->setMinimumHeight(200);
    whatsThisPath=generateDatadirPath("whatsthis.info");
}


bool common::remove(const QString &path)
{

    if (QFileInfo(path).isDir())
    {
        return  removeDirectory(path) ;
    }
    return false;
}

bool common::removeDirectory(const QString &path)
{
    if (path.isEmpty()) return false;

    QDir dir(path);

    foreach (QFileInfo fileinfo, dir.entryInfoList(QDir::AllEntries|QDir::System|QDir::Hidden))
    {
        if (fileinfo.fileName() == "." || fileinfo.fileName() == "..") continue;


        if (fileinfo.isFile()||fileinfo.isSymLink())
            dir.remove(fileinfo.absoluteFilePath());
        else
            if (fileinfo.isDir())
            {
                QString p;
                if (dir.rmpath(p=fileinfo.absoluteFilePath()) == false)
                    removeDirectory(p);
            }
    }

    dir.rmdir(path);
    return true;

}

qint64 common::recursiveDirectorySize(const QString &path, const QString &extension)
{

    QDir dir(path);
    qint64 size=0;

    QStringList filters;
    filters+=extension;

    foreach (QString file, dir.entryList(filters, QDir::Files))
        size+=QFileInfo(dir, file).size();

    foreach (QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        size+=recursiveDirectorySize(path+QDir::separator()+subDir, extension);

    return size;

}

void common::writeFile(QString & path, const QStringList &list, QFlags<QIODevice::OpenModeFlag> flag)
{
    QFile data(path);
    if (data.open(flag))
    {
        QTextStream out(&data);
        QStringListIterator i(list);
        while (i.hasNext())
            out << i.next() << "\n";
    }
    data.close();
}


int common::readFile(QString &path, QStringList &list, int start, int stop, int width)
{
QFile file(path);
int j=0;
if (file.open(QIODevice::ReadOnly | QIODevice::Text))
{
  QTextStream in(&file);
  in.seek(0);
  while (++j < start)  in.readLine();
  while (!in.atEnd() )
  {
      QString line = in.readLine(width);
      list << line;
      if  (j == stop) break;
      j++;
  }
}
else QMessageBox::warning(this, tr("Warning"), tr("WhatsThis file could not be opened: ") + path );
file.close();
return j;

}

QString common::readFile(QString &path,  int start, int stop, int width)
{
QFile file(path);
QStringList L=QStringList();
readFile(path, L, start, stop, width);
QString string=L.join("\n");
return string;
}

QString common::generateDatadirPath(QString &path)
{
  QString pathstr= QDir::cleanPath(  QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + path);
  return pathstr;
}

QString common::generateDatadirPath(const char* path)
{
  const QString str= QString(path);
  QString pathstr= QDir::cleanPath(  QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + str);
  return pathstr;
}

void common::setWhatsThisText(QWidget* widget, int start, int stop)
{
  widget->setWhatsThis("<html>"+readFile(whatsThisPath, 2, 2)+readFile(whatsThisPath, start, stop)+"</html>");
}

void common::openDir(QString path)
{
   if (path.isEmpty()) return;
  if (!QFileInfo(path).isDir())
    {
      QMessageBox::warning(this, "", path + " is not a directory");
      return;
    }

QUrl url("file:///" + path);
QDesktopServices::openUrl(url);
}

// dynamic allocation is obligatory
//  ImageViewer *v = new ImageViewer(videoMenuLineEdit->setXmlFromWidget());
//  v->show();



bool common::checkVideoStandardCompliance(QString &filename)
{
  return true;
  //return  (fileinfo->compliance == isDVDVideoCompliant);
}

bool common::checkAudioStandardCompliance(QString &filename)
{
  return true;
  //return (fileinfo->compliance == isDVDAudioCompliant);
}

void common::getAudioCharacteristics(QString &filename)
{
  fileinfo = new fileinfo_t;
  fileinfo->filename=filename.toLocal8Bit();

  /* filepath is non-locale char compliant, so that dvda-author cannot be used for input filename format reasons */
  if (QString(fileinfo->filename) != filename) fileinfo->compliance=isNonCompliant;


  QProcess process;
  QStringList args=QStringList() << "--test" << filename;
  process.start("dvda-author", args);
  connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(assignAudioCharacteristics(int, QProcess::ExitStatus )));

  /*  */

}

void common::assignAudioCharacterisics(int exitcode, QProcess::ExitStatus status)
{

  /* ecode dvda-author --test filename exit status as : channels << 4 [byte 1] | samplerate << 1 [byte 2-4] | bitspersample [byte 5] */

  if (status != QProcess::NormalExit) return;
  fileinfo->bitspersample = exitcode & 0xFF;
  fileinfo->samplerate = (exitcode >> 1) & 0xFFFFFF;
  fileinfo->channels = (exitcode >> 4) & 0xFF;
  if (((fileinfo->channels ==0) || (fileinfo->samplerate == 0) || (fileinfo->bitspersample == 0)) ||
      (fileinfo->channels > 6) ||
      ((fileinfo->bitspersample != 16) || (fileinfo->bitspersample != 24)))
    {
      fileinfo->compliance=isNonCompliant;
      return;
    }


  if ((fileinfo->samplerate = 96000) || (fileinfo->samplerate == 48000) || (fileinfo->samplerate == 44100) || (fileinfo->samplerate == 88200) || (fileinfo->samplerate == 176400) || (fileinfo->samplerate == 192000))
         fileinfo->compliance=isDVDAudioCompliant;
  else
    if ((fileinfo->samplerate == 96000) || (fileinfo->samplerate == 48000))
       fileinfo->compliance=isDVDVideoCompliant;
  else
    fileinfo->compliance=isNonCompliant;

  return;
}
