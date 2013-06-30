#ifndef FLISTFRAME_H
#define FLISTFRAME_H

#include "fwidgets.h"
#include "fstring.h"
#include <QToolButton>
#include <QFileSystemModel>


class FListFrame : public QWidget//: public common
{
Q_OBJECT


private:
  inline void updateIndexInfo();
#if 0
  void deleteGroups(QList<int> &L);
#endif

 QList<QListWidget*> widgetContainer;
 QString frameHashKey;

 void startDrag();
 QPoint startPos;

public:
QToolButton *importFromMainTree,  *moveDownItemButton, *moveUpItemButton, *retrieveItemButton, *clearList;

QAbstractItemView *fileTreeView;
QStringList* slotList;

QList<int> cumulativePicCount;
int slotListSize;
int getRank() {return fileListWidget->rank;}
void setRank(int r) {fileListWidget->rank=r;}
void decrementRank() {fileListWidget->rank--;}
void incrementRank() {fileListWidget->rank++;}

QString &getHashKey() {return frameHashKey;}

void addDirectoryToListWidget(const QFileInfo&, int);

int row, currentIndex;

bool addStringToListWidget(QString , int );

void initializeWidgetContainer()
{
    widgetContainer = QList<QListWidget*>() << fileListWidget->currentListWidget;
    fileListWidget->rank=0;
}

void clearWidgetContainer()
{
    widgetContainer.clear(); ;
}

QList<QListWidget*>  getWidgetContainer() {return widgetContainer;}

QTabWidget* mainTabWidget, *embeddingTabWidget;
QToolButton *addGroupButton, *deleteGroupButton;

FListWidget* fileListWidget;

FListFrame(QObject* parent,  QAbstractItemView * fileTreeView, short import_type, const QString &hashKey,
            const QString &description, const QString &command_line, int commandLineType, const QStringList &separator, const QStringList &xml_tags,
            int mainTabWidgetRank=-1, QIcon* icon=NULL, QTabWidget* parentTabWidget=NULL,
           QStringList* terms=NULL, QStringList* translation=NULL, QStringList* slotL=NULL);

QLabel* fileLabel;
QString fileLabelText;
QStringList *signalList;
QFileSystemModel *model;
QGroupBox *controlButtonBox, *tabBox;

public slots:
void addGroup(bool force=false);
void deleteGroup();
void on_retrieveItemButton_clicked();
 void on_clearList_clicked(int currentIndex=-1);

protected slots:

void on_importFromMainTree_clicked();
void on_moveDownItemButton_clicked();
void on_moveUpItemButton_clicked();

void addGroups(int);

void on_mainTabIndex_changed(int =0);
void on_embeddingTabIndex_changed(int =0);

protected:
short importType;
QStringList tags;
void addDraggedFiles(QList<QUrl> urls);
void mousePressEvent(QMouseEvent *event);
void mouseMoveEvent(QMouseEvent *event);
void dragMoveEvent(QDragMoveEvent *event);
void dragEnterEvent(QDragEnterEvent *event);
void dropEvent(QDropEvent *event);

signals:
void is_signalList_changed(int);

};

inline void FListFrame::updateIndexInfo()
{
  fileListWidget->currentListWidget=qobject_cast<QListWidget*>(mainTabWidget->currentWidget());
  if (fileListWidget->currentListWidget == NULL) return;
  row=fileListWidget->currentListWidget->currentRow();
  currentIndex=mainTabWidget->currentIndex();
}


#endif // FLISTFRAME_H
