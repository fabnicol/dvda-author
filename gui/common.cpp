#include "common.h"

QStringList common::extraAudioFilters=QStringList();
FString common::htmlLogPath;
QString common::tempdir=QDir::homePath ()+QDir::separator()+"tempdir";  // should be equal to main app globals.settings.tempdir=TEMPDIR;



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


void StandardComplianceProbe::parseBuffer()
{
audioFileFormat=decoder.fileFormat();

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

void StandardComplianceProbe::getAudioCharacteristics(QString &filename)
{
     if (filename.isEmpty()  || audioZone > 1)
     {
         decoderCompliance=isNonCompliant;
         return;
     }

    if (decoder.open(filename))  parseBuffer();
}

