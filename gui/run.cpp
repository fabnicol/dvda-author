#include "dvda.h"



void dvda::showEvent(QShowEvent *)
{
  myTimerId=startTimer(800);
}

void dvda::hideEvent(QHideEvent *)
{
  killTimer(myTimerId);
}


void dvda::timerEvent(QTimerEvent *event)
{
  qint64 new_value=0;
  qint64 new_isoSize;
  unsigned short int counter;
  static unsigned short int static_value;

  if (event->timerId() == myTimerId)
    {
      if (startProgressBar)
        {
          new_value=recursiveDirectorySize(Hash::wrapper["targetDir"]->toQString(), "*.AOB");
          progress->setValue(qFloor(discShare(new_value)));
          value=new_value;
        }
      else

        if (startProgressBar2)
          {
            new_isoSize=QFileInfo(Hash::wrapper["mkisofsPath"]->toQString()).size();
            outputTextEdit->append(tr(MSG_HTML_TAG "Size of iso output: %1").arg(QString::number(new_isoSize)));
            counter=qFloor(((float) new_isoSize*102)/ ((float) value));
            progress2->setValue(counter);
          }
        else

          if (startProgressBar3)
            {
              static_value += 3;
              progress3->setValue(static_value);

            }
          else static_value=0;
    }

  else
    QWidget::timerEvent(event);
}

float dvda::discShare(qint64 directorySize)
{
  qint64 tot=dvda::totalSize[AUDIO]+dvda::totalSize[VIDEO];
  if (tot > 1024*1024*1024*4.7) outputTextEdit->append(tr(ERROR_HTML_TAG "total size exceeds 4.7 GB\n"));
  float share=100* ((float) directorySize ) /((float) tot);
  return share;
}

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

  progress->reset();
  if (progress3)
    {
      if ((*FString("burnDisc")).isTrue())
        {
          progressLayout->removeWidget(progress3);
          delete(progress3);
          progressLayout->removeWidget(killCdrecordButton);
          delete(killCdrecordButton);
          progress3=NULL;

        }
      else
        if (progress3->isEnabled()) progress3->reset();
    }

  if (progress2)
    {
      if ((*FString("runMkisofs")).isTrue())
        {
          progressLayout->removeWidget(progress2);
          delete(progress2);
          progressLayout->removeWidget(killMkisofsButton);
          delete(killMkisofsButton);
          progress2=NULL;

        }
      else
        progress2->reset();
    }


  if (dvda::totalSize[AUDIO] + dvda::totalSize[VIDEO] == 0)
    {
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  args << "-P0" << "-o" << common::tempdir+"/output"  << "-g" << "/home/fab/celice.wav" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          << "/home/fab/celice.wav" << "-j" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          <<          "-g" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
                    << "/home/fab/celice.wav" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
                              << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav";


  args << createCommandLineString(dvdaCommandLine|createIso|createDisc);

  //args << createCommandLineString(lplexFiles).split("-ts");

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing input directory..."));
  outputTextEdit->append(tr(MSG_HTML_TAG "Size of Audio zone input %1").arg(QString::number(dvda::totalSize[AUDIO])));
  outputTextEdit->append(tr(MSG_HTML_TAG "Size of Video zone input %1").arg(QString::number(dvda::totalSize[VIDEO])));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : dvda-author %1").arg(command));

  startProgressBar=1;
  outputType="DVD-Audio authoring";
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start(/*"konsole"*/ "dvda-author", args);

  // runLplex();
  outputTextEdit->moveCursor(QTextCursor::End);

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

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing input directory..."));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  startProgressBar=1;
  outputType="audio DVD-Video disc authoring";
  process.start(/*"konsole"*/ "Lplex", args);

}

