#include <QtGui>
#include <QFile>
#include <sys/stat.h>
#include <errno.h>
#include <QModelIndex>
#include <QtXml>
#include <QSettings>

#include "dvda-author-gui.h"
#include "options.h"
#include "browser.h"


RefreshManagerFilter dvda::RefreshFlag=NoCreate;


class hash;


void dvda::initialize()
{
  adjustSize();

  myMusic=0;
  rank[0]=rank[1]=0;
  maxRange=0;

  startProgressBar=startProgressBar2=startProgressBar3=0;
  inputSizeCount=inputTotalSize=value=0;
  memset(inputSize,0,2*99*sizeof(quint64));

  myTimerId=isVideo=0;
  tempdir=QDir::homePath ()+QDir::separator()+"tempdir";  // should be equal to main app globals.settings.tempdir=TEMPDIR

  extraAudioFilters=QStringList() << "*.wav" << "*.flac";

  hash::description["titleset"]="DVD-Video titleset";
  hash::description["group"]="DVD-Audio group";
  hash::description["recent"]="Recent file";

}


void dvda::on_playItem_changed()
{
  if (!myMusic ) return;

  myMusic->setMedia(QUrl::fromLocalFile(hash::fstringlist.value((isVideo)? "DVD-A" : "DVD-V")->at(currentIndex).at(row)));
  myMusic->play();
}


void dvda::on_playItemButton_clicked()
{
  static int count;

  updateIndexInfo();
  if (row < 0)
    {
      row=0;
      project[isVideo]->fileListWidget->setCurrentRow(0);
    }
  updateIndexChangeInfo();

  if (count == 0)
    {
      myMusic = new QMediaPlayer;
    }

  if (count % 2 == 0)
    {
      myMusic->play();
      outputTextEdit->append(tr(INFORMATION_HTML_TAG "Playing...\n   file %1\n   in %2 %3   row %4" "<br>").arg(hash::fstringlist.value(tag)->at(currentIndex).at(row),groupType,QString::number(currentIndex+1),QString::number(row+1)));
      playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
      playItemButton->setToolTip(tr("Stop playing"));
    }
  else
    {
      playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
      playItemButton->setToolTip(tr("Play selected file"));
      myMusic->stop();
      outputTextEdit->append(tr(INFORMATION_HTML_TAG "Stopped.\n"));
    }
  count++;
}


dvda::dvda()
{
  setAttribute(Qt::WA_DeleteOnClose);
  initialize();

  model = new QFileSystemModel;
  model->setReadOnly(false);
  model->setRootPath(QDir::homePath());
  model->sort(Qt::AscendingOrder);
  model->setNameFilterDisables(false);

  fileTreeView = new QTreeView;
  fileTreeView->setModel(model);
  fileTreeView->hideColumn(1);
  fileTreeView->setMinimumWidth(400);
  fileTreeView->setColumnWidth(0,300);

  fileTreeView->header()->setStretchLastSection(true);
  fileTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);
  fileTreeView->header()->setSortIndicatorShown(true);
//  fileTreeView->header()->setClickable(true);
  fileTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  fileTreeView->setSelectionBehavior(QAbstractItemView::SelectItems);

  QModelIndex index = model->index(QDir::currentPath());
  fileTreeView->expand(index);
  fileTreeView->scrollTo(index);

  audioFilterButton= new QToolButton(this);
  audioFilterButton->setToolTip("Show audio files with extension "+ common::extraAudioFilters.join(", ")+"\nTo add extra file formats to this filter button go to Options>Audio Processing,\ncheck the \"Enable multiformat input\" box and fill in the file format field.");
  const QIcon iconAudioFilter = QIcon(QString::fromUtf8( ":/images/audio_file_icon.png"));
  audioFilterButton->setIcon(iconAudioFilter);
  audioFilterButton->setIconSize(QSize(22, 22));
  audioFilterButton->setCheckable(true);

//  tabWidget[AUDIO]->setToolTip(tr("List files in each audio group tab"));

  QIcon* iconDVDA = new QIcon(":/images/64x64/dvd-audio.png");
  QIcon* iconDVDV = new QIcon(":/images/64x64/dvd-video.png");


  project[AUDIO]=new FListFrame(NULL,
                                fileTreeView,
                                importFiles,
                                "DVD-A",
                                "audio group",
                                "g",
                                dvdaCommandLine | hasListCommandLine|flags::enabled,
                               {" ", " -g "},
                               {"file", "group"},
                                0,
                                iconDVDA);


  mainTabWidget=project[AUDIO]->embeddingTabWidget;

  mainTabWidget->setIconSize(QSize(64, 64));
  mainTabWidget->setMovable(true);
  mainTabWidget->setMinimumWidth(250);
