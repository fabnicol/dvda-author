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

  int row, currentIndex;

bool addStringToListWidget(QString , int );

QTabWidget* mainTabWidget, *embeddingTabWidget;
QToolButton *addGroupButton, *deleteGroupButton;

FListWidget* fileListWidget;

FListFrame(QObject* parent,  QAbstractItemView * fileTreeView, short import_type, const QString &hashKey,
            const QString &description, const QString &command_line, int commandLineType, const QStringList &separator, const QStringList &xml_tags,
            int mainTabWidgetRank=-1, QIcon* icon=NULL, QTabWidget* parentTabWidget=NULL,
           QStringList* terms=NULL, QStringList* translation=NULL, QStringList* slotL=NULL);
//,           QWidget* controlledWidget=NULL);


QLabel* fileLabel;
QString fileLabelText;
QStringList *signalList;
QFileSystemModel *model;
QGroupBox *controlButtonBox, *tabBox;

public slots:
void addGroup();
void deleteGroup();
void on_retrieveItemButton_clicked();
 void on_clearList_clicked();

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

public:
QToolButton *importFromMainTree,  *moveDownItemButton, *moveUpItemButton, *retrieveItemButton, *clearList;

QAbstractItemView *fileTreeView;
QStringList* slotList;

QList<int> cumulativePicCount;
int slotListSize;

QString &hashKey() {return frameHashKey;}

void addDirectoryToListWidget(QDir&, int);

signals:
void is_signalList_changed(int);

};

#endif // FLISTFRAME_H
