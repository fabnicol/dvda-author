#include "flistframe.h"
#include <QMessageBox>

FListFrame::FListFrame(QObject* parent,  QAbstractItemView* tree, short import_type, const QString &hashKey,
                         const QString &description, const QString &command_line, int cli_type, const QStringList &separator, const QStringList &xml_tags,
                         int mainTabWidgetRank, QIcon *icon, QTabWidget *parentTabWidget,
                         QStringList* terms, QStringList* translation, QStringList* slotL,
                         QWidget* controlledWidget)
{

 importType=import_type;
 tags=xml_tags;
 signalList= new QStringList;
 slotList = slotL;

 fileTreeView=tree;
 fileLabel=new QLabel;
 fileLabelText=  tags[0] + "s for "+ tags[1];
 fileLabel->setWordWrap(true);
 frameHashKey=hashKey;

 fileListWidget = new FListWidget(hashKey,
                                  cli_type,
                                  description,
                                  command_line,
                                  separator,
                                  tags,
                                  NULL,
                                  terms,
                                  translation);

 fileListWidget->rank=0;
 fileListWidget->setObjectName(hashKey+" "+description);

 if (mainTabWidgetRank != -1)
   {
     if (parentTabWidget == NULL)
       {
         embeddingTabWidget = new QTabWidget(this);
       }
     else
       {
         embeddingTabWidget = parentTabWidget;
       }

     mainTabWidget=new QTabWidget(embeddingTabWidget);
     if (icon)
       {
           embeddingTabWidget->insertTab(mainTabWidgetRank, mainTabWidget, *icon, "");
           embeddingTabWidget->setIconSize(QSize(48,48));
           embeddingTabWidget->setMovable(true);
           embeddingTabWidget->setTabToolTip(mainTabWidgetRank, description);
       }
     else
       embeddingTabWidget->insertTab(mainTabWidgetRank, mainTabWidget, xml_tags[1]);
  }
 else
  {
     embeddingTabWidget = new QTabWidget(this);
     embeddingTabWidget->addTab(fileListWidget, xml_tags[1]+" "+ QString::number(fileListWidget->rank+1));
     mainTabWidget=embeddingTabWidget;
  }

 mainTabWidget->addTab(fileListWidget, xml_tags[1]+" "+ QString::number(fileListWidget->rank+1));
 cumulativePicCount =QList<int>() << 0 << 0;
 importFromMainTree = new QToolButton;

 const QIcon importIcon = QIcon(QString::fromUtf8( ":/images/document-import.png"));
 importFromMainTree->setIcon(importIcon);
 importFromMainTree->setIconSize(QSize(22, 22));

 addGroupButton = new QToolButton(this);
 addGroupButton->setToolTip(tr("Add new DVD-Audio group tab"));
 const QIcon iconNew = QIcon(QString::fromUtf8( ":/images/tab-new.png"));
 addGroupButton->setIcon(iconNew);
 addGroupButton->setIconSize(QSize(22,22));

 deleteGroupButton = new QToolButton;
 deleteGroupButton->setToolTip(tr("Delete current DVD-Audio group tab"));
 const QIcon iconDelete = QIcon(QString::fromUtf8( ":/images/tab-close-other.png"));
 deleteGroupButton->setIcon(iconDelete);
 deleteGroupButton->setIconSize(QSize(22,22));

 moveUpItemButton = new QToolButton(this);
 moveUpItemButton->setToolTip(tr("Move group item up"));
 const QIcon iconUp = QIcon(QString::fromUtf8( ":/images/arrow-up.png"));
 moveUpItemButton->setIcon(iconUp);
 moveUpItemButton->setIconSize(QSize(22, 22));

 retrieveItemButton = new QToolButton(this);
 retrieveItemButton->setToolTip(tr("Retrieve group item"));
 retrieveItemButton->setObjectName(QString::fromUtf8("Retrieve"));
 const QIcon iconRetrieve = QIcon(QString::fromUtf8( ":/images/retrieve.png"));
 retrieveItemButton->setIcon(iconRetrieve);
 retrieveItemButton->setIconSize(QSize(22, 22));

 moveDownItemButton = new QToolButton(this);
 moveDownItemButton->setToolTip(tr("Move group item down"));
 moveDownItemButton->setObjectName(QString::fromUtf8("Down"));
 const QIcon iconDown = QIcon(QString::fromUtf8( ":/images/arrow-down.png"));
 moveDownItemButton->setIcon(iconDown);
 moveDownItemButton->setIconSize(QSize(22, 22));

 clearList=new QToolButton;
 clearList->setToolTip(tr("Erase selected menu file list"));
 const QIcon clearIcon = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
 clearList->setIcon(clearIcon);
 clearList->setIconSize(QSize(22,22));

 controlButtonBox=new QGroupBox(this);
 QGridLayout *controlButtonLayout=new QGridLayout;

 controlButtonLayout->addWidget(moveUpItemButton, 1,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(retrieveItemButton, 2,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(moveDownItemButton, 3,1,1,1,Qt::AlignCenter);
 controlButtonLayout->setRowMinimumHeight(4, 50);
 controlButtonLayout->addWidget(clearList, 4, 1,1,1, Qt::AlignTop);
 controlButtonLayout->addWidget(addGroupButton, 5,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(deleteGroupButton, 6,1,1,1,Qt::AlignCenter);
 controlButtonBox->setLayout(controlButtonLayout);
 controlButtonBox->setFlat(true);

 tabBox=new QGroupBox(this);
 QVBoxLayout *tabLayout=new QVBoxLayout;
 tabLayout->addWidget(embeddingTabWidget);
 tabBox->setLayout(tabLayout);
 tabBox->setFlat(true);

 connect(addGroupButton, SIGNAL(clicked()), this, SLOT(addGroup()));
 connect(deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
 connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_mainTabIndex_changed(int)));
 connect(embeddingTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_embeddingTabIndex_changed(int)));
  if (parent) connect(parent, SIGNAL(is_signalList_changed(int)), this, SLOT(on_mainTabIndex_changed(int)));
 connect(importFromMainTree, SIGNAL(clicked()), this,  SLOT(on_importFromMainTree_clicked()));
 connect(moveUpItemButton, SIGNAL(clicked()), this, SLOT(on_moveUpItemButton_clicked()));
 connect(retrieveItemButton, SIGNAL(clicked()), this, SLOT(on_retrieveItemButton_clicked()));
 connect(moveDownItemButton, SIGNAL(clicked()), this, SLOT(on_moveDownItemButton_clicked()));
 connect(fileListWidget, SIGNAL(open_tabs_signal(int)), this, SLOT(addGroups(int)));
 connect(clearList, SIGNAL(clicked()), this, SLOT(on_clearList_clicked()));

}

inline void FListFrame::updateIndexInfo()
{
  currentListWidget=qobject_cast<QListWidget*>(mainTabWidget->currentWidget());
  if (currentListWidget == NULL) return;
  row=currentListWidget->currentRow();
  currentIndex=mainTabWidget->currentIndex();
}

void FListFrame::on_clearList_clicked()
{
  updateIndexInfo();

  fileListWidget->clear();
  for (int j=1; currentIndex +j < cumulativePicCount.count() ; j++)
    cumulativePicCount[currentIndex+j] -= hash::fstringlist[frameHashKey]->at(currentIndex).count();
  hash::fstringlist[frameHashKey]->value(currentIndex).clear();
}

void FListFrame::on_moveUpItemButton_clicked()
{
  updateIndexInfo();

  if (!row) return;

  int currentIndex=mainTabWidget->currentIndex();

  QListWidgetItem *temp=currentListWidget->takeItem(row);
  QListWidgetItem *temp2=currentListWidget->takeItem(row-1);
  currentListWidget->insertItem(row-1, temp);
  currentListWidget->insertItem(row, temp2);
  currentListWidget ->setCurrentRow(row-1);

   (*hash::fstringlist[frameHashKey])[currentIndex].swap(row, row-1);
}

void FListFrame::on_retrieveItemButton_clicked()
{
  updateIndexInfo();

  if (hash::fstringlist[frameHashKey]->at(currentIndex).isEmpty()) return;
  if (row <0) return;

   currentListWidget->takeItem(row);

  (*hash::fstringlist[frameHashKey])[currentIndex].removeAt(row);

   if (row) currentListWidget->setCurrentRow(row-1);
   row--;
}

void FListFrame::on_moveDownItemButton_clicked()
{

  updateIndexInfo();

  if (row < 0) return;
  if (row == currentListWidget->count() -1) return;

  QListWidgetItem *temp=currentListWidget->takeItem(row+1);
  QListWidgetItem *temp2=currentListWidget->takeItem(row);
  currentListWidget->insertItem(row, temp);
  currentListWidget->insertItem(row+1, temp2);
  currentListWidget->setCurrentRow(row+1);

   (*hash::fstringlist[frameHashKey])[currentIndex].swap(row, row+1);
}

void FListFrame::on_mainTabIndex_changed(int index)
{
  if ((slotList == NULL) || ( slotList->count() <= index)) return;
  if (index > 0) fileLabel->setText(fileLabelText+" "+QString::number(index)+"\n"+ slotList->at(index-1));
  // now delete groups that should be deleted, possibly parsing hashes to determine which
  // use deleteGroups(L) when list of groups (=tracks) to be deleted is determined

  // do likewise for project[AUDIO] tracks/ FListFrame groups to be added.
}


void FListFrame::on_embeddingTabIndex_changed(int index)
{
 if ((!slotList) || (slotList->count() <= mainTabWidget->currentIndex())) return;
 if (index  > -1) fileLabel->setText(fileLabelText+" "+QString::number(mainTabWidget->currentIndex()+1)+"\n"+ slotList->at(mainTabWidget->currentIndex()));
}

void FListFrame::addGroup()
{
   slotListSize=(slotList)? slotList->size() : 0;
   if ((slotListSize) && (fileListWidget->rank >= slotListSize-1)) return;

   if ((hash::fstringlist[frameHashKey]->size() < slotListSize) || slotListSize == 0) hash::fstringlist[frameHashKey]->append(QStringList());

   if (cumulativePicCount.count() <  slotListSize+1) cumulativePicCount.append(cumulativePicCount[fileListWidget->rank]+hash::fstringlist[frameHashKey]->at(fileListWidget->rank).count());

   fileListWidget->rank++;

 mainTabWidget->insertTab(fileListWidget->rank, new QListWidget, tags[1] + " "+ QString::number(fileListWidget->rank+1));
 mainTabWidget->setCurrentIndex(fileListWidget->rank);
}

void FListFrame::addGroups(int n)
{
 for (int j=1; j<= n; j++)
   {
     QListWidget* widget =new QListWidget;
      widget->addItems((*hash::fstringlist[frameHashKey])[j]);
      mainTabWidget->insertTab(j, widget, tags[1] + " "+ QString::number(j+1));
      *signalList << hash::fstringlist.value(frameHashKey)->at(j);
   }

}


void FListFrame::deleteGroup()
{
 updateIndexInfo();

 if (fileListWidget->rank < 1) return;

 mainTabWidget->removeTab(currentIndex);

 hash::fstringlist[frameHashKey]->removeAt(currentIndex);

 fileListWidget->rank--;

 if (currentIndex <fileListWidget->rank)
   {

     for (int j=currentIndex; j < fileListWidget->rank+1 ; j++)
       {
         mainTabWidget->setTabText(j,  tags[1] + " " + QString::number(j+1));
       }
   }
}


void FListFrame::deleteGroups(QList<int> &L)
{

 foreach (int j,  L)
   {
     mainTabWidget->removeTab(j);
     hash::fstringlist[frameHashKey]->removeAt(j);
     fileListWidget->rank--;
   }

 if (L[0] <fileListWidget->rank)
   {

     for (int j=L[0]; j < fileListWidget->rank+1 ; j++)
       {
         mainTabWidget->setTabText(j,  tags[1] + " " + QString::number(j+1));
       }
   }
}


void FListFrame::addDirectoryToListWidget(QDir& dir, int filerank)
{
 QStringList filters;
 filters+="*";

 foreach (QFileInfo file, dir.entryInfoList(filters,QDir::AllDirs | QDir::NoDotAndDotDot|QDir::Files))
   {
     if (file.isDir())
       {
         QDir dir=QDir(file.canonicalFilePath());
         addDirectoryToListWidget(dir, filerank);
       }
     else
       {
         QString path=file.canonicalFilePath();
         addStringToListWidget(path, filerank);
       }
   }
}


bool FListFrame::addStringToListWidget(QString filepath, int file)
{
 if ((filepath.isEmpty()) || (fileListWidget->rank >= (*hash::fstringlist[frameHashKey]).count() ) || (signalList == NULL)) return false;
 // normaly it should be useless to call updateIndexInfo() here

 (*hash::fstringlist[frameHashKey])[currentIndex] << filepath;
 *(fileListWidget->signalList) << filepath;
 *signalList << filepath; //make a copy. Necessary to avoid losing dragged and dropped files to list widget directly.
 for (int j=1; file+j < cumulativePicCount.count() ;  j++)
   cumulativePicCount[file+j]++;
 //Q(signalList->join(" "))
 emit(is_signalList_changed(!signalList->isEmpty()));
 return true;
}


void FListFrame::on_importFromMainTree_clicked()
{

  updateIndexInfo();
 if (slotList)
   {
     slotListSize=slotList->count();

     if(slotListSize == 0)
       {
         QMessageBox::warning(this, "No "+ tags[1] +"s in appropriate tab widget\n", "You should first import some "+tags[1]+"(s) before adding a "+tags[0]);
         return;
       }
   }

 QItemSelectionModel *selectionModel = fileTreeView->selectionModel();
 QModelIndexList  indexList=selectionModel->selectedIndexes();

 if (indexList.isEmpty()) return;

 uint size=indexList.size();

 for (uint i = 0; i < size; i++)
   {
     QModelIndex index;
     index=indexList.at(i);

     if (importType == flags::importFiles)
       {
         QFileInfo info=model->fileInfo(index);

         if (info.isFile())
           {
             QString filepath=info.canonicalFilePath();
             currentListWidget->addItem(filepath);
             addStringToListWidget(filepath, currentIndex);
           }
         else if (info.isDir())
           {
             QDir dir=info.absoluteDir();
             addDirectoryToListWidget(dir, currentIndex);
           }
         else
           {
             QMessageBox::warning(this, tr("Browse"),
                                  tr("%1 is not a file or a directory.").arg(index.data().toString()));
           }
       }
     else if (importType == flags::importNames)
       {
         QString name=index.data().toString();

         currentListWidget->addItem(name);
         addStringToListWidget(name, currentIndex);
       }
  }
}
