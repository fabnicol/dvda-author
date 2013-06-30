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
 FListWidget* fileListWidget;
 QString frameHashKey;

 void startDrag();
 QPoint startPos;
 int row, currentIndex,  slotListSize;

public:

 QToolButton *importFromMainTree,
                        *moveDownItemButton,
                        *moveUpItemButton,
                        *retrieveItemButton,
                        *clearList,
                        *addGroupButton,
                        *deleteGroupButton;

 QTabWidget* mainTabWidget, *embeddingTabWidget;
 QAbstractItemView *fileTreeView;
 QStringList* slotList;
 QList<int> cumulativePicCount;
 QLabel* fileLabel;
 QString fileLabelText;
 QStringList *signalList;
 QFileSystemModel *model;
 QGroupBox *controlButtonBox, *tabBox;


 /* accessors */
 int getRank() {return widgetContainer.count()-1;}
 QString &getHashKey() {return frameHashKey;}

 void initializeWidgetContainer()
 {
    widgetContainer = QList<QListWidget*>() << fileListWidget->currentListWidget;
 }
 void clearWidgetContainer()
 {
    widgetContainer.clear(); ;
 }

inline QList<QListWidget*>  getWidgetContainer() {return widgetContainer;}
inline QListWidget*  getWidgetContainer(int rank) {if (rank < widgetContainer.count()) return widgetContainer[rank]; else return nullptr;}
inline QListWidget*  getCurrentWidget() {return widgetContainer[this->mainTabWidget->currentIndex()];}
inline int getCurrentIndex() {return this->mainTabWidget->currentIndex();}
inline int getCurrentRow() {return getCurrentWidget()->currentRow();}

void addDirectoryToListWidget(const QFileInfo&, int);
void addNewTab();
bool addStringToListWidget(QString , int );


FListFrame(QObject* parent,  QAbstractItemView * fileTreeView, short import_type, const QString &hashKey,
            const QString &description, const QString &command_line, int commandLineType, const QStringList &separator, const QStringList &xml_tags,
            int mainTabWidgetRank=-1, QIcon* icon=NULL, QTabWidget* parentTabWidget=NULL,
           QStringList* terms=NULL, QStringList* translation=NULL, QStringList* slotL=NULL);


public slots:
    void addGroup();
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
