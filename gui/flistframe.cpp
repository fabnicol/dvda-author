#include "flistframe.h"
#include "common.h"

#include <QMessageBox>

FListFrame::FListFrame(QObject* parent,  QAbstractItemView* tree, short import_type, const QString &hashKey,
                         const QString &description, const QString &command_line, int cli_type, const QStringList &separator, const QStringList &xml_tags,
                         int mainTabWidgetRank, QIcon *icon, QTabWidget *parentTabWidget,
                         QStringList* terms, QStringList* translation, QStringList* slotL)

{
 setAcceptDrops(true);
 currentIndex=0;  // necessary for project parsing
 importType=import_type;
 tags=xml_tags;
 slotList = slotL;

 fileTreeView=tree;
 fileLabelText=  tags[0] + "s for "+ tags[1];
 fileLabel->setWordWrap(true);
 frameHashKey=hashKey;

 fileListWidget = new FListWidget(hashKey,
                                  cli_type,
                                  description,
                                  command_line,
                                  separator,
                                  tags,
                                  terms,
                                  translation);

 initializeWidgetContainer();

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
     embeddingTabWidget->addTab(fileListWidget->currentListWidget, xml_tags[1]+" 1");
     mainTabWidget=embeddingTabWidget;
  }


 mainTabWidget->addTab(fileListWidget->currentListWidget, xml_tags[1]+" 1");
 cumulativePicCount =QList<int>() << 0 << 0;

 const QIcon importIcon = QIcon(QString::fromUtf8( ":/images/document-import.png"));
 importFromMainTree->setIcon(importIcon);
 importFromMainTree->setIconSize(QSize(22, 22));

 addGroupButton->setToolTip(tr("Add new DVD-Audio group tab"));
 const QIcon iconNew = QIcon(QString::fromUtf8( ":/images/tab-new.png"));
 addGroupButton->setIcon(iconNew);
 addGroupButton->setIconSize(QSize(22,22));

 deleteGroupButton->setToolTip(tr("Delete current DVD-Audio group tab"));
 const QIcon iconDelete = QIcon(QString::fromUtf8( ":/images/tab-close-other.png"));
 deleteGroupButton->setIcon(iconDelete);
 deleteGroupButton->setIconSize(QSize(22,22));

 moveUpItemButton->setToolTip(tr("Move group item up"));
 const QIcon iconUp = QIcon(QString::fromUtf8( ":/images/arrow-up.png"));
 moveUpItemButton->setIcon(iconUp);
 moveUpItemButton->setIconSize(QSize(22, 22));

 retrieveItemButton->setToolTip(tr("Retrieve group item"));
 retrieveItemButton->setObjectName(QString::fromUtf8("Retrieve"));
 const QIcon iconRetrieve = QIcon(QString::fromUtf8( ":/images/retrieve.png"));
 retrieveItemButton->setIcon(iconRetrieve);
 retrieveItemButton->setIconSize(QSize(22, 22));

 moveDownItemButton->setToolTip(tr("Move group item down"));
 moveDownItemButton->setObjectName(QString::fromUtf8("Down"));
 const QIcon iconDown = QIcon(QString::fromUtf8( ":/images/arrow-down.png"));
 moveDownItemButton->setIcon(iconDown);
 moveDownItemButton->setIconSize(QSize(22, 22));

 clearListButton->setToolTip(tr("Erase selected menu file list"));
 const QIcon clearIcon = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
 clearListButton->setIcon(clearIcon);
 clearListButton->setIconSize(QSize(22,22));

 QGridLayout *controlButtonLayout=new QGridLayout;

 controlButtonLayout->addWidget(moveUpItemButton, 1,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(retrieveItemButton, 2,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(moveDownItemButton, 3,1,1,1,Qt::AlignCenter);
 controlButtonLayout->setRowMinimumHeight(4, 50);
 controlButtonLayout->addWidget(clearListButton, 4, 1,1,1, Qt::AlignTop);
 controlButtonLayout->addWidget(addGroupButton, 5,1,1,1,Qt::AlignCenter);
 controlButtonLayout->addWidget(deleteGroupButton, 6,1,1,1,Qt::AlignCenter);
 controlButtonBox->setLayout(controlButtonLayout);
 controlButtonBox->setFlat(true);

 QVBoxLayout *tabLayout=new QVBoxLayout;
 tabLayout->addWidget(embeddingTabWidget);
 tabBox->setLayout(tabLayout);
 tabBox->setFlat(true);

 connect(addGroupButton, &QToolButton::clicked, [=] () {
                                                                                                     if (hash::FStringListHash[frameHashKey]->last().isEmpty()) return;
                                                                                                     hash::FStringListHash[frameHashKey]->append(QStringList());
                                                                                                     addGroup();
                                                                                                  });
 connect(deleteGroupButton, SIGNAL(clicked()), this, SLOT(deleteGroup()));
 connect(mainTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_mainTabIndex_changed(int)));
 connect(embeddingTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_embeddingTabIndex_changed(int)));
  if (parent) connect(parent, SIGNAL(is_signalList_changed(int)), this, SLOT(on_mainTabIndex_changed(int)));
 connect(importFromMainTree, SIGNAL(clicked()), this,  SLOT(on_importFromMainTree_clicked()));
 connect(moveUpItemButton, SIGNAL(clicked()), this, SLOT(on_moveUpItemButton_clicked()));
 connect(retrieveItemButton, SIGNAL(clicked()), this, SLOT(on_deleteItem_clicked()));
 connect(moveDownItemButton, SIGNAL(clicked()), this, SLOT(on_moveDownItemButton_clicked()));
 connect(fileListWidget, SIGNAL(open_tabs_signal(int)), this, SLOT(addGroups(int)));
 connect(clearListButton, SIGNAL(clicked()), this, SLOT(on_clearList_clicked()));

}



