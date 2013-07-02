#ifndef DVDA_H
#define DVDA_H

#include "fstring.h"
#include "dvda-author-gui.h"
#include "flistframe.h"

class MainWindow;

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
    QString projectName;
    QString curFile;
    QString groupType;
    FListFrame *project[2];

    QToolButton *audioFilterButton;

    int getZone() {return isVideo;}

//    void addDraggedFiles(QList<QUrl> urls);
    void initializeProject(const bool cleardata=true);
    void DomParser(QIODevice*);

public slots:

   void saveProject();
   void on_openManagerWidgetButton_clicked(bool );
   void on_frameTab_changed(int index);
   void on_openProjectButton_clicked();

private slots:

    void on_openManagerWidgetButton_clicked();
    void on_importFromMainTree_clicked();
    void on_moveUpItemButton_clicked();
    void on_moveDownItemButton_clicked();
    void on_deleteItem_clicked();
    void on_cdrecordButton_clicked();
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
    void deleteGroup();
    void killMkisofs();
    void killDvda();
    void killCdrecord();
    void on_helpButton_clicked();
    void requestSaveProject();
    void writeProjectFile();
    void assignGroupFiles(const int isVideo, const int group_index, QString size,  QString file);
    void openProjectFile();
    void on_playItemButton_clicked();
    void on_playItem_changed();
    void on_audioFilterButton_clicked(bool active);
    void on_displayConsoleButton_clicked();
    void closeProject();
    void feedConsole();


private:

    bool hasIndexChanged;
    bool startProgressBar;
    bool startProgressBar2;
    bool startProgressBar3;
    int myTimerId;
    int row;
    uint isVideo;
    uint currentIndex;
    uint test;
    qint64 value;
    static qint64 totalSize;

    QString tag;

    QList<FStringList> xmlDataWrapper;
    QHash <int,  QList<QStringList>  > fileSizeDataBase;

    QDialog *consoleDialog;
    QIcon iconShowMaximized, iconShowNormal;
    QMediaPlayer *myMusic;
    QProcess    process, process2, process3, helpProcess;
    QProgressBar *progress, *progress2, *progress3;
    QToolButton *mkdirButton, *removeButton, *moveUpItemButton,
                         *moveDownItemButton, *retrieveItemButton, *addGroupButton, *deleteGroupButton,
                         *killMkisofsButton, *killButton, *killCdrecordButton,*playItemButton;
    QTextEdit *console;

    QVBoxLayout *mainLayout, *progressLayout, *managerLayout;
    QHBoxLayout *allLayout;

    void addSelectedFileToProject();
    void addDirectoryToListWidget(QDir dir);
    void addStringToListWidget();
    void assignVariables(const QList<FStringList> &value);
    void clearProjectData();
    QStringList createCommandLineString(int commandLineType);
    float discShare(qint64 directorySize);
    void hideEvent(QHideEvent *event);
    void initialize();
    QString makeParserString(int start, int end=Abstract::abstractWidgetList.size()-1);
    QString  makeDataString( );
    QString  makeSystemString( );
    QList<QStringList> processSecondLevelData(QList<QStringList> &L, bool isFile=true);
    FStringList parseEntry(const QDomNode &, QTreeWidgetItem *parent);
    void parseXmlNodes(const QDomNode &node, const QString &maintag);
    bool refreshProjectManager();
    void refreshRowPresentation();
    void refreshRowPresentation(uint, uint);
    void setIndexedProperties(QModelIndexList* indexList);
    void setDialogFromProject();
    void showEvent(QShowEvent *event);
    void showFilenameOnly();
    void timerEvent(QTimerEvent *event);
    void updateIndexInfo();
    void updateIndexChangeInfo();
    void displayTotalSize();

 protected:

    QString     outputType, sourceDir;
    unsigned int maxRange;
    static RefreshManagerFilter  RefreshFlag;

signals:

  void clearOptionData();
  void hasIndexChangedSignal();
  void is_signalList_changed(int);


};

#endif // DVDA_H
