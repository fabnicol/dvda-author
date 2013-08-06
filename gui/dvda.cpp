
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



int dvda::RefreshFlag=0;
int flags::lplexRank=0;
qint64   dvda::totalSize[]={0,0};
int dvda::dialVolume=25;
class Hash;


void dvda::initialize()
{
  adjustSize();
  extraAudioFilters=QStringList() << "*.wav" << "*.flac";
  Hash::description["titleset"]={"DVD-Video titleset"};
  Hash::description["group"]={"DVD-Audio group"};
  Hash::description["recent"]={"Recent file"};
}


void dvda::on_playItem_changed()
{
  if (!myMusic ) return;
  myMusic->setMedia(QUrl::fromLocalFile(Hash::wrapper.value(dvda::zoneTag())->at(currentIndex).at(row)));
  myMusic->play();
}


void dvda::on_playItemButton_clicked()
{
  static int count;
  updateIndexInfo();
  if (row < 0)
    {
      row=0;
      project[isVideo]->getCurrentWidget()->setCurrentRow(0);
    }
  updateIndexChangeInfo();

  if (count == 0)
    {
      myMusic = new QMediaPlayer(this, QMediaPlayer::StreamPlayback);
      myMusic->setMedia(QMediaContent(QUrl::fromLocalFile(Hash::wrapper[dvda::zoneTag(isVideo)]->at(currentIndex).at(row))));
      myMusic->setVolume(dvda::dialVolume);
      myMusic->play();
    }

  if (count % 2 == 0)
    {
      myMusic->play();
      outputTextEdit->append(tr(INFORMATION_HTML_TAG "Playing...   file %1\n   in %2 %3   row %4" )
                                  .arg(Hash::wrapper.value(dvda::zoneTag())->at(currentIndex).at(row),
                                  zoneGroupLabel(isVideo),QString::number(currentIndex+1),QString::number(row+1)));

      playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
      playItemButton->setToolTip(tr("Stop playing"));
    }
  else
    {
      playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
      playItemButton->setToolTip(tr("Play selected file"));
      myMusic->stop();
      outputTextEdit->append(tr(INFORMATION_HTML_TAG "Stopped."));
    }
  count++;
}