void FListFrame::on_clearList_clicked(int currentIndex)
{
    if (currentIndex == -1)
    {
       updateIndexInfo();
       currentIndex=this->currentIndex;
    }

  if (hash::FStringListHash[frameHashKey]->count() < currentIndex+1) return;

  widgetContainer[currentIndex]->clear();

  /* warning : use *[], not ->value, to modifie any list content, even subordinate */

  (*hash::FStringListHash[frameHashKey])[currentIndex].clear();

  for (int j=1; currentIndex +j < cumulativePicCount.count() ; j++)
      if  (cumulativePicCount.count() >= currentIndex+j+1)
          cumulativePicCount[currentIndex+j] -= hash::FStringListHash[frameHashKey]->at(currentIndex).count();

}

void FListFrame::on_moveUpItemButton_clicked()
{
  updateIndexInfo();

  if (!row) return;

  int currentIndex=mainTabWidget->currentIndex();

  QListWidgetItem *temp=fileListWidget->currentListWidget->takeItem(row);
  QListWidgetItem *temp2=fileListWidget->currentListWidget->takeItem(row-1);
  fileListWidget->currentListWidget->insertItem(row-1, temp);
  fileListWidget->currentListWidget->insertItem(row, temp2);
  fileListWidget->currentListWidget ->setCurrentRow(row-1);

   (*hash::FStringListHash[frameHashKey])[currentIndex].swap(row, row-1);
}

void FListFrame::on_deleteItem_clicked()
{
  updateIndexInfo();

  if (hash::FStringListHash[frameHashKey]->at(currentIndex).isEmpty()) return;
  if (row <0) return;

  QModelIndexList L=fileListWidget->currentListWidget->selectionModel()->selectedRows();
  int size=L.size();
  int  rank=0, localrow;
  while (rank < size)
  {
      localrow=L[rank].row() - rank;
      fileListWidget->currentListWidget->takeItem(localrow);
      (*hash::FStringListHash[frameHashKey])[currentIndex].removeAt(localrow);
      rank++;
  }

   if (localrow) fileListWidget->currentListWidget->setCurrentRow(localrow-1);
   else if (localrow==0) fileListWidget->currentListWidget->setCurrentRow(0);
   row=localrow-1;
}