void dvda::processFinished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  QStringList  argsMkisofs;
  startProgressBar=0;
  startProgressBar3=0;

  if ((exitStatus == QProcess::CrashExit) ||  (exitCode == EXIT_FAILURE))
    {
       outputTextEdit->append(ERROR_HTML_TAG  +outputType + tr(": dvda-author crashed"));
       return;
     }
    else
     {
        outputTextEdit->append(MSG_HTML_TAG "\n" + outputType + tr(" completed, output directory is %1").arg(v(targetDir)));
        qint64 fsSize=recursiveDirectorySize(v(targetDir), "*.*");
        outputTextEdit->append(tr(MSG_HTML_TAG "File system size: ")+ QString::number(fsSize) + " Bytes ("+ QString::number(((float)fsSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");
        progress->setValue(maxRange);

        if ((*FString("runMkisofs")).isTrue())
          {
            if ((*FString("targetDir")).isFilled() & (v(mkisofsPath)).isFilled())

              argsMkisofs << "-dvd-audio" << "-o" << v(mkisofsPath) << v(targetDir);

            if (progress2 == NULL)
              {
                killMkisofsButton = new QToolButton(this);
                killMkisofsButton->setToolTip(tr("Kill mkisofs"));
                const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
                killMkisofsButton->setIcon(iconKill);
                killMkisofsButton->setIconSize(QSize(22,22));

                connect(killMkisofsButton, SIGNAL(clicked()), this, SLOT(killMkisofs()));

                progress2 = new QProgressBar(this);
                progress2->setRange(0, maxRange=100);
                progress2->setToolTip(tr("ISO file creation progress bar"));

                QHBoxLayout *progress2Layout= new QHBoxLayout;
                progress2Layout->addWidget(killMkisofsButton);
                progress2Layout->addWidget(progress2);
                progressLayout->addLayout(progress2Layout);
              }
            progress2->reset();
            startProgressBar2=1;
            outputTextEdit->append(tr(MSG_HTML_TAG "mkisofs command line : %1").arg(argsMkisofs.join(" ")));
            process2.start("/usr/bin/mkisofs", argsMkisofs);
          }
   }
}


void dvda::process2Finished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  startProgressBar2=0;

  QFileInfo info=QFileInfo(v(mkisofsPath));

  if ((exitStatus == QProcess::CrashExit) || (exitCode == EXIT_FAILURE))
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG " mkisofs crashed"));
    }
   else
    {
      if (!info.isFile() ||  info.size() == 0)
      {
          outputTextEdit->append(tr(MSG_HTML_TAG "\nISO file could not be created by mksiofs"));
          progress2->reset();
      }
        else
      {
          outputTextEdit->append(tr(MSG_HTML_TAG "\nISO file %1 created").arg(v(mkisofsPath)));

        progress2->setValue(maxRange);
        outputTextEdit->append(tr(MSG_HTML_TAG " You can now burn your DVD-Audio disc"));
      }
     }
}

void dvda::process3Finished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  startProgressBar3=0;

  if (exitStatus == QProcess::CrashExit)
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG "cdrecord crashed"));
    } else

    if (exitCode == EXIT_FAILURE)
      {
        outputTextEdit->append(tr(ERROR_HTML_TAG "DVD-Audio disc was not burned"));
      } else
      {
        progress3->setValue(maxRange);
      }
}



void dvda::on_cdrecordButton_clicked()
{

  if (((*FString("burnDisc")).isFalse())||((*FString("dvdwriterPath")).isEmpty())) return;

  QStringList argsCdrecord;


  if ((*FString("runMkisofs")).isFalse())
    {
      QMessageBox::warning(this, tr("Record"), tr("You need to create an ISO file first to be able to burn a DVD-Audio disc."), QMessageBox::Ok );
      return;
    }


  if ((*FString("dvdwriterPath")).isEmpty())
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
  argsCdrecord << "dev="<< v(dvdwriterPath) << v(mkisofsPath);
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line: cdrecord %1").arg(argsCdrecord.join(" ")));

  if (progress3 == NULL)
    {
      progress3 = new QProgressBar(this);
      killCdrecordButton = new QToolButton(this);
      killCdrecordButton->setToolTip(tr("Kill cdrecord"));
      const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
      killCdrecordButton->setIcon(iconKill);
      killCdrecordButton->setIconSize(QSize(22,22));

      connect(killCdrecordButton, SIGNAL(clicked()), this, SLOT(killCdrecord()));

      QHBoxLayout *progress3Layout= new QHBoxLayout;
      progress3Layout->addWidget(killCdrecordButton);
      progress3Layout->addWidget(progress3);
      progressLayout->addLayout(progress3Layout);
    }

  progress3->setRange(0, maxRange=100);
  progress3->setToolTip(tr("Burning DVD-Audio disc with cdrecord"));
  progress3->reset();

  startProgressBar3=1;
  process3.start("cdrecord", argsCdrecord);

}


void dvda::killDvda()
{
  if (!process.atEnd()) return;
  process.kill();
  outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
  progress->reset();
  processFinished(EXIT_FAILURE, QProcess::NormalExit );
}

void dvda::killMkisofs()
{
  if (!process2.atEnd()) return;
  process2.kill();
  progress2->reset();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG " mkisofs processing was killed (SIGKILL)"));
  process2Finished(EXIT_FAILURE, QProcess::NormalExit);
}

void dvda::killCdrecord()
{
  if (process3.atEnd()) return;
  process3.kill();
  progress3->reset();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG " cdrecord processing was killed (SIGKILL)"));
  process3Finished(EXIT_FAILURE, QProcess::NormalExit);
}


void dvda::extract()
{
  QStringList args;

  progress->reset();
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
      dvda::totalSize[AUDIO]=(sourceDir.isEmpty())? 0 : recursiveDirectorySize(sourceDir, "*.AOB");
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
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing DVD-Audio structure %1").arg(sourceDir));

  outputTextEdit->append(tr(MSG_HTML_TAG "Size of audio content %1").arg(QString::number(dvda::totalSize[AUDIO])));

  QString command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  startProgressBar3=1;
  //FAbstractWidget::setProtectedFields(runMkisofs="0";

  process.start(/*"konsole"*/ "dvda-author", args);
}