dvda::dvda()
{
  setAttribute(Qt::WA_DeleteOnClose);
  initialize();
  setAcceptDrops(true);

  model->setReadOnly(false);
  model->setRootPath(QDir::homePath());
  model->sort(Qt::AscendingOrder);
  model->setNameFilterDisables(false);

  fileTreeView->setModel(model);
  fileTreeView->hideColumn(1);
  fileTreeView->setMinimumWidth(400);
  fileTreeView->setColumnWidth(0,300);

  fileTreeView->header()->setStretchLastSection(true);
  fileTreeView->header()->setSortIndicator(0, Qt::AscendingOrder);
  fileTreeView->header()->setSortIndicatorShown(true);

  fileTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  fileTreeView->setSelectionBehavior(QAbstractItemView::SelectItems);

  QModelIndex index = model->index(QDir::currentPath());
  fileTreeView->expand(index);
  fileTreeView->scrollTo(index);

  audioFilterButton->setToolTip("Show audio files with extension "+ common::extraAudioFilters.join(", ")+"\nTo add extra file formats to this filter button go to Options>Audio Processing,\ncheck the \"Enable multiformat input\" box and fill in the file format field.");
  const QIcon iconAudioFilter = QIcon(QString::fromUtf8( ":/images/audio_file_icon.png"));
  audioFilterButton->setIcon(iconAudioFilter);
  audioFilterButton->setIconSize(QSize(22, 22));
  audioFilterButton->setCheckable(true);

  QIcon* iconDVDA = new QIcon(":/images/64x64/dvd-audio.png");
  QIcon* iconDVDV = new QIcon(":/images/64x64/dvd-video.png");

  project[AUDIO]=new FListFrame(NULL,      // no parent widget
                                fileTreeView,                   // files may be imported from this tree view
                                importFiles,                     // FListFrame type
                                "DVD-A",                          // superordinate xml tag
  {"DVD-Audio"},                   // project manager widget on-screen tag
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

  project[VIDEO]=new FListFrame(NULL,
                                fileTreeView,                   // files may be imported from this tree view
                                importFiles,                     // FListFrame type
                                "DVD-V",                          // superordinate xml tag
  {"DVD-Video"},                   // project manager widget on-screen tag
                                "",                                   // command line label
                                lplexFiles | hasListCommandLine|flags::enabled,  // command line characteristic features
                               {" ", " -ts "},                     // command line separators
                               {"file", "titleset"},             // subordinate xml tags
                                1,                                    // rank
                                iconDVDV,                      // tab icon
                                mainTabWidget);             // parent tab under which this frame is inserted

  project[VIDEO]->embeddingTabWidget->setIconSize(QSize(64, 64));

  mkdirButton->setToolTip(tr("Create Directory..."));
  const QIcon iconCreate = QIcon(QString::fromUtf8( ":/images/folder-new.png"));
  mkdirButton->setIcon(iconCreate);
  mkdirButton->setIconSize(QSize(22, 22));

  removeButton->setToolTip(tr("Remove directory or file..."));
  const QIcon iconRemove = QIcon(QString::fromUtf8( ":/images/edit-delete.png"));
  removeButton->setIcon(iconRemove);
  removeButton->setIconSize(QSize(22, 22));

    playItemButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  playItemButton->setIconSize(QSize(22, 22));
  playItemButton->setToolTip(tr("Play selected file"));

  dial->setFocusPolicy(Qt::StrongFocus);
  dial->setMinimum(0);
  dial->setMaximum(100);
  dial->setValue(dvda::dialVolume);
  dial->setNotchesVisible(true);
  dial->setMaximumWidth(40);
  dial->setToolTip(tr("Volume"));

  killButton->setToolTip(tr("Kill dvda-author"));
  const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
  killButton->setIcon(iconKill);
  killButton->setIconSize(QSize(22,22));

  progress->reset();
  progress->setRange(0, maxRange=100);
  progress->setToolTip(tr("DVD-Audio structure authoring progress bar"));

  outputTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  outputTextEdit->setAcceptDrops(false);
  outputTextEdit->setMinimumHeight(200);

  QGridLayout *projectLayout = new QGridLayout;
  QGridLayout *updownLayout = new QGridLayout;
  QVBoxLayout *mkdirLayout = new QVBoxLayout;
  QHBoxLayout *progress1Layout= new QHBoxLayout;

  mkdirLayout->addWidget(mkdirButton);
  mkdirLayout->addWidget(removeButton);
  mkdirLayout->addWidget(audioFilterButton);
  projectLayout->addLayout(mkdirLayout,0,0);

  connect(mkdirButton, SIGNAL(clicked()), this, SLOT(createDirectory()));
  connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
  connect(killButton, SIGNAL(clicked()), this, SLOT(killDvda()));

  connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
  connect(&process2, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process2Finished(int, QProcess::ExitStatus)));
  connect(&process2, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_cdrecordButton_clicked()));
  connect(&process3, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(process3Finished(int, QProcess::ExitStatus)));
  connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_frameTab_changed(int )));
  connect(playItemButton, SIGNAL(clicked()), this, SLOT(on_playItemButton_clicked()));
  connect(dial, &QDial::valueChanged, [&]{ dvda::dialVolume=dial->value(); if (myMusic) myMusic->setVolume(dvda::dialVolume);});
  connect(this, SIGNAL(hasIndexChangedSignal()), this, SLOT(on_playItem_changed()));
  connect(audioFilterButton, SIGNAL(toggled(bool)), this, SLOT(on_audioFilterButton_clicked(bool)));

  for (int ZONE : {AUDIO, VIDEO})
 {
      project[ZONE]->model=model;
      project[ZONE]->slotList=NULL;
      connect(project[ZONE]->addGroupButton, SIGNAL(clicked()), this, SLOT(addGroup()));
      connect(project[ZONE]->deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
      connect(project[ZONE]->importFromMainTree, &QToolButton::clicked, [this] {addSelectedFileToProject();});
      connect(project[ZONE]->moveUpItemButton, SIGNAL(clicked()), this, SLOT(on_moveUpItemButton_clicked()));
      connect(project[ZONE]->moveDownItemButton, SIGNAL(clicked()), this, SLOT(on_moveDownItemButton_clicked()));
      connect(project[ZONE]->retrieveItemButton, SIGNAL(clicked()), this, SLOT(on_deleteItem_clicked()));
      connect(project[ZONE]->clearListButton, &QToolButton::clicked, [this] { saveProject(); displayTotalSize(); });
      // set visible importFromMaintree and controlButtonBox !
      projectLayout->addWidget(project[ZONE]->tabBox, 0,2);
      updownLayout->addWidget(project[ZONE]->controlButtonBox, 0,0);
      //in this order!
      projectLayout->addWidget(project[1-ZONE]->importFromMainTree, 0,1);
  }

  updownLayout->setRowMinimumHeight(1, 40);
  updownLayout->addWidget(playItemButton, 2, 0,Qt::AlignBottom | Qt::AlignHCenter);
  updownLayout->addWidget(dial, 3, 0, Qt::AlignTop | Qt::AlignHCenter);
  updownLayout->setRowMinimumHeight(3, 40);

  projectLayout->addLayout(updownLayout, 0,3);

  mainLayout->addLayout(projectLayout);

  progress1Layout->addWidget(killButton);
  progress1Layout->addWidget(progress);
  progressLayout->addLayout(progress1Layout);

  mainLayout->addLayout(progressLayout);

  QStringList labels;
  labels << tr("Setting") << tr("Value/Path") << tr("Size");
  managerWidget->hide();
  managerWidget->setHeaderLabels(labels);

  managerLayout->addWidget(managerWidget);

  allLayout->addLayout(mainLayout);
  allLayout->addLayout(managerLayout);

  setLayout(allLayout);
  setWindowTitle(tr("dvda-author"));
  const QIcon dvdaIcon=QIcon(QString::fromUtf8( ":/images/dvda-author.png"));
  setWindowIcon(dvdaIcon);

}