void FListFrame::on_moveDownItemButton_clicked()
{

  updateIndexInfo();

  if (row < 0) return;
  if (row == fileListWidget->currentListWidget->count() -1) return;

  QListWidgetItem *temp=fileListWidget->currentListWidget->takeItem(row+1);
  QListWidgetItem *temp2=fileListWidget->currentListWidget->takeItem(row);
  fileListWidget->currentListWidget->insertItem(row, temp);
  fileListWidget->currentListWidget->insertItem(row+1, temp2);
  fileListWidget->currentListWidget->setCurrentRow(row+1);

   (*hash::FStringListHash[frameHashKey])[currentIndex].swap(row, row+1);
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


inline void FListFrame::addNewTab()
{
    mainTabWidget->insertTab(getRank() ,widgetContainer.at(getRank()) , tags[1] + " "+ QString::number(getRank()+1));
    mainTabWidget->setCurrentIndex(getRank());
}

void FListFrame::addGroup()
{
        int size=hash::FStringListHash[frameHashKey]->size();
        slotListSize=(slotList)? slotList->size() : 0;

        // do not create an new group over an empty group (strict behaviour)

       if (size < 2 ||hash::FStringListHash[frameHashKey]->at(size-2).isEmpty()) return;
      //  if ((slotListSize) && (getRank() >= slotListSize-1)) return;

        if (cumulativePicCount.count() <  slotListSize+1) cumulativePicCount.append(cumulativePicCount[getRank()]+hash::FStringListHash[frameHashKey]->at(getRank()).count());

        //TODO: check this out

        fileListWidget->currentListWidget=new QListWidget;
        fileListWidget->currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        fileListWidget->currentListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

        widgetContainer << fileListWidget->currentListWidget;

        addNewTab();
}

/* Unlike addGroup, this function is just used for reading groups from Xml */

void FListFrame::addGroups(int n)
{
    widgetContainer.clear();
    widgetContainer << fileListWidget->currentListWidget;
    for (int j=0; j <= n; j++)
   {
     if (j) addGroup();

     widgetContainer[j]->addItems((*hash::FStringListHash[frameHashKey])[j]);
      *signalList << hash::FStringListHash.value(frameHashKey)->at(j);
   }
}


void FListFrame::deleteGroup()
{
 updateIndexInfo();

 if (getRank() < 1) return;

 mainTabWidget->removeTab(currentIndex);

 hash::FStringListHash[frameHashKey]->removeAt(currentIndex);

 if (currentIndex <getRank())
   {

     for (int j=currentIndex; j < getRank()+1 ; j++)
       {
         mainTabWidget->setTabText(j,  tags[1] + " " + QString::number(j+1));
       }
   }

 if (getRank() < widgetContainer.size()) widgetContainer.removeAt(getRank());

}

#if 0
void FListFrame::deleteGroups(QList<int> &L)
{

 foreach (int j,  L)
   {
     mainTabWidget->removeTab(j);
     hash::FStringListHash[frameHashKey]->removeAt(j);
     getRank()--;
   }

 if (L[0] <getRank())
   {

     for (int j=L[0]; j < getRank() +1 ; j++)
       {
         mainTabWidget->setTabText(j,  tags[1] + " " + QString::number(j+1));
       }
   }
}
#endif

void FListFrame::addDirectoryToListWidget(const QFileInfo& info, int filerank)
{
 QStringList filters;
 filters+="*";
 QDir dir=QDir(info.canonicalFilePath());
 foreach (QFileInfo file, dir.entryInfoList(filters,QDir::AllDirs | QDir::NoDotAndDotDot|QDir::Files))
   {
     if (file.isDir())
       {
         addDirectoryToListWidget(file, filerank);
       }
     else
       {
         QString path=file.canonicalFilePath();
         addStringToListWidget(path, filerank);
       }
   }
}


bool FListFrame::addStringToListWidget(QString filepath, int index)
{
  // normaly it should be useless to call updateIndexInfo() here
 updateIndexInfo();
 if ((filepath.isEmpty()) || (currentIndex >= (*hash::FStringListHash[frameHashKey]).count() ) || (signalList == NULL)) return false;

 fileListWidget->currentListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
 fileListWidget->currentListWidget->addItem(filepath);
 fileListWidget->currentListWidget->setCurrentRow(row+1);
 fileListWidget->currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

 (*hash::FStringListHash[frameHashKey])[currentIndex] << filepath;

 *(fileListWidget->signalList) << filepath;
 *signalList << filepath; //make a copy. Necessary to avoid losing dragged and dropped files to list widget directly.

 for (int j=1; index+j < cumulativePicCount.count() ;  j++)
   cumulativePicCount[index+j]++;

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
         const QFileInfo info=model->fileInfo(index);

         if (info.isFile())
           {
             QString filepath=info.canonicalFilePath();
             //fileListWidget->currentListWidget->addItem(filepath);
            addStringToListWidget(filepath, currentIndex);
           }
         else if (info.isDir())
           {
             addDirectoryToListWidget(info, currentIndex);
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

         //
         addStringToListWidget(name, currentIndex);
       }
  }
}

