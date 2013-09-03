#ifndef DVDA_H
#define DVDA_H

#include "fstring.h"
#include "dvda-author-gui.h"
#include "flistframe.h"
#include "stream_decoder.h"
#include "probe.h"

class MainWindow;



class dvda : public common
{
    Q_OBJECT


public:

    dvda();

    void setCurrentFile(const QString &fileName);

    MainWindow *parent;
    enum { MaxRecentFiles = 5 };
    static int RefreshFlag;
    static int dialVolume;

    QFileSystemModel *model=new QFileSystemModel;
    QTabWidget  *mainTabWidget= new QTabWidget;
    QTreeWidget *managerWidget= new QTreeWidget;
    QTreeView *fileTreeView= new QTreeView;
    QString projectName;
    QString curFile;
    FListFrame *project[2];
    QToolButton *audioFilterButton=new QToolButton;
    QTextEdit *outputTextEdit = new QTextEdit;

    int getZone() {return isVideo;}
    void initializeProject(const bool cleardata=true);

    void checkEmptyProjectName()
      {
         if (projectName.isEmpty())
            projectName=QDir::currentPath() + "/default.dvp";
      }

    //    void addDraggedFiles(QList<QUrl> urls);

    void startDrag();
    void addDraggedFiles(const QList<QUrl>& urls);
 /*   void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event)*/
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
     QPoint startPos;
     QProcess process;


public slots:

   void updateProject(bool=false);
   void on_openManagerWidgetButton_clicked(bool );
   void on_frameTab_changed(int index);
   void on_openProjectButton_clicked();
   void on_playItemButton_clicked(bool =false);

private slots:

    void on_openManagerWidgetButton_clicked();
    void on_moveUpItemButton_clicked();
    void on_moveDownItemButton_clicked();
    void on_deleteItem_clicked();
    void on_cdrecordButton_clicked();
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
    void assignGroupFiles(const int isVideo, const int group_index, const QString& file);
    void openProjectFile();

    void on_playItem_changed();
    void on_audioFilterButton_clicked(bool active);
    void closeProject();
    void checkStandardCompliance();
    void deleteSonicVisualiserProcess(int);


private:

    bool hasIndexChanged;
    bool startProgressBar=0;
    bool startProgressBar2=0;
    bool startProgressBar3=0;
    int myTimerId=0;
    int row=0;
    uint isVideo=AUDIO;
    uint currentIndex=0;
    qint64 value=0;
    static qint64 totalSize[2];

    QDial* dial=new QDial;

    QString  zoneTag(int ZONE){return ((ZONE)? "DVD-V" : "DVD-A");}
    QString  zoneTag(){return ((mainTabWidget->currentIndex())? "DVD-V" : "DVD-A");}
    QString  zoneGroupLabel(int ZONE){return ((ZONE)? "titleset" : "group");}
    QString  zoneGroupLabel(){return ((mainTabWidget->currentIndex())? "titleset" : "group");}

    QHash <int,  QList<QStringList>  > fileSizeDataBase;

    QIcon iconShowMaximized, iconShowNormal;
    QMediaPlayer *myMusic=nullptr;
    QProcess   process2, process3;
    QProgressBar *progress=new QProgressBar, *progress2=nullptr, *progress3=nullptr;
    QToolButton *mkdirButton= new QToolButton;
    QToolButton *removeButton= new QToolButton;
    QToolButton *killMkisofsButton= new QToolButton;
    QToolButton *killButton= new QToolButton;
    QToolButton *killCdrecordButton= new QToolButton;
    QToolButton *playItemButton= new QToolButton;
    //QTextEdit     *console  = new QTextEdit;

    QVBoxLayout *mainLayout= new QVBoxLayout;
    QVBoxLayout *progressLayout= new QVBoxLayout;
    QVBoxLayout *managerLayout= new QVBoxLayout;
    QHBoxLayout *allLayout= new QHBoxLayout;

    void assignVariables();
    void clearProjectData();
    QStringList createCommandLineString(int commandLineType);
    float discShare(qint64 directorySize);
    void hideEvent(QHideEvent *event);
    void initialize();
    const QString  makeParserString(int start, int end=Abstract::abstractWidgetList.size()-1);
    const QString  makeDataString( );
    const QString  makeSystemString( );
    QList<QStringList> processSecondLevelData(QList<QStringList> &L, bool isFile=true);
    FStringList parseEntry(const QDomNode &, QTreeWidgetItem *parent=0);

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

    void DomParser(QIODevice*);
    void refreshProjectManagerValues(int= refreshAllZones );
    bool refreshProjectManager();
    void msg (const QString & text);

    StandardComplianceProbe  *probe;
    QProcess *sonicVisualiserProcess=nullptr;

 protected:

    QString     outputType, sourceDir;
    unsigned int maxRange=0;

signals:

  void hasIndexChangedSignal();
  void is_signalList_changed(int);


};

#endif // DVDA_H
