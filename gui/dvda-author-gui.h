#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDialog>
#include <QProcess>
#include <QtGui>
#include <QMediaPlayer>
#include <QDomNode>

#include "common.h"
#include "flistframe.h"

#define PARAMETER_HTML_TAG "<img src=\":/images/configure.png\"  height=\"16\" width=\"16\"/> "
#define MSG_HTML_TAG "<img src=\":/images/msg.png\"/> "
#define ERROR_HTML_TAG "<img src=\":/images/error.png\"/> "
#define WARNING_HTML_TAG "<img src=\":/images/warning.png\"/> "
#define INFORMATION_HTML_TAG "<img src=\":/images/information.png\"/> "
#define NAVY_HTML_TAG "<span style=\"color: navy;\"> "


class QAction;
class QDirModel;
class QPushButton;
class QTreeView;

class QProcess;
class QCheckBox;
class QTextEdit;
class QListWidget;
class QTabWidget;
class QDomElement;
class QIODevice;
class QTreeWidget;
class QTreeWidgetItem;
class options;
class dvda;
class common;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public :
   MainWindow();
   options* dialog;
   QSettings  *settings;

   enum { MaxRecentFiles = 5 };
   QStringList recentFiles;
   void updateRecentFileActions();
   QString strippedName(const QString &fullFuleName);

  protected :

//   void dragEnterEvent(QDragEnterEvent *event);
//   void dropEvent(QDropEvent *event);

  private slots:

   void on_exitButton_clicked();
   void on_displayOutputButton_clicked();
   void on_displayFileTreeViewButton_clicked();
   void on_optionsButton_clicked();
   void showMainWidget();
   void about();

  private :

   bool readFile(const QString &fileName);
   dvda *dvda_author;

   void createActions();
   void createMenus();
   void createToolBars();
   void loadFile(const QString &fileName);

   QDockWidget* fileTreeViewDockWidget;
   QDockWidget* outputTextEditDockWidget;
   QMenu *fileMenu;
   QMenu *processMenu;
   QMenu *editMenu;
   QMenu *optionsMenu;
   QMenu *aboutMenu;

   QToolBar *fileToolBar;
   QToolBar *processToolBar;
   QToolBar *editToolBar;
   QToolBar *optionsToolBar;
   QToolBar *aboutToolBar;

   QAction *recentFileActions[MaxRecentFiles];
   QAction *separatorAction;
   QAction *openAction;
   QAction *saveAction;
   QAction *closeAction;
   QAction *burnAction;
   QAction *encodeAction;
   QAction *decodeAction;
   QAction *aboutAction;
   QAction *optionsAction;
   QAction *exitAction;
   QAction *helpAction;
   QAction *displayAction;
   QAction *displayManagerAction;
   QAction *displayConsoleAction;
   QAction *displayOutputAction;
   QAction *displayFileTreeViewAction;
   QAction *clearOutputTextAction;
   QList<QAction*>  actionList;

};

enum RefreshManagerFilter {
  NoCreate=0x0000,
  Create=0x0001,
  UpdateTree=0x0010,
  SaveTree=0x0100,
  UpdateTabs=0x1000,
  SaveAndUpdateTree=0x0110,
  RefreshAll=0x1010,
  CreateTreeAndRefreshAll=0x1011,
  CreateSaveAndUpdateTree=0x0111,
  UpdateOptionTabs=0x2000
};

class dvda : public common
{
    Q_OBJECT

public:

    dvda();

    void setCurrentFile(const QString &fileName);
    void setOutputTextEdit(QString filename);
    MainWindow *parent;
    enum { MaxRecentFiles = 5 };

    QFileSystemModel *model;
    QTabWidget  *mainTabWidget;
    QTreeWidget *managerWidget;
    QTreeView *fileTreeView;
    QString projectFile;
    QString projectName;
    QString curFile;
    QString groupType;
    unsigned int rank[2];
    FListFrame *project[2];

    QToolButton *audioFilterButton;

    int getZone() {return isVideo;}

//    void addDraggedFiles(QList<QUrl> urls);
    void initializeProject(const bool cleardata=true);
    void DomParser(QIODevice*);

private slots:

    void on_rightButton_clicked();
    void on_moveUpItemButton_clicked();
    void on_moveDownItemButton_clicked();
    void on_retrieveItemButton_clicked();
    void on_cdrecordButton_clicked();
    void on_openManagerWidgetButton_clicked();
    void on_openProjectButton_clicked();
    void on_clearOutputTextButton_clicked();
    void remove();
    void extract();
    void createDirectory();
    void run();
    void runLplex();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void process2Finished(int exitCode,  QProcess::ExitStatus exitStatus);
    void process3Finished(int exitCode,  QProcess::ExitStatus exitStatus);
    void addGroup();
    void addGroup(int,int);
    void deleteGroup();
    void killMkisofs();
    void killDvda();
    void killCdrecord();
    void on_helpButton_clicked();
    void requestSaveProject();
    void writeProjectFile();
    void assignGroupFiles(const int isVideo, uint group_index, qint64 size,  QString file);

    void openProjectFile();
    void on_playItemButton_clicked();
    void on_playItem_changed();
    void on_audioFilterButton_clicked(bool active);
    void on_displayConsoleButton_clicked();
    void closeProject();
    void feedConsole();
    void on_frameTab_changed(int index);

private:

    QIcon iconShowMaximized, iconShowNormal;
    QToolButton *mkdirButton, *removeButton, *moveUpItemButton,
                         *moveDownItemButton, *retrieveItemButton, *addGroupButton, *deleteGroupButton,
                         *killMkisofsButton, *killButton, *killCdrecordButton,*playItemButton;

    QProcess    process, process2, process3, helpProcess;
    QProgressBar *progress, *progress2, *progress3;
    QCheckBox  *debugFCheckBox;
    QVBoxLayout *mainLayout, *progressLayout, *managerLayout;
    QHBoxLayout *allLayout;
    bool startProgressBar, startProgressBar2, startProgressBar3;
    uint test;
    quint64 inputSizeCount;
    quint64 inputSize[2][99];
    QList<quint64> rowFileSize[2][99];
    qint64 inputTotalSize;
    qint64 value;
    int myTimerId;
    uint isVideo;
    uint currentIndex;
    int row;
    bool hasIndexChanged;
    QString tag;

    void showEvent(QShowEvent *event);
    void timerEvent(QTimerEvent *event);
    void hideEvent(QHideEvent *event);
    void initialize();
    void setIndexedProperties(QModelIndexList* indexList);
    void addSelectedFileToProject();
    uint addStringToListWidget(QString filepath);
    void showFilenameOnly();
    void addDirectoryToListWidget(QDir dir);
    void updateIndexInfo();
    bool refreshProjectManager();
    float discShare(qint64 directorySize);
    void setDialogFromProject();
    void clearProjectData();
    QList<QStringList> processSecondLevelData(QList<QStringList> &L);
    void parseEntry(const QDomNode &, QTreeWidgetItem *parent);
    void refreshRowPresentation();
    void refreshRowPresentation(uint, uint);
    void updateIndexChangeInfo();

    QString  makeDataString( );
    QString  makeSystemString( );
    void assignVariables(FStringList &value);
    QStringList createCommandLineString(int commandLineType);

    QTextEdit *console;
    QDialog *consoleDialog;

    QMediaPlayer *myMusic;

 protected:

    QString     outputType, sourceDir;
    unsigned int maxRange;
    static RefreshManagerFilter  RefreshFlag;

signals:

  void clearOptionData();
  void hasIndexChangedSignal();
  void is_signalList_changed(int);

 public slots:

    void saveProject();

};



#endif