//  tabWidget[VIDEO]->setToolTip(tr("List files in each video titleset tab"));

  project[VIDEO]=new FListFrame(NULL,
                                fileTreeView,
                                importFiles,
                                "DVD-V",
                                "video titleset",
                                "",
                                lplexFiles | hasListCommandLine|flags::enabled,
                               {" ", " -ts "},
                               {"file", "titleset"},
                                1,
                                iconDVDV,
                                mainTabWidget);

  project[VIDEO]->fileListWidget->properties= new FStringList(QList<QStringList>() << QStringList({"size=\"0\""}) << QStringList({"rank=\"0\"", "count=\"0\""}));
  project[AUDIO]->fileListWidget->properties= new FStringList(QList<QStringList>() << QStringList({"size=\"0\""}) << QStringList({"rank=\"0\"", "count=\"0\""}));
  project[VIDEO]->embeddingTabWidget->setIconSize(QSize(64, 64));

  project[AUDIO]->model=model;
  project[VIDEO]->model=model;
  project[AUDIO]->slotList=NULL;
  project[VIDEO]->slotList=NULL;

  mkdirButton = new QToolButton(this);
  mkdirButton->setToolTip(tr("Create Directory..."));
  const QIcon iconCreate = QIcon(QString::fromUtf8( ":/images/folder-new.png"));
  mkdirButton->setIcon(iconCreate);
  mkdirButton->setIconSize(QSize(22, 22));

  removeButton = new QToolButton(this);
  removeButton->setToolTip(tr("Remove directory or file..."));
  const QIcon iconRemove = QIcon(QString::fromUtf8( ":/images/edit-delete.png"));
  removeButton->setIcon(iconRemove);
  removeButton->setIconSize(QSize(22, 22));

  playItemButton = new QToolButton(this);
  playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  playItemButton->setIconSize(QSize(22, 22));
  playItemButton->setToolTip(tr("Play selected file"));

  killButton = new QToolButton(this);
  killButton->setToolTip(tr("Kill dvda-author"));
  const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
  killButton->setIcon(iconKill);
  killButton->setIconSize(QSize(22,22));

  progress= new QProgressBar(this);
  progress->reset();
  progress->setRange(0, maxRange=100);
  progress->setToolTip(tr("DVD-Audio structure authoring progress bar"));

  consoleDialog= new QDialog(this, Qt::Window | Qt::WindowStaysOnTopHint);
  QVBoxLayout* consoleLayout=new QVBoxLayout;
  console= new QTextEdit;
  consoleLayout->addWidget(console);
  consoleDialog->setLayout(consoleLayout);
  consoleDialog->setWindowTitle("Console");
  consoleDialog->setMinimumSize(800,600);

  connect(mkdirButton, SIGNAL(clicked()), this, SLOT(createDirectory()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
  connect(project[AUDIO]->addGroupButton, SIGNAL(clicked()), this, SLOT(addGroup()));
  connect(project[VIDEO]->addGroupButton, SIGNAL(clicked()), this, SLOT(addGroup()));
  connect(project[AUDIO]->deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
  connect(project[VIDEO]->deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
  connect(killButton, SIGNAL(clicked()), this, SLOT(killDvda()));
  connect(&process, SIGNAL(readyReadStandardOutput ()), this, SLOT(feedConsole()));
  connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
  connect(&process2, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process2Finished(int, QProcess::ExitStatus)));
  connect(&process2, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_cdrecordButton_clicked()));
  connect(&process3, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process3Finished(int, QProcess::ExitStatus)));
  connect(project[AUDIO]->importFromMainTree, SIGNAL(clicked()), this, SLOT(on_rightButton_clicked()));
  connect(project[VIDEO]->importFromMainTree, SIGNAL(clicked()), this, SLOT(on_rightButton_clicked()));
  connect(project[AUDIO]->moveUpItemButton, SIGNAL(clicked()), this, SLOT(on_moveUpItemButton_clicked()));
  connect(project[VIDEO]->moveUpItemButton, SIGNAL(clicked()), this, SLOT(on_moveUpItemButton_clicked()));
  connect(project[AUDIO]->moveDownItemButton, SIGNAL(clicked()), this, SLOT(on_moveDownItemButton_clicked()));
  connect(project[VIDEO]->moveDownItemButton, SIGNAL(clicked()), this, SLOT(on_moveDownItemButton_clicked()));
  connect(project[AUDIO]->retrieveItemButton, SIGNAL(clicked()), this, SLOT(on_retrieveItemButton_clicked()));
  connect(project[VIDEO]->retrieveItemButton, SIGNAL(clicked()), this, SLOT(on_retrieveItemButton_clicked()));
  connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_frameTab_changed(int )));
  connect(playItemButton, SIGNAL(clicked()), this, SLOT(on_playItemButton_clicked()));
  connect(this, SIGNAL(hasIndexChangedSignal()), this, SLOT(on_playItem_changed()));
  connect(audioFilterButton, SIGNAL(toggled(bool)), this, SLOT(on_audioFilterButton_clicked(bool)));

  QGridLayout *projectLayout = new QGridLayout;

  QGridLayout *updownLayout = new QGridLayout;
  QVBoxLayout *mkdirLayout = new QVBoxLayout;
  QHBoxLayout *progress1Layout= new QHBoxLayout;

  progressLayout = new QVBoxLayout;

  mkdirLayout->addWidget(mkdirButton);
  mkdirLayout->addWidget(removeButton);
  mkdirLayout->addWidget(audioFilterButton);
  projectLayout->addLayout(mkdirLayout,0,0);

  projectLayout->addWidget(project[AUDIO]->importFromMainTree, 0,1);
  projectLayout->addWidget(project[VIDEO]->importFromMainTree, 0,1);
  // set visible goo importFromMaintree and controlButtonBox !

  projectLayout->addWidget(project[AUDIO]->tabBox, 0,2);
  projectLayout->addWidget(project[VIDEO]->tabBox, 0,2);
