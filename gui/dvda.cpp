
#include <QFile>
#include <sys/stat.h>
#include <errno.h>
#include <QModelIndex>
#include <QtXml>
#include <QSettings>

#include "dvda.h"
#include "options.h"
#include "browser.h"
#include "fstring.h"


RefreshManagerFilter dvda::RefreshFlag=NoCreate;
int flags::lplexRank=0;

class hash;


void dvda::initialize()
{
  adjustSize();

  myMusic=0;
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

  myMusic->setMedia(QUrl::fromLocalFile(hash::FStringListHash.value((isVideo)? "DVD-A" : "DVD-V")->at(currentIndex).at(row)));
  myMusic->play();
}


void dvda::on_playItemButton_clicked()
{
  static int count;

  updateIndexInfo();
  if (row < 0)
    {
      row=0;
      project[isVideo]->fileListWidget->currentListWidget->setCurrentRow(0);
    }
  updateIndexChangeInfo();

  if (count == 0)
    {
      myMusic = new QMediaPlayer;
    }

  if (count % 2 == 0)
    {
      myMusic->play();
      outputTextEdit->append(tr(INFORMATION_HTML_TAG "Playing...\n   file %1\n   in %2 %3   row %4" "<br>").arg(hash::FStringListHash.value(tag)->at(currentIndex).at(row),groupType,QString::number(currentIndex+1),QString::number(row+1)));
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

  project[AUDIO]=new FListFrame(NULL,      // no parent widget
                                fileTreeView,                   // files may be imported from this tree view
                                importFiles,                     // FListFrame type
                                "DVD-A",                          // superordinate xml tag
                                "DVD-Audio",                   // project manager widget on-screen tag
                                "g",                                  // command line label
                                dvdaCommandLine|hasListCommandLine|flags::enabled,  // command line characteristic features
                               {" ", " -g "},                       // command line separators
                               {"file", "group"},                // subordinate xml tags
                                0,                                     // rank
                                iconDVDA);                      //tab icon


  mainTabWidget=project[AUDIO]->embeddingTabWidget;

  mainTabWidget->setIconSize(QSize(64, 64));
  mainTabWidget->setMovable(true);
  mainTabWidget->setMinimumWidth(250);
//  tabWidget[VIDEO]->setToolTip(tr("List files in each video titleset tab"));

  project[VIDEO]=new FListFrame(NULL,
                                fileTreeView,                   // files may be imported from this tree view
                                importFiles,                     // FListFrame type
                                "DVD-V",                          // superordinate xml tag
                                "DVD-Video",                   // project manager widget on-screen tag
                                "",                                   // command line label
                                lplexFiles | hasListCommandLine|flags::enabled,  // command line characteristic features
                               {" ", " -ts "},                     // command line separators
                               {"file", "titleset"},             // subordinate xml tags
                                1,                                    // rank
                                iconDVDV,                      // tab icon
                                mainTabWidget);             // parent tab under which this frame is inserted


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
  connect(project[AUDIO]->importFromMainTree, SIGNAL(clicked()), this, SLOT(on_importFromMainTree_clicked()));
  connect(project[VIDEO]->importFromMainTree, SIGNAL(clicked()), this, SLOT(on_importFromMainTree_clicked()));
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

  QString msg=QString(MSG_HTML_TAG "Added file %4 to " + groupType +" %1\n"+groupType+" size:  %2, total size: %3\n");

  QFileInfo fileInfo(filepath);
  quint64 size=(quint64) fileInfo.size();
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
  QPalette palette;
  palette.setColor(QPalette::AlternateBase,QColor("silver"));
  QFont font=QFont("Courier",10);

  QListWidget *widget=project[ZONE]->getWidgetContainer()[j];
  widget->setPalette(palette);
  widget->setAlternatingRowColors(true);
  widget->setFont(font);


  for (int r=0; r < hash::FStringListHash[localTag]->at(j).size(); r++ )
    {
      widget->item(r)->setText(hash::FStringListHash.value(localTag)->at(j).at(r).section('/',-1));
      widget->item(r)->setTextColor(QColor("navy"));
      widget->item(r)->setToolTip(fileSizeDataBase[ZONE].at(j).at(r)+" MB");
    }
}

//TODO insert button somewhere or right-click option, and back to sort by name
void dvda::showFilenameOnly()
{
  updateIndexInfo();
  refreshRowPresentation(isVideo, currentIndex);
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
  static bool must_close;

  if (must_close) closeProject();
  projectName=QFileDialog::getOpenFileName(this,  tr("Open project"), QDir::currentPath(),  tr("dvp projects (*.dvp)"));

  if (projectName.isEmpty()) return;

  initializeProject();
  must_close=true;

}

void dvda::openProjectFile()
{
  projectName=qobject_cast<QAction *>(sender())->data().toString();
  initializeProject();
}


void dvda::initializeProject(const bool cleardata)
{

    if (cleardata)
    {
        clearProjectData();
        refreshProjectManager();
    }

    if (!projectName.isEmpty())
    {
      setCurrentFile(projectName);
    }
}

void dvda::closeProject()
{
  projectName="";
  clearProjectData();

  refreshProjectManager();

  for (int ZONE : {AUDIO, VIDEO})
  {
    for  (int i = project[ZONE]->getRank()+1; i >=0;   i--)
    {
      project[ZONE]->mainTabWidget->removeTab(i);
    }

    project[ZONE]->addNewTab();
  }
}

void dvda::clearProjectData()
{
  RefreshFlag = (RefreshFlag == NoCreate)? CreateTreeAndRefreshAll : RefreshAll ;
  memset(inputSize, 0, 2*99*sizeof(quint64));

  for (int ZONE : {AUDIO, VIDEO})
    {
      for (int i=0; i <= project[ZONE]->getRank(); i++)
             project[ZONE]->on_clearList_clicked(i);

      project[ZONE]->signalList->clear();
      project[ZONE]->clearWidgetContainer();
      fileSizeDataBase[ZONE].clear();
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

  for (int ZONE : {AUDIO, VIDEO})
  {
      project[ZONE]->embeddingTabWidget->setCurrentIndex(0);
      project[ZONE]->initializeWidgetContainer();
  }

    /* cleanly wipe out main hash */
        hash::initializeFStringListHashes();

}

void dvda::on_helpButton_clicked()
{
  QUrl url=QUrl::fromLocalFile(this->generateDatadirPath("GUI.html") );
   browser::showPage(url);
}



void dvda::on_openManagerWidgetButton_clicked(bool isHidden)
{

  if (RefreshFlag == NoCreate)
    {
      RefreshFlag=Create;
      refreshProjectManager();

    }
  managerWidget->setVisible(isHidden);
 }

void dvda::on_openManagerWidgetButton_clicked()
{
    on_openManagerWidgetButton_clicked(managerWidget->isHidden());
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

  if (project[isVideo]->getRank() >=  9*(int) isVideo*10+9)
   {
      QMessageBox::information(this, tr("Group"), tr(QString("A maximum of %1 "+ groupType + "s can be created.").toUtf8()).arg(QString::number(9*isVideo*10+9)));
      return;
    }

}

void dvda::deleteGroup()
{
  updateIndexInfo();
  uint rank=(uint) project[isVideo]->getRank();
  inputSizeCount-=inputSize[isVideo][currentIndex];
  if ((uint) fileSizeDataBase[isVideo].size() > currentIndex)
      fileSizeDataBase[isVideo][currentIndex].clear();

  if (rank > 0)
    {
      if (currentIndex < rank)
        {

          for (unsigned j=currentIndex; j < rank ; j++)
            {
              fileSizeDataBase[isVideo][j]=fileSizeDataBase[isVideo][j+1];
              inputSize[isVideo][j]=inputSize[isVideo][j+1];
            }
        }
    }else
    {
      inputSize[isVideo][currentIndex]=0;
    }


  if (currentIndex) outputTextEdit->append(QString(MSG_HTML_TAG "Deleted "+groupType+" %1, total size: %2\n").
                                                                           arg(QString::number(currentIndex+1), QString::number(inputSizeCount)));

  if (parent->defaultSaveProjectBehavior) saveProject();
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

void dvda::on_importFromMainTree_clicked()
{
  addSelectedFileToProject();
}


void dvda::on_moveUpItemButton_clicked()
{
  updateIndexInfo();
  if (row == 0) return;
  fileSizeDataBase[isVideo][currentIndex].swap(row, row-1);

  RefreshFlag=SaveAndUpdateTree;
  if (parent->defaultSaveProjectBehavior) saveProject();
  refreshRowPresentation();
}

void dvda::on_moveDownItemButton_clicked()
{
  updateIndexInfo();

  if (row < 0) return;
  if (row == project[isVideo]->fileListWidget->currentListWidget->count() -1) return;

  fileSizeDataBase[isVideo][currentIndex].swap(row, row+1);

  RefreshFlag=SaveAndUpdateTree;
  if (parent->defaultSaveProjectBehavior) saveProject();
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

  if (parent->defaultSaveProjectBehavior) saveProject();
  showFilenameOnly();
}


void dvda::on_retrieveItemButton_clicked()
{

  updateIndexInfo();

  if (row <0) return;

  quint64 size=(quint64) fileSizeDataBase[isVideo][currentIndex].takeAt(row).toInt();

  inputSize[isVideo][currentIndex]-=size;
  inputSizeCount-=size;

  outputTextEdit->append(QString(MSG_HTML_TAG "Retrieved file from " + groupType  + " %1\n"+ groupType+ " size: %2, total size: %3\n").arg(QString::number(currentIndex+1),
                                                                                                                                 QString::number(inputSize[isVideo][currentIndex]), QString::number(inputSizeCount)));
  RefreshFlag=SaveAndUpdateTree;
  if (parent->defaultSaveProjectBehavior) saveProject();

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
 QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);
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

  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

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
  QListIterator<FAbstractWidget*>  w(Abstract::abstractWidgetList);
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

/* Remember that the first two elements of the FAvstractWidgetList are DVD-A and DVD-V respectively, which cuts down parsing time */



inline QString dvda::makeParserString(int start, int end)
{

    QStringList L=QStringList();

    for (int j=start; j <=end; j++)
      {

        FAbstractWidget* widget=Abstract::abstractWidgetList.at(j);
        QString hK=widget->getHashKey();

        if  (widget->getHashKey().isEmpty())
          {
            QMessageBox::warning(this, tr("Error"), tr(".dvp project parsing error"));
            continue;
          }

        QString xml=widget->setXmlFromWidget().toQString();
        QString widgetDepth=widget->getDepth();

        L <<  "  <" + hK + " widgetDepth=\"" + widgetDepth +  "\">\n   "
                                 + xml
              +"\n  </" + hK + ">\n";

      }

    return L.join("");

}


inline QString  dvda::makeDataString()
{
    return  makeParserString(0,1);
}

inline QString  dvda::makeSystemString()
{
    return makeParserString(2);
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
  out << " <data>\n";

  out << dvda::makeDataString();

  out << " </data>\n";
  out << " <system>\n";

  out << dvda::makeSystemString();

  out << " </system>\n <recent>\n";

  QStringListIterator w(parent->recentFiles);
  QString str;
  while (w.hasNext() && QFileInfo(str=w.next()).isFile())
     out    <<  "  <file>" << str << "</file>\n";

  out << " </recent>\n</project>\n";
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


void dvda::assignVariables(const QList<FStringList> &value)
{

  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);
  QListIterator<FStringList> z(value);


  while ((w.hasNext()) && (z.hasNext()))// && (!z.peekNext().isEmpty()))
  {
      w.next()->setWidgetFromXml(z.next());
  }

}

void dvda::assignGroupFiles(const int ZONE, const int group_index, QString size, QString file)
{

  if (group_index > project[ZONE]->getRank())
    {
        /* call the FListFrame function. The dvda auxiliary function will be managed by it */

      outputTextEdit->append(MSG_HTML_TAG "Adding group " + QString::number(group_index));
    }

  QString group_type=(ZONE)?"titleset":"group";

  if (!ZONE) *(project[ZONE]->signalList) << file;
  fileSizeDataBase[ZONE][group_index].append(size);
  quint64 localsize=(quint64) size.toInt();
  inputSize[ZONE][group_index]+=localsize;
  inputSizeCount+=localsize;
  outputTextEdit->append(QString(MSG_HTML_TAG "Added file %4 to "+group_type+" %1:\n"+group_type+" size %2 MB, total size %3 MB\n").arg(QString::number(group_index+1),
                                                                                                                 QString::number(inputSize[ZONE][group_index]), QString::number(inputSizeCount), file));
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
      //emit(is_signalList_changed(project[AUDIO]->signalList->size()));
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

}