void dvda::on_frameTab_changed(int index)
{
    for (int ZONE: {AUDIO, VIDEO})
    {
        project[ZONE]->controlButtonBox->setVisible(index == ZONE);
        project[ZONE]->importFromMainTree->setVisible(index == ZONE);
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

void dvda::refreshRowPresentation()
{
  // indexes are supposed to have been recently updated
  refreshRowPresentation(isVideo, currentIndex);
}


void dvda::refreshRowPresentation(uint ZONE, uint j)
{

  QPalette palette;
  palette.setColor(QPalette::AlternateBase,QColor("silver"));
  QFont font=QFont("Courier",10);

  QListWidget *widget=project[ZONE]->getWidgetContainer(j);
  if (widget == nullptr) return;
  widget->setPalette(palette);
  widget->setAlternatingRowColors(true);
  widget->setFont(font);

  for (int r=0; (r < widget->count()) && (r < Hash::wrapper[zoneTag(ZONE)]->at(j).size()); r++ )
    {

      widget->item(r)->setText(Hash::wrapper.value(zoneTag(ZONE))->at(j).at(r).section('/',-1));
      widget->item(r)->setTextColor(QColor("navy"));
      //widget->item(r)->setToolTip(fileSizeDataBase[ZONE].at(j).at(r)+" B");
    }
}

//TODO insert button somewhere or right-click option, and back to sort by name
void dvda::showFilenameOnly()
{
  updateIndexInfo();
  refreshRowPresentation(isVideo, currentIndex);
 }




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
        options::RefreshFlag |= UpdateOptionTabs;
        refreshProjectManager();
    }

    checkEmptyProjectName();
    setCurrentFile(projectName);
}

