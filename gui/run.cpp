#include "dvda.h"
#include "fwidgets.h"






QStringList dvda::createCommandLineString(int commandLineType)
{
 QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);
 QStringList commandLine;

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();
      int itemCommandLineType=item->commandLineType & flags::commandLineMask;
      if ((itemCommandLineType & commandLineType) == itemCommandLineType)
        {
           commandLine +=  item->commandLineStringList();
        }
    }

  return commandLine;
}


void dvda::run()
{
  QStringList args;
  QString command;

  progress->show();
  progress2->hide();
  progress3->hide();


  if (dvda::totalSize[AUDIO] + dvda::totalSize[VIDEO] == 0)
    {
      process.kill();
//      processFinished(EXIT_FAILURE);
      return;
    }

  args << "-P0" << "-o" << common::tempdir+"/output"  << "-g" << "/home/fab/Audio/pn.wav" << "/home/fab/Audio/pn.wav";


  args << createCommandLineString(dvdaCommandLine|createIso|createDisc);

  //args << createCommandLineString(lplexFiles).split("-ts");

  outputTextEdit->append(tr(MSG_HTML_TAG "Processing input directory..."));
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Size of Audio zone input %1").arg(QString::number(dvda::totalSize[AUDIO])));
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Size of Video zone input %1").arg(QString::number(dvda::totalSize[VIDEO])));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : dvda-author %1").arg(command));

  outputType="DVD-Audio authoring";

  process.setProcessChannelMode(QProcess::MergedChannels);

  process.start(/*"konsole"*/ "dvda-author", args);

  progress->setTarget(Hash::wrapper["targetDir"]->toQString());
  progress2->setTarget(Hash::wrapper["mkisofsPath"]->toQString());
  //progress->setTarget(Hash::wrapper[""]->toQString());

  progress->setReference(dvda::totalSize[AUDIO]+dvda::totalSize[VIDEO]);
  progress->start(500);

}

void dvda::runLplex()
{
  QStringList args;
  QString command;


  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      if (item->commandLineType == lplexFiles)
        args << item->commandLineStringList();
    }

  outputTextEdit->append(tr(MSG_HTML_TAG "Processing input directory..."));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  outputType="audio DVD-Video disc authoring";
  process.start(/*"konsole"*/ "Lplex", args);

}