//    projectLayout->addWidget(mainTabWidget, 0,2);

  updownLayout->addWidget(project[AUDIO]->controlButtonBox, 0,0);
  updownLayout->addWidget(project[VIDEO]->controlButtonBox, 0,0);

  updownLayout->setRowMinimumHeight(1, 40);
  updownLayout->addWidget(playItemButton, 2, 0);
  updownLayout->setRowMinimumHeight(3, 40);

  projectLayout->addLayout(updownLayout, 0,3);

  mainLayout = new QVBoxLayout;
  mainLayout->addLayout(projectLayout);

  progressLayout = new QVBoxLayout;
  progress1Layout->addWidget(killButton);
  progress1Layout->addWidget(progress);
  progressLayout->addLayout(progress1Layout);

  mainLayout->addLayout(progressLayout);

  QHBoxLayout *allLayout =new QHBoxLayout;

  managerLayout =new QVBoxLayout;

  allLayout->addLayout(mainLayout);
  allLayout->addLayout(managerLayout);

  setLayout(allLayout);
  setWindowTitle(tr("dvda-author"));
  const QIcon dvdaIcon=QIcon(QString::fromUtf8( ":/images/dvda-author.png"));
  setWindowIcon(dvdaIcon);
  on_frameTab_changed(0);

  /* requested initialization */
  progress2=NULL;
  progress3=NULL;
}



void dvda::on_frameTab_changed(int index)
{
  project[AUDIO]->controlButtonBox->setVisible(index == AUDIO);
  project[VIDEO]->controlButtonBox->setVisible(index == VIDEO);
  project[AUDIO]->importFromMainTree->setVisible(index == AUDIO);
  project[VIDEO]->importFromMainTree->setVisible(index == VIDEO);
}


void dvda::on_displayConsoleButton_clicked()
{
  bool visibility=consoleDialog->isVisible();
  visibility = !visibility;
  consoleDialog->setVisible(visibility);
  if (visibility)
    {
      consoleDialog->raise();
      consoleDialog->activateWindow();
    }
}


void dvda::on_audioFilterButton_clicked(bool active)
{
  QStringList filters= QStringList();

  if (active)
    filters += common::extraAudioFilters;

  model->setNameFilters(filters);
  fileTreeView->update();
}


void dvda::on_clearOutputTextButton_clicked()
{
  outputTextEdit->clear();
}


uint dvda::addStringToListWidget(QString filepath)
{

  updateIndexInfo();

  project[isVideo]->fileListWidget->properties->value(0)[0]="count=" + QString("\"")+ QString::number(row)+ QString("\"");

  QString msg=QString(MSG_HTML_TAG "Added file %4 to " + groupType +" %1\n"+groupType+" size:  %2, total size: %3\n");

  QFileInfo fileInfo(filepath);
  quint64 size=(quint64) fileInfo.size();
  rowFileSize[isVideo][currentIndex].append(size);
  inputSize[isVideo][currentIndex] += size;
  inputSizeCount += size;

  outputTextEdit->append(msg.arg(QString::number(currentIndex+1), QString::number(inputSize[isVideo][currentIndex]), QString::number(inputSizeCount),fileInfo.fileName()));

  return 0;

}


void dvda::refreshRowPresentation()
{
  // indexes are supposed to have been recently updated
  refreshRowPresentation(isVideo, currentIndex);

}


void dvda::refreshRowPresentation(uint ZONE, uint j)
{
  QString localTag=(ZONE == AUDIO)? "DVD-A" : "DVD-V";
  Q(hash::fstringlist[localTag]->at(j).join(','))
  for (int r=0; r < hash::fstringlist[localTag]->at(j).size(); r++ )
    {
      //resetting text
      project[ZONE]->currentListWidget->item(r)->setText(hash::fstringlist.value(localTag)->at(j).at(r).section('/',-1));
      project[ZONE]->currentListWidget->item(r)->setTextColor(QColor((r % 2)?"white":"navy"));
      project[ZONE]->currentListWidget->item(r)->setToolTip(QString::number(rowFileSize[ZONE][j][r]/1024)+" KB");
    }
}

//TODO insert button somewhere or right-click option, and back to sort by name
void dvda::showFilenameOnly()
{
  QPalette palette;
  palette.setColor(QPalette::AlternateBase,QColor("silver"));
  QFont font=QFont("Courier",10);

  updateIndexInfo();
  project[isVideo]->currentListWidget=qobject_cast<QListWidget*>(project[isVideo]->mainTabWidget->currentWidget());
  project[isVideo]->currentListWidget->setPalette(palette);
  project[isVideo]->currentListWidget->setAlternatingRowColors(true);
  project[isVideo]->currentListWidget->setFont(font);
  refreshRowPresentation(isVideo, currentIndex);
  //          project[ZONE][j]->sortItems(); TODO: if sort then reset tool tip after sort.

}


void dvda::addDirectoryToListWidget(QDir dir)
{

  QStringList filters;
  filters+="*";

  foreach (QFileInfo file, dir.entryInfoList(filters,QDir::AllDirs | QDir::NoDotAndDotDot|QDir::Files))
    {
      if (file.isDir())
        {
          outputTextEdit->insertHtml(QString("<b style='color:red;'>Recursing into subfolder " +file.fileName()+"...</b><br>"));
          addDirectoryToListWidget(QDir(file.canonicalFilePath()));
        }
      else
        addStringToListWidget(file.canonicalFilePath());
    }
}

//void dvda::addDraggedFiles(QList<QUrl> urls)
//{
//  updateIndexInfo();
//  uint size=urls.size();

//  for (uint i = 0; i < size; i++)
//    {
//      if (false == addStringToListWidget((QString) urls.at(i).toLocalFile())) return;
//    }
//  saveProject();
//  showFilenameOnly();
//}



void dvda::on_openProjectButton_clicked()
{

  projectName=QFileDialog::getOpenFileName(this,  tr("Open project"), QDir::currentPath(),  tr("Projects (*.dvp)"));

  if (projectName.isEmpty()) return;

  recentProjectFile();
  managerWidget->show();

}