void dvda::closeProject()
{
  projectName="";
  clearProjectData();

  dvda::totalSize[AUDIO]=dvda::totalSize[VIDEO]=0;
  displayTotalSize();

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
  RefreshFlag = RefreshFlag|UpdateMainTabs|UpdateOptionTabs|UpdateTree;

  for (int ZONE : {AUDIO, VIDEO})
    {
      for (int i=0; i <= project[ZONE]->getRank(); i++)
             project[ZONE]->on_clearList_clicked(i);

      project[ZONE]->signalList->clear();
      project[ZONE]->clearWidgetContainer();
      fileSizeDataBase[ZONE].clear();
    }

   managerWidget->clear();

  QMessageBox::StandardButton choice=QMessageBox::Cancel;

  if (options::RefreshFlag ==  hasUnsavedOptions)
    {
      choice=QMessageBox::information(this, "New settings",
                                      "This project contains new option settings.\nPress OK to replace your option settings,\notherwise No to parse only file paths\nor Cancel to exit project parsing.\n",
                                      QMessageBox::Ok|QMessageBox::No|QMessageBox::Cancel);
      switch (choice)
        {
            case QMessageBox::Ok  :
              parent->dialog->clearOptionData();
              break;

            case QMessageBox::No :
              options::RefreshFlag = KeepOptionTabs;
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

    /* cleanly wipe out main Hash */
        Abstract::initializeFStringListHashes();
}

void dvda::on_helpButton_clicked()
{
  QUrl url=QUrl::fromLocalFile(this->generateDatadirPath("GUI.html") );
   browser::showPage(url);
}

void dvda::on_openManagerWidgetButton_clicked(bool isHidden)
{
   managerWidget->setVisible(isHidden);
 }

void dvda::on_openManagerWidgetButton_clicked()
{
    on_openManagerWidgetButton_clicked(managerWidget->isHidden());
}

void dvda::addGroup()
{
  updateIndexInfo();

  if (project[isVideo]->getRank() >=  9*(int) isVideo*10+9)
   {
      QMessageBox::information(this, tr("Group"), tr(QString("A maximum of %1 "+ zoneGroupLabel(isVideo)+ "s can be created.").toUtf8()).arg(QString::number(9*isVideo*10+9)));
      return;
    }
}


void dvda::displayTotalSize()
{
    static qint64 comp;
    qint64 tot=dvda::totalSize[AUDIO]+dvda::totalSize[VIDEO];
    if (tot != comp)
       outputTextEdit->append(MSG_HTML_TAG "Total size:  " + QString::number(tot) + " B ("+QString::number(tot/(1024*1024))+" MB)");
    comp=tot;
}

void dvda::deleteGroup()
{
  updateIndexInfo();
  uint rank=(uint) project[isVideo]->getRank();

  if ((uint) fileSizeDataBase[isVideo].size() > currentIndex)
      fileSizeDataBase[isVideo][currentIndex].clear();

  if (rank > 0)
    {
      if (currentIndex < rank)
        {

          for (unsigned j=currentIndex; j < rank ; j++)
            {
              fileSizeDataBase[isVideo][j]=fileSizeDataBase[isVideo][j+1];
             }
        }
    }

  saveProject();
  displayTotalSize();
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
  currentIndex=project[isVideo]->getCurrentIndex();
  row=project[isVideo]->getCurrentRow();

    // row = -1 if nothing selected
}


void dvda::on_moveUpItemButton_clicked()
{
  updateIndexInfo();
  if (row == 0) return;
  fileSizeDataBase[isVideo][currentIndex].swap(row, row-1);

  RefreshFlag |= SaveTree|UpdateTree;
  saveProject();
  refreshRowPresentation();
}

void dvda::on_moveDownItemButton_clicked()
{
  updateIndexInfo();
  if (row < 0) return;
  if (row == project[isVideo]->getCurrentWidget()->count() -1) return;

  fileSizeDataBase[isVideo][currentIndex].swap(row, row+1);
  RefreshFlag |= SaveTree | UpdateTree;
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
              RefreshFlag |= SaveTree|UpdateTree;
        }
      else
        {
          QMessageBox::warning(this, tr("Browse"),
                               tr("%1 is not a file or a directory.").arg(model->fileInfo(index).fileName()));
          return;
        }
    }

  saveProject();
  // in this order
  displayTotalSize();
  showFilenameOnly();
}


