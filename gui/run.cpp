#include "dvda.h"
#include "fwidgets.h"






QStringList dvda::createCommandLineString(flags::commandLineType commandLineType)
{
 QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);
 QStringList commandLine;

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      //if ((itemCommandLineType & commandLineType) == itemCommandLineType)

//      Q(item->optionLabel)
//              q(item->commandLineType)
//              q(itemCommandLineType)
//              q(commandLineType)

      //    item->setXmlFromWidget();

      if (item->commandLineType == commandLineType)
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

// args << "-P0"   << "-g" << "/home/fab/Audio/a.wav" << "/home/fab/Audio/pn.wav" << "--output="+  common::tempdir+"/output" ;

//|createIso|createDisc

  args << "--pause=0" << "--disable-lexer" << createCommandLineString(flags::commandLineType::dvdaCommandLine);

  //args << createCommandLineString(lplexFiles).split("-ts");

  outputTextEdit->append(tr(MSG_HTML_TAG "Processing input directory..."));
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Size of Audio zone input %1").arg(QString::number(dvda::totalSize[AUDIO])));
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Size of Video zone input %1").arg(QString::number(dvda::totalSize[VIDEO])));
  command=args.join("#");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : dvda-author %1").arg(command));

  outputType="DVD-Audio authoring";

  process.setProcessChannelMode(QProcess::MergedChannels);

  process.start(/*"konsole"*/ "dvda-author", args);

  progress->setTarget(Hash::wrapper["targetDir"]->toQString());
  progress2->setTarget(Hash::wrapper["mkisofsPath"]->toQString());

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

      if (item->commandLineType == flags::commandLineType::lplexFiles)
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

qint64 isoSize=1, fsSize=1;

if (outputType == "DVD-Audio authoring")
 {
    outputTextEdit->append(INFORMATION_HTML_TAG  + outputType + tr(" completed, output directory is %1").arg(v(targetDir)));

    fsSize=getDirectorySize(v(targetDir), "*.*");

    progress2->setReference(fsSize);

    outputTextEdit->append(tr(INFORMATION_HTML_TAG "File system size: ")+ QString::number(fsSize) + " Bytes ("+ QString::number(((float)fsSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");

    if (v(skipMkisofs).isFalse())
        runMkisofs();
}
else
 if (outputType == "Disc image authoring")
    {
        outputTextEdit->append(INFORMATION_HTML_TAG  + outputType + tr(" completed. Image file is: %1").arg(v(mkisofsPath)));

        isoSize=getFileSize(v(mkisofsPath));

        progress3->setReference(isoSize);

        outputTextEdit->append(tr(INFORMATION_HTML_TAG"Iso file size: ")+ QString::number(isoSize) + " Bytes ("+ QString::number(((float)isoSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");
    }
else
 if (outputType == "Burning")
 {
     resetCdRecordProcessedOutput();
     if ((process.exitStatus() == QProcess::NormalExit) && (process.exitCode() == 0))
        outputTextEdit->append(INFORMATION_HTML_TAG  + outputType + tr(" completed."));
     else
     {
        outputTextEdit->append(ERROR_HTML_TAG  + outputType + tr(" issues: check disc."));
        QString msg=(process.exitStatus() == QProcess::NormalExit)?(QString(ERROR_HTML_TAG) + "Cdrecord: Normal exit") :(QString(ERROR_HTML_TAG) + "Crash exit");
        outputTextEdit->append(msg+". Error code: "+QString::number(process.exitCode()));
     }
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



void dvda::runCdrecord()
{

  if ((v(burnDisc).isFalse())||(v(dvdwriterPath).isEmpty())) return;

  QStringList argsCdrecord;


  if (v(skipMkisofs).isTrue() && v(mkisofsPath).isEmpty())
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

  if (!v(dvdwriterScan).isEmpty()) argsCdrecord << "dev="+ v(dvdwriterScan) ;
  if (v(ejectDvd)=="yes") argsCdrecord  << "-eject";
  if (v(verboseCdrecord)=="yes") argsCdrecord  <<  "-v" ;
  if (v(overBurnDvd)=="yes")  argsCdrecord  << "-overburn";
  if (v(blankDvd)=="yes") argsCdrecord  << "blank=fast";
  argsCdrecord  <<         v(mkisofsPath);

 //outputTextEdit->append(tr(MSG_HTML_TAG "Command line: cdrecord %1").arg(argsCdrecord.join(" ")));
  outputType="Burning";

  process.start(cdRecordCommandStr, argsCdrecord)   ;

  outputTextEdit->append(tr(MSG_HTML_TAG) +cdRecordCommandStr+" "+argsCdrecord.join(" "));
  progress3->setReference(getFileSize(v(mkisofsPath)));
  progress3->show();
  progress3->start(500);
}


void dvda::killProcess()
{
  process.kill();
  outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
}



void dvda::printMsg(qint64 new_value, const QString &str)
{
    if (process.state() != QProcess::Running)        return;
    if (new_value < 1024*1024*1024*4.7)
                   outputTextEdit->append(tr(MSG_HTML_TAG) + str + QString::number(new_value) +" B ("+QString::number(new_value/(1024*1024))+ " MB)");
             else
                 outputTextEdit->append(tr(WARNING_HTML_TAG) + "Total size exceeds 4.7 GB");
}

void dvda::printDiscSize(qint64 new_value)
{
    if (new_value > 1024) printMsg(new_value, "Processing audio files...");
}


void dvda::printFileSize(qint64 new_value)
{
    if (new_value > 1024) printMsg(new_value, "Processing .iso disc image...");
}

void dvda::printBurnProcess(qint64 new_value)
{
    if (new_value > 1024) printMsg(new_value, "Burning disc...");
}

qint64 dvda::getCdrecordProcessedOutput(const QString& dummy1, const QString& dummy2)
{
  return static_cast<qint64>(parent->cdRecordProcessedOutput*1024*1024);
}

void dvda::resetCdRecordProcessedOutput() {parent->cdRecordProcessedOutput=0;}

void dvda::killCdrecord()
{
  if (progress3->updateProgressBar() > 0)
  {
      outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" cannot be killed by cdrecord (too late!)"));
      return;
  }

  process.kill() ; // may not be enough, you have to close all channels
  if  (process.state() == QProcess::Running)
     process.close();

  if  (process.exitStatus() == QProcess::CrashExit)
  {
      outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
     // to avoid generating standard messages
      ejectProcess.start(cdRecordCommandStr,  QStringList()<< "-eject");
  }
  else
        outputTextEdit->append(WARNING_HTML_TAG+  tr("Failed to kill cdrecord, plesase use your task manager to do so."));
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

      if (item->commandLineType == flags::commandLineType::dvdaExtract)
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


  process.start(/*"konsole"*/ "dvda-author", args);
}