void dvda::openProjectFile()
{
  projectName=qobject_cast<QAction *>(sender())->data().toString();
  recentProjectFile();
}


void dvda::recentProjectFile()
{
  clearProjectData();

  if (!projectName.isEmpty())
    {
      setCurrentFile(projectName);
    }
}

void dvda::closeProject()
{
  projectName="";
  clearProjectData();
}

void dvda::clearProjectData()
{
  RefreshFlag = (RefreshFlag == NoCreate)? CreateTreeAndRefreshAll : RefreshAll ;
  memset(inputSize, 0, 2*99*sizeof(quint64));

  for (uint ZONE=AUDIO; ZONE <=VIDEO; ZONE++)
    {
      for (uint i=0; i <= rank[ZONE]; i++)
        {
          project[ZONE]->on_clearList_clicked();
          rowFileSize[ZONE][i].clear();
          hash::fstringlist[(ZONE == VIDEO)?"DVD-V":"DVD-A"]->removeAt(rank[ZONE]);
        }
      project[ZONE]->signalList->clear();
    }

  inputSizeCount=inputTotalSize=0;

  QMessageBox::StandardButton choice=QMessageBox::Cancel;

  if (options::RefreshFlag ==  NoCreate)
    {
      choice=QMessageBox::information(this, "New settings",
                                      "This project contains new option settings.\nPress OK to replace your option settings,\notherwise No to parse only file paths\nor Cancel to exit project parsing.\n",
                                      QMessageBox::Ok|QMessageBox::No|QMessageBox::Cancel);

      switch (choice)
        {
        case QMessageBox::Ok  :
          options::RefreshFlag = UpdateOptionTabs;
          emit(clearOptionData());
          break;


        case QMessageBox::No :
          options::RefreshFlag = NoCreate;
          break;

        case QMessageBox::Cancel :
        default:
          return;
          break;
        }
    }

refreshProjectManager();

project[AUDIO]->embeddingTabWidget->setCurrentIndex(0);
project[VIDEO]->embeddingTabWidget->setCurrentIndex(0);
}


void dvda::on_helpButton_clicked()
{
  QUrl url=QUrl::fromLocalFile(this->generateDatadirPath("GUI.html") );
   browser::showPage(url);
}



void dvda::on_openManagerWidgetButton_clicked()
{
  if (RefreshFlag == NoCreate)
    {
      RefreshFlag=Create;
      refreshProjectManager();

    }

  managerWidget->setVisible(managerWidget->isHidden());
}