void dvda::on_deleteItem_clicked()
{
  RefreshFlag |= SaveTree | UpdateTree;
  saveProject();
  updateIndexInfo();
  displayTotalSize();
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



void dvda::requestSaveProject()
{
  projectName=QFileDialog::getSaveFileName(this,  tr("Set project file name"), "default.dvp", tr("dvp projects (*.dvp)"));
  saveProject(true);
}


void dvda::saveProject(bool requestSave)
{
  QListIterator<FAbstractWidget*>  w(Abstract::abstractWidgetList);

  // On adding files or deleting files, or saving project, write project file and the update tree par reparsing project
  // Yet do not reparse tabs, as it should be useless (Tabs have been refreshed already)

  RefreshFlag = SaveTree|UpdateTree ;
  //if ((RefreshFlag&hasProjectManagerTreeMask) == hasNoProjectManagerTree)   RefreshFlag |= hasProjectManagerTree;

  audioFilterButton->setToolTip("Show audio files with extension "+ common::extraAudioFilters.join(", ")+"\nTo add extra file formats to this filter button go to Options>Audio Processing,\ncheck the \"Enable multiformat input\" box and fill in the file format field.");

  if (parent->defaultSaveProjectBehavior->isChecked() || requestSave)
        writeProjectFile();

  refreshProjectManager();
}

/* Remember that the first two elements of the FAvstractWidgetList are DVD-A and DVD-V respectively, which cuts down parsing time */


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


void dvda::assignVariables()
{
  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

  if (w.hasNext())
  {
      FAbstractWidget* widget=w.next();
      if (dvda::RefreshFlag&UpdateMainTabs)
      {
             widget->setWidgetFromXml(*Hash::wrapper[widget->getHashKey()]);
      }
  }

  if (w.hasNext())
  {
     FAbstractWidget* widget=w.next();
      if (dvda::RefreshFlag&UpdateMainTabs)
              widget->setWidgetFromXml(*Hash::wrapper[widget->getHashKey()]);
  }

  if (options::RefreshFlag&UpdateOptionTabs)
      while (w.hasNext())
      {
          FAbstractWidget* widget=w.next();
          widget->setWidgetFromXml(*Hash::wrapper[widget->getHashKey()]);
      }

}

void dvda::assignGroupFiles(const int ZONE, const int group_index,  const QString& file)
{
  static int last_group;
  if (group_index-last_group) outputTextEdit->append(MSG_HTML_TAG "Adding group " + QString::number(group_index+1));
  last_group=group_index;
  if (!ZONE) *(project[ZONE]->signalList) << file;
}


bool dvda::refreshProjectManager()
{
  // Step 1: prior to parsing
      checkEmptyProjectName();
      QFile file(projectName);

  if ((RefreshFlag&UpdateTreeMask) == UpdateTree)
    {
      managerWidget->clear();
    }

  if ((RefreshFlag&SaveTreeMask) == SaveTree)
    {
      if (!file.isOpen())
        file.open(QIODevice::ReadWrite);
      else
        file.seek(0);
    }

  // Step 2: parsing on opening .dvp project  (=update tree +refresh tabs) or adding/deleting tab files (=update tree)

  if ((RefreshFlag&UpdateTreeMask) == UpdateTree)
     {
      QPalette palette;
      palette.setColor(QPalette::AlternateBase,QColor("silver"));
      managerWidget->setPalette(palette);
      managerWidget->setAlternatingRowColors(true);

      if ((RefreshFlag&ParseXmlMask) == ParseXml)  // refresh display by parsing xml file again
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

          DomParser(&file);

      }
      else  // refresh display using containers without parsing xml file
      {
          refreshProjectManagerValues(refreshProjectInteractiveMode | refreshAudioZone |  refreshVideoZone | refreshSystemZone);
      }

      // Step3: adjusting project manager size
      managerWidget->resizeColumnToContents(0);
      managerWidget->resizeColumnToContents(1);
      managerWidget->resizeColumnToContents(2);
     }

  if (file.isOpen()) file.close();
  RefreshFlag &= hasSavedOptionsMask|SaveTreeMask|UpdateTreeMask|UpdateTabMask ;

  return true;

}



void dvda::dragEnterEvent(QDragEnterEvent *event)
{

    if (event->source() != this)
        {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
}

void dvda::dragMoveEvent(QDragMoveEvent *event)
{

    if (event->source() != this)
    {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void dvda::dropEvent(QDropEvent *event)
{

    if (event->source() != this)
    {
        QList<QUrl> urls=event->mimeData()->urls();
        if (urls.isEmpty()) return;

        QString fileName = urls.first().toLocalFile();
        if (fileName.isEmpty()) return;

        addDraggedFiles(urls);
    }

}


void dvda::addDraggedFiles(const QList<QUrl>& urls)
{
  updateIndexInfo();

  for (const QUrl &u: urls)
    {
      if (false == project[isVideo]->addStringToListWidget(u.toLocalFile(), currentIndex)) return;
    }
  saveProject();
  showFilenameOnly();
}