void dvda::processFinished(int exitCode)
{

  if (exitCode == EXIT_FAILURE)
  {
        outputTextEdit->append(ERROR_HTML_TAG  +outputType + tr(": crash exit"));
        progress->stop();
       return;
  }

if (process.exitStatus() == QProcess::CrashExit) return;

if (outputType == "DVD-Audio authoring")
 {
    outputTextEdit->append(INFORMATION_HTML_TAG "\n" + outputType + tr(" completed, output directory is %1").arg(v(targetDir)));
    qint64 fsSize=getDirectorySize(v(targetDir), "*.*");

    progress2->setReference(fsSize);

    outputTextEdit->append(tr(INFORMATION_HTML_TAG "File system size: ")+ QString::number(fsSize) + " Bytes ("+ QString::number(((float)fsSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");

    if (!v(runMkisofs).isTrue()) return;
    runMkisofs();
}
else
 if (outputType == "Disc image authoring")
    {
        outputTextEdit->append(INFORMATION_HTML_TAG "\n" + outputType + tr(" completed. Image file is: %1").arg(v(mkisofsPath)));
        qint64 fsSize=getFileSize(v(mkisofsPath));
        outputTextEdit->append(tr(INFORMATION_HTML_TAG"File size: ")+ QString::number(fsSize) + " Bytes ("+ QString::number(((float)fsSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");
    }
else
 if (outputType == "Burning")
 {
     if ((process.exitStatus() == QProcess::NormalExit) && (process.exitCode() == 0))
        outputTextEdit->append(INFORMATION_HTML_TAG "\n" + outputType + tr(" completed."));
     else
        outputTextEdit->append(ERROR_HTML_TAG "\n" + outputType + tr(" issues: check disc."));
 }

}


void dvda::runMkisofs()
{
    QStringList  argsMkisofs;

    progress2->show();

    if (v(targetDir).isEmpty() || v(mkisofsPath).isEmpty()) return;

     argsMkisofs << "-dvd-audio" << "-o" << v(mkisofsPath) << v(targetDir);

   outputTextEdit->append(tr(MSG_HTML_TAG "mkisofs command line : %1").arg(argsMkisofs.join(" ")));

   outputType="Disc image authoring";

   progress2->start(500);

   process.start("/usr/bin/mkisofs", argsMkisofs);

 }

void dvda::feedLog()
{
    char data[5000]={0};

    process.readLine(data, 5000*sizeof(char));

    QRegExp reg("\rTrack.*[ ]([0-9]*) of [ ]*([0-9]*) .*written");

   QString  text=QString(data).replace(reg,  "\\1/\\2\n");

    outputTextEdit->append(">"+text);

   // if (process.atEnd()) timer->stop();
}



void dvda::runCdrecord()
{

  if ((v(burnDisc).isFalse())||(v(dvdwriterPath).isEmpty())) return;

  QStringList argsCdrecord;


  if (v(runMkisofs).isFalse())
    {
      QMessageBox::warning(this, tr("Record"), tr("You need to create an ISO file first to be able to burn a DVD-Audio disc."), QMessageBox::Ok );
      return;
    }


  if (v(dvdwriterPath).isEmpty())
    {
      QMessageBox::warning(this, tr("Record"), tr("You need to enter the path to a valid DVD writer device."), QMessageBox::Ok );
      return;
    }

  QFileInfo f(v(mkisofsPath));
  f.refresh();

  if (! f.isFile())
    {
      QMessageBox::warning(this, tr("Record"), tr("No valid ISO file path was entered:\n %1").arg(v(mkisofsPath)), QMessageBox::Ok );
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "\nBurning disc...please wait."));

  argsCdrecord << "dev="+ v(dvdwriterScan) <<
                              ((v(ejectDvd)=="yes")?"-eject":"") <<
                               ((v(verboseCdrecord)=="yes")?"-v":"") <<
                               ((v(overBurnDvd)=="yes")?"-overburn":"") <<
                               ((v(blankDvd)=="yes")?"blank=fast":"") <<
                                v(mkisofsPath);

  outputTextEdit->append(tr(MSG_HTML_TAG "Command line: cdrecord %1").arg(argsCdrecord.join(" ")));
  outputType="Burning";

#ifndef CDRECORD_PATH
 #ifdef CDRECORD_LOCAL_PATH
    process.start(QDir::toNativeSeparators(QDir::currentPath ()+"/bindir/"+ QString("cdrecord.exe")),  argsCdrecord);
 #else
    #define CDRECORD_PATH "/opt/schily/bin"
     process.start(QString(CDRECORD_PATH) + QString("/cdrecord"),  argsCdrecord);
 #endif
 #else
  process.start(QString(CDRECORD_PATH) + QString("/cdrecord"),  argsCdrecord);
#endif


  timer = new QTimer(this);
  //timer->start(100);

  //connect(timer, &QTimer::timeout, this, &dvda::feedLog);


}


void dvda::killProcess()
{
  process.kill();
  outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
}


void dvda::extract()
{
  QStringList args;

  progress->show();
  outputType="Audio recovery";

  QItemSelectionModel *selectionModel = fileTreeView->selectionModel();
  QModelIndexList  indexList=selectionModel->selectedIndexes();

  if (indexList.isEmpty()) return;

  updateIndexInfo();

  uint size=indexList.size();

  if (size > 1) { QMessageBox::warning(this, "Error", tr("Enter just one directory")); return;}

  QModelIndex index;
  index=indexList.at(0);
  if (!index.isValid()) return;

  if (model->fileInfo(index).isFile())
    { QMessageBox::warning(this, "Error", tr("Enter a directory path")); return;}

  else if  (model->fileInfo(index).isDir())
    {
      sourceDir=model->fileInfo(index).absoluteFilePath();
      dvda::totalSize[AUDIO]=(sourceDir.isEmpty())? 0 : getDirectorySize(sourceDir, "*.AOB");
      if (dvda::totalSize[AUDIO] < 100)
        {
          QMessageBox::warning(this, tr("Extract"), tr("Directory path is empty. Please select disc structure."), QMessageBox::Ok | QMessageBox::Cancel);
          return;
        }
    }
  else
    {
      QMessageBox::warning(this, tr("Browse"),
                           tr("%1 is not a file or a directory.").arg(model->fileInfo(index).fileName()));
      return;
    }

  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      if (item->commandLineType == dvdaExtract)
        args << item->commandLineStringList();

    }

  if (dvda::totalSize[AUDIO] == 0)
    {
   process.kill();
      //      processFinished(EXIT_FAILURE);
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing DVD-Audio structure %1").arg(sourceDir));

  outputTextEdit->append(tr(MSG_HTML_TAG "Size of audio content %1").arg(QString::number(dvda::totalSize[AUDIO])));

  QString command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));


  //FAbstractWidget::setProtectedFields(runMkisofs="0";

  process.start(/*"konsole"*/ "dvda-author", args);
}