void dvda::showEvent(QShowEvent *)
{
  myTimerId=startTimer(3000);
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
          new_value=recursiveDirectorySize(hash::qstring["targetDir"], "*.AOB");
          progress->setValue(qFloor(discShare(new_value)));
          value=new_value;
        }
      else

        if (startProgressBar2)
          {
            new_isoSize=QFileInfo(hash::qstring["mkisofsPath"]).size();
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

void dvda::addGroup()
{
  updateIndexInfo();

  addGroup(rank[isVideo]+1, isVideo);

}

void dvda::addGroup(int group_index, int isVideo)
{
  QString number;

  if (group_index < 9*isVideo*10+9)
    number=QString::number(group_index+1);
  else
    {
      QMessageBox::information(this, tr("Group"), tr(QString("A maximum of %1 "+ groupType + "s can be created.").toUtf8()).arg(QString::number(9*isVideo*10+9)));
      return;
    }

  rank[isVideo]++;
}

void dvda::deleteGroup()
{
  updateIndexInfo();

  inputSizeCount-=inputSize[isVideo][currentIndex];
  rowFileSize[isVideo][currentIndex].clear();

  if (rank[isVideo] > 0)
    {
      if (currentIndex < rank[isVideo])
        {

          for (unsigned j=currentIndex; j < rank[isVideo] ; j++)
            {
              rowFileSize[isVideo][j]=rowFileSize[isVideo][j+1];
              inputSize[isVideo][j]=inputSize[isVideo][j+1];
            }
        }
      rank[isVideo]--;
    }else
    {
      inputSize[isVideo][currentIndex]=0;
    }


  if (currentIndex) outputTextEdit->append(QString(MSG_HTML_TAG "Deleted "+groupType+" %1, total size: %2\n").arg(QString::number(currentIndex+1), QString::number(inputSizeCount)));

  saveProject();
}

static bool firstSelection=true;

void dvda::updateIndexChangeInfo()
{
  static uint oldVideo;
  static uint oldCurrentIndex;
  static int oldRow;
  hasIndexChanged=(isVideo != oldVideo) | (currentIndex != oldCurrentIndex) |  (row != oldRow);
  if (firstSelection) hasIndexChanged=false;

  emit(hasIndexChangedSignal());

  oldVideo=isVideo;
  oldCurrentIndex=currentIndex;
  oldRow=row;
  firstSelection=false;
}

void dvda::updateIndexInfo()
{
  isVideo=mainTabWidget->currentIndex();
  currentIndex=project[isVideo]->currentIndex;
  row=project[isVideo]->row;
  groupType=(isVideo)?"titleset":"group";
  tag=(isVideo)? "DVD-V" : "DVD-A";

  // row = -1 if nothing selected
}

void dvda::on_rightButton_clicked()
{
  addSelectedFileToProject();
}


void dvda::on_moveUpItemButton_clicked()
{
  updateIndexInfo();
  if (row == 0) return;
  rowFileSize[isVideo][currentIndex].swap(row, row-1);

  RefreshFlag=SaveAndUpdateTree;
  saveProject();
  refreshRowPresentation();
}

void dvda::on_moveDownItemButton_clicked()
{
  updateIndexInfo();

  if (row < 0) return;
  if (row == project[isVideo]->currentListWidget->count() -1) return;

  rowFileSize[isVideo][currentIndex].swap(row, row+1);

  RefreshFlag=SaveAndUpdateTree;
  saveProject();
  refreshRowPresentation();

}

void dvda::addSelectedFileToProject()
{
  QItemSelectionModel *selectionModel = fileTreeView->selectionModel();
  QModelIndexList  indexList=selectionModel->selectedIndexes();

  if (indexList.isEmpty()) return;

  updateIndexInfo();

  uint size=indexList.size();

  for (uint i = 0; i < size; i++)
    {
      QModelIndex index;
      index=indexList.at(i);

      if ((model->fileInfo(index).isFile())||(model->fileInfo(index).isDir()))
        {
          QString path=model->filePath(index);
          bool ok=(isVideo== 0)? checkAudioStandardCompliance(path) : checkVideoStandardCompliance(path);
          if (!ok)
          {
             outputTextEdit->append(tr(ERROR_HTML_TAG "Track does not comply with the standard.\n"));
             return;
          }

          addStringToListWidget(path);
        }
      else
        {
          QMessageBox::warning(this, tr("Browse"),
                               tr("%1 is not a file or a directory.").arg(model->fileInfo(index).fileName()));
          return;
        }
    }

  saveProject();
  showFilenameOnly();
 // Q("signal: " + project[AUDIO]->signalList->join(" "))

}


void dvda::on_retrieveItemButton_clicked()
{

  updateIndexInfo();

  if (row <0) return;

  quint64 size=rowFileSize[isVideo][currentIndex].takeAt(row);

  inputSize[isVideo][currentIndex]-=size;
  inputSizeCount-=size;

  outputTextEdit->append(QString(MSG_HTML_TAG "Retrieved file from " + groupType  + " %1\n"+ groupType+ " size: %2, total size: %3\n").arg(QString::number(currentIndex+1),
                                                                                                                                 QString::number(inputSize[isVideo][currentIndex]), QString::number(inputSizeCount)));
  RefreshFlag=SaveAndUpdateTree;
  saveProject();

}


void dvda::createDirectory()
{
  QModelIndex index = fileTreeView->currentIndex();
  if (!index.isValid())
    return;

  QString dirName = QInputDialog::getText(this, tr("Create Directory"), tr("Directory name"));

  if (!dirName.isEmpty())
    {
      if (!model->mkdir(index, dirName).isValid())
        QMessageBox::information(this, tr("Create Directory"),
                                 tr("Failed to create the directory"));
    }
}

void dvda::remove()
{

  bool ok;
  QModelIndex index  = fileTreeView->currentIndex();

  if (!index.isValid())    return;

  if (model->fileInfo(index).isDir())
    ok = removeDirectory(model->filePath(index)) ;
  else
    ok = model->remove(index);
  //update?

  if (!ok)
    QMessageBox::information(this, tr("Remove"),
                             tr("Failed to remove %1").arg(model->fileName(index)));

}


float dvda::discShare(qint64 directorySize)
{
  if (inputTotalSize > 1024*1024*1024*4.7) outputTextEdit->append(tr(ERROR_HTML_TAG "total size exceeds 4.7 GB\n"));
  float share=100* ((float) directorySize ) /((float) inputTotalSize);
  return share;
}

QStringList dvda::createCommandLineString(int commandLineType)
{
 QListIterator<FAbstractWidget*> w(FAbstractWidget::abstractWidgetList);
 QStringList commandLine;

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();
      int itemCommandLineType=item->commandLineType & flags::commandLineMask;
      if ((itemCommandLineType & commandLineType) == itemCommandLineType)
        {
           QStringList cli=item->commandLineStringList();
           commandLine +=  cli;
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


  inputTotalSize=inputSizeCount;

  if (inputTotalSize == 0)
    {
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  args << "-P0";

  args << createCommandLineString(dvdaCommandLine|createIso|createDisc);

//  args << createCommandLineString(lplexFiles).split("-ts");


  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing input directory..."));
  outputTextEdit->append(tr(MSG_HTML_TAG "Size of input %1").arg(QString::number(inputTotalSize)));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : dvda-author %1").arg(command));

  startProgressBar=1;
  outputType="DVD-Audio authoring";
  process.start(/*"konsole"*/ "dvda-author", args);

  // runLplex();
  outputTextEdit->moveCursor(QTextCursor::End);

}



void dvda::feedConsole()
{
    QByteArray data = process.readAllStandardOutput();


  QRegExp reg("\\[INF\\]([^\\n]*)\n");
  QRegExp reg2("\\[PAR\\]([^\\n]*)\n");
  QRegExp reg3("\\[MSG\\]([^\\n]*)\n");
  QRegExp reg4("\\[ERR\\]([^\\n]*)\n");
  QRegExp reg5("\\[WAR\\]([^\\n]*)\n");
  QRegExp reg6("(===.*licenses/.)");

        QString text=QString(data).replace(reg6, (QString) NAVY_HTML_TAG "\\1</span><br>");
        text= text.replace(reg, (QString) INFORMATION_HTML_TAG "\\1<br>");
        text=text.replace(reg2, (QString) PARAMETER_HTML_TAG "\\1<br>");
        text=text.replace(reg3, (QString) MSG_HTML_TAG "\\1<br>");
        text=text.replace(reg4, (QString) ERROR_HTML_TAG "\\1<br>");
        text=text.replace(reg5, (QString) WARNING_HTML_TAG "\\1<br>");


        console->append(text.replace('\n',"<br>"));

 }


void dvda::runLplex()
{
  QStringList args;
  QString command;


  QListIterator<FAbstractWidget*> w(FAbstractWidget::abstractWidgetList);

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

  if (exitStatus == QProcess::CrashExit)
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG "dvda-author crashed"));
      return;
    } else
    if (exitCode == EXIT_FAILURE)
      {
        outputTextEdit->append(ERROR_HTML_TAG "" +outputType + tr(" failed"));
        return;
      } else
      {
        outputTextEdit->append(MSG_HTML_TAG "\n" + outputType + tr(" completed, output directory is %1").arg(hash::qstring["targetDir"]));
        outputTextEdit->append(QString::number(recursiveDirectorySize(hash::qstring["targetDir"], "*.*")));
        progress->setValue(maxRange);

        if ((*FString("runMkisofs")).isTrue())
          {
            if ((*FString("targetDir")).isFilled() & (*FString("mkisofsPath")).isFilled())

              argsMkisofs << "-dvd-audio" << "-o" << hash::qstring["mkisofsPath"] << hash::qstring["targetDir"];

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
            process2.start("mkisofs", argsMkisofs);
          }
      }
}

void dvda::process2Finished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  startProgressBar2=0;

  if (exitStatus == QProcess::CrashExit)
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG "mkisofs crashed"));
    } else
    if (exitCode == EXIT_FAILURE)
      {
        outputTextEdit->append(tr(ERROR_HTML_TAG "ISO disc authoring failed"));
      } else
      {
        outputTextEdit->append(tr(MSG_HTML_TAG "\nISO file %1 created").arg(hash::qstring["mkisofsPath"]));
        progress2->setValue(maxRange);
        outputTextEdit->append(tr(MSG_HTML_TAG "You can now burn your DVD-Audio disc"));
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

  QFileInfo f(*FString("mkisofsPath"));
  f.refresh();

  if (! f.isFile())
    {
      QMessageBox::warning(this, tr("Record"), tr("No valid ISO file path was entered:\n %1").arg(hash::qstring["mkisofsPath"]), QMessageBox::Ok );
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "\nBurning disc...please wait."));
  argsCdrecord << "dev="<< hash::qstring["dvdwriterPath"] << hash::qstring["mkisofsPath"];
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
  process.kill();
  outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
  processFinished(EXIT_FAILURE, QProcess::NormalExit );
}

void dvda::killMkisofs()
{
  process2.kill();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "mkisofs processing was killed (SIGKILL)"));
  process2Finished(EXIT_FAILURE, QProcess::NormalExit);
}

void dvda::killCdrecord()
{
  process3.kill();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG "cdrecord processing was killed (SIGKILL)"));
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
      inputTotalSize=(sourceDir.isEmpty())? 0 : recursiveDirectorySize(sourceDir, "*.AOB");
      if (inputTotalSize < 100)
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

  QListIterator<FAbstractWidget*> w(FAbstractWidget::abstractWidgetList);

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      if (item->commandLineType == dvdaExtract)
        args << item->commandLineStringList();

    }

  if (inputTotalSize == 0)
    {
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing DVD-Audio structure %1").arg(sourceDir));

  outputTextEdit->append(tr(MSG_HTML_TAG "Size of audio content %1").arg(QString::number(inputTotalSize)));

  QString command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  startProgressBar3=1;
  //FAbstractWidget::setProtectedFields(runMkisofs="0";

  process.start(/*"konsole"*/ "dvda-author", args);
}

void dvda::requestSaveProject()
{
  projectName=QFileDialog::getSaveFileName(this,  tr("Set project file name"), "default.dvp", tr("dvp projects (*.dvp)"));
  saveProject();

}



void dvda::saveProject()
{
  QListIterator<FAbstractWidget*>  w(FAbstractWidget::abstractWidgetList);
  if ((projectName == NULL)||(projectName.isEmpty()))
    {
      projectName=QDir::currentPath()+"/"+ "default.dvp";
    }

  // On adding files or deleting files, or saving project, write project file and the update tree par reparsing project
  // Yet do not reparse tabs, as it should be useless (Tabs have been refreshed already)

  RefreshFlag=(RefreshFlag == NoCreate)?CreateSaveAndUpdateTree:SaveAndUpdateTree;

  // managerWidget == NULL test would not work

  audioFilterButton->setToolTip("Show audio files with extension "+ common::extraAudioFilters.join(", ")+"\nTo add extra file formats to this filter button go to Options>Audio Processing,\ncheck the \"Enable multiformat input\" box and fill in the file format field.");

  writeProjectFile();
  refreshProjectManager();
}

QString  dvda::makeString()
{
  QStringList L=QStringList();
  QListIterator<FAbstractWidget*> w(FAbstractWidget::abstractWidgetList);
  while (w.hasNext())
    {
      FAbstractWidget* widget=w.next();

      if  (widget->getHashKey().isEmpty())
        {
          QMessageBox::warning(this, tr("Error"), tr(".dvp project parsing error"));
          continue;
        }

      QString xml=widget->setXmlFromWidget().toQString();

      L << "  <switch hashKey=\""<< widget->getHashKey() << "\">\n" << "    <value>"
         << xml << "</value>\n  </switch>\n";
    }
  return L.join("");
}

void dvda::writeProjectFile()
{

  QFile projectFile;
  projectFile.setFileName(projectName);
  QErrorMessage *errorMessageDialog = new QErrorMessage(this);
  if (!projectFile.open(QIODevice::WriteOnly))
    {
      errorMessageDialog->showMessage(tr("Cannot open file for writing\n")+ qPrintable(projectFile.errorString()));
      QLabel *errorLabel = new QLabel;
      errorLabel->setText(tr("If the box is unchecked, the message "
                             "won't appear again."));
      return;
    }

  QTextStream out(&projectFile);

  out.setCodec("UTF-8");
  out << "<?xml version=\"1.0\"?>\n" <<"<project>\n";
  out << " <system>\n";

  out << dvda::makeString();

  out << " </system>\n";
  out << " <data>\n";

  for (int i=0; (i < parent->recentFiles.count()) && (i < MaxRecentFiles);  i++)
    {
      out << "  <switch hashKey=\"recent\"" <<   " rank=\"" << QString::number(i) << "\">\n"
          <<  "    <file>" << QDir::toNativeSeparators(parent->recentFiles.at(i)) << "</file>\n  </switch>\n";
    }

  out << " </data>\n";
  out << "</project>\n";
  out.flush();
}

void dvda::setCurrentFile(const QString &fileName)
{
  curFile =fileName;
  setWindowModified(false);

  QString shownName = "Untitled";

  if (!curFile.isEmpty())
    {
      shownName =parent->strippedName(curFile);
      parent->recentFiles.prepend(curFile);
      parent->updateRecentFileActions();
    }

  parent->settings->setValue("default", QVariant(curFile));
}


void dvda::assignVariables(FStringList &value)
{
  static QListIterator<FAbstractWidget*> w(FAbstractWidget::abstractWidgetList);

  if (!w.hasNext())
    {
       w.toFront();
    }

    if (w.hasNext())
      w.next()->setWidgetFromXml(value);

}

void dvda::assignGroupFiles(int isVideo, uint group_index, qint64 size, QString file)
{

  if (group_index > rank[isVideo])
    {
      addGroup(group_index, isVideo);
      outputTextEdit->append(MSG_HTML_TAG "Adding group " + QString::number(group_index));
    }
  QString group_type=(isVideo)?"titleset":"group";

  project[isVideo]->addStringToListWidget(file.section('/',-1), currentIndex);
  if (!isVideo) *(project[isVideo]->signalList) << file;
  rowFileSize[isVideo][group_index].append(size);
  inputSize[isVideo][group_index]+=size;
  inputSizeCount+=size;
  outputTextEdit->append(QString(MSG_HTML_TAG "Added file %4 to "+group_type+" %1:\n"+group_type+" size %2, total size %3\n").arg(QString::number(group_index+1),
                                                                                                                 QString::number(inputSize[isVideo][group_index]), QString::number(inputSizeCount), file));

}

bool dvda::refreshProjectManager()
{
  // Step 1: prior to parsing

  if (RefreshFlag&Create)
    {
      QStringList labels;
      labels << tr("Setting") << tr("Value/Path") << tr("Size");
      managerWidget=new QTreeWidget;

      managerLayout->addWidget(managerWidget);
      managerWidget->hide();
      managerWidget->setHeaderLabels(labels);
    }

  if (RefreshFlag&UpdateTree)
    {
      managerWidget->clear();
    }

  if (projectName.isEmpty())
    {
      managerWidget->setVisible(false);
      emit(is_signalList_changed(project[AUDIO]->signalList->size()));
      return false;
    }

  QFile file(projectName);

  if (RefreshFlag&SaveTree)
    {

      if (!file.isOpen())
        file.open(QIODevice::ReadWrite);
      else
        file.seek(0);
    }

  // Step 2: parsing on opening .dvp project  (=update tree +refresh tabs) or adding/deleting tab files (=update tree)

  if (RefreshFlag&UpdateTree)
    {
      if (!file.isOpen())
        file.open(QIODevice::ReadWrite);
      else
        file.seek(0);

      if (file.size() == 0)
        {
          outputTextEdit->append(WARNING_HTML_TAG "file is empty!");
          return false;
        }
      QPalette palette;
      palette.setColor(QPalette::AlternateBase,QColor("silver"));
      managerWidget->setPalette(palette);
      managerWidget->setAlternatingRowColors(true);

      DomParser(&file);

      // Step3: adjusting project manager size
      managerWidget->resizeColumnToContents(0);
      managerWidget->resizeColumnToContents(1);
      managerWidget->resizeColumnToContents(2);
    }

  if (file.isOpen()) file.close();


  return true;

  //managerWidget->adjustSize();
}


void dvda::DomParser(QIODevice* file)
{
  // Beware: to be able to interactively modify managerWidget in the DomParser child class constructor,
  // pass it as a parameter to the constructor otherwise the protected parent member will be accessible yet unaltered
  file->seek(0);

  QString errorStr;
  int errorLine;
  int errorColumn;

  QTreeWidgetItem *item=new QTreeWidgetItem(managerWidget);

  QDomDocument doc;
  if (!doc.setContent(file, true, &errorStr, &errorLine, &errorColumn))
    {
      QMessageBox::warning(0, tr("DOM Parser"), tr("Parse error at line %1, " "column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
      return;
    }

  QDomElement root=doc.documentElement();
  if (root.tagName() != "project") return;

  QDomNode node=root.firstChild();
  if (node.toElement().tagName() != "system") return;
  item->setText(0,"system");
  item->setExpanded(true);

  QDomNode subnode=node.firstChild();

  while (!subnode.isNull())
    {
      if (subnode.toElement().tagName() == "switch")
        parseEntry(subnode.toElement(), item);
      subnode=subnode.nextSibling();
    }

  node=node.nextSibling();

  QTreeWidgetItem *item2=new QTreeWidgetItem(managerWidget);

  if (node.toElement().tagName() != "data") return;
  item2->setText(0,"data");
  item2->setExpanded(true);

  subnode=node.firstChild();

  while (!subnode.isNull())
    {
      if (subnode.toElement().tagName() == "switch")
        parseEntry(subnode.toElement(), item2);
      subnode=subnode.nextSibling();
    }

  // References are needed here otherwise variables to be equated to booleanList members will not be as l-values.
  // Equating booleanList0=booleanList will not do either as references cannot be l-values.

  if (dvda::RefreshFlag & UpdateTabs)
          showFilenameOnly();

     emit(is_signalList_changed(project[AUDIO]->signalList->size()));
}


void dvda::parseEntry(const QDomElement &element, QTreeWidgetItem *itemParent)
{
  QString labelVariable=element.attribute("hashKey");
  QString groupIndexString;
  QString firstColumnText;
  int group_index=0;
  QStringList embeddedTags={"menu" ,  "track" ,  "slide" , "YCrCb", "group", "titleset"};

  if ((labelVariable == "titleset") || (labelVariable =="group") || (labelVariable =="recent"))
    {
      groupIndexString = element.attribute("rank");
      group_index=groupIndexString.toInt();
      firstColumnText = hash::description[labelVariable] + " "+QString::number(group_index + 1);
    }
  else
    firstColumnText = hash::description[labelVariable];

  if (firstColumnText.isEmpty()) return;

  QTreeWidgetItem *item;

  if (itemParent)
    item = new QTreeWidgetItem(itemParent);
  else
    item = new QTreeWidgetItem(managerWidget);

  item->setText(0, firstColumnText);

  bool isRecent=(labelVariable == "recent");
  QString allFiles,allSizes;
  QDomNode node=element.firstChild();

  while (!node.isNull())
    {
      QString tagName=node.toElement().tagName();
      if (tagName.isEmpty()) break;
      QDomNode childNode =node.firstChild();
      FStringList textInfo=FStringList();

      if ((tagName == "value") || (tagName == "recent"))
        {
          QStringList textStringList= QStringList() ;

          while (!childNode.isNull())
            {


              if  (childNode.nodeType() == QDomNode::TextNode)
                {
                  QString textInfoString;
                  textInfoString=childNode.toText().data();
                  if (isRecent)
                    {
                      if (!textInfoString.isEmpty())
                        setCurrentFile(textInfoString);
                    }
                  item->setText(1, textInfoString);
                  textInfo << (QStringList() << textInfoString);
                }
              else
                {
                  QString header;
                  QString secondColumn;
                  QString thirdColumn;
                  qint64 allSizes=0;
                  int j=0;
                  int depth=0;

                  while (!(childNode.isNull()) && (embeddedTags.contains(childNode.toElement().tagName())))
                    {

                      depth=1;
                      header = (j ==0)? "":"\n";

                      secondColumn +=  header + childNode.toElement().tagName() +" "+ QString::number(++j);
                      thirdColumn     += header ;

                      QDomNode grandChildNode =childNode.firstChild();
                      int i=0;

                      while (!(grandChildNode.isNull()) )
                        {

                          if (grandChildNode.nodeType() == QDomNode::TextNode)
                            {
                              static int k;
                              QString text=grandChildNode.toText().data();
                              textStringList << text;
                              secondColumn +=  " "+ QString((k==0)?"Track":((k==1)?"Highlight":"Album/Group")) + "  "+  text  ;
                              k++;

                            }
                          else
                            {


                              QDomNode grandChildChildNode =grandChildNode.firstChild();
                              depth=2;
                              while ((!grandChildChildNode.isNull()) && (grandChildChildNode.nodeType() == QDomNode::TextNode))
                                {
                                  QString text=grandChildChildNode.toText().data();
                                  qint64 byteCount=QFileInfo(text).size();
                                  // force coertion into float or double using .0
                                  double s=byteCount/1048576.0;
                                  allSizes+=byteCount;
                                  QString size=QString::number(s , 'f', 1);
                                  textStringList << text;
                                  if ((dvda::RefreshFlag & UpdateTabs)&&(!isRecent))
                                    assignGroupFiles(isVideo, group_index, byteCount,QDir::toNativeSeparators(text));

                                  secondColumn +=  "\n "+grandChildNode.toElement().tagName() +QString::number(i+1) +": "+ text  ;
                                  thirdColumn    +=  "\n "+ size + " MB" ;

                                  grandChildChildNode=grandChildChildNode.nextSibling();
                                }

                              i++;

                              textStringList.clear();
                              if  (grandChildNode.toElement().tagName() == "file")
                                {
                                  item->setTextColor(1,QColor("navy"));
                                  item->setTextColor(2,QColor("grey"));
                                  item->setTextAlignment(2,Qt::AlignRight);
                                  item->setText(1, "\n"+QDir::toNativeSeparators(secondColumn));
                                  item->setText(2, tr("Total size: ")+QString::number(allSizes/1048576.0, 'f', 1) +"MB"+ "\n"+ thirdColumn);
                                }

                            }

                          grandChildNode=grandChildNode.nextSibling();
                       }

                      if (depth == 2)
                        {
                          textInfo << textStringList;
                          textStringList.clear();
                        }
                      childNode=childNode.nextSibling();
                    }
                  if (depth == 1)
                    {
                      textInfo << textStringList;
                      textStringList.clear();
                    }



               }

               if ((tagName == "value") &&  ((dvda::RefreshFlag&0xF000) == UpdateTabs))
                 {
                     assignVariables(textInfo);
                 }

              childNode = childNode.nextSibling();
            }
        }

      node=node.nextSibling();
    }
}

