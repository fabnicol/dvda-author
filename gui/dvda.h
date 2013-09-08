#ifndef DVDA_H
#define DVDA_H

#include "fstring.h"
#include "dvda-author-gui.h"
#include "flistframe.h"
#include "stream_decoder.h"
#include "probe.h"


class MainWindow;
class FProgressBar;



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
    QPushButton *audioFilterButton=new QPushButton;
    QTextEdit *outputTextEdit = new QTextEdit;

    int getZone() {return isVideo;}
    void initializeProject(const bool cleardata=true);
    int resample(int,int);

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
     QProcess ejectProcess;
     WavFile wavFile;


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
    void runCdrecord();
    void remove();
    void extract();
    void createDirectory();
    void run();
    void runMkisofs();
    void runLplex();
    void processFinished(int exitCode);
    void addGroup();
    void deleteGroup();
    void killProcess();
    void killCdrecord();

    void on_helpButton_clicked();
    void requestSaveProject();
    void writeProjectFile();
    void assignGroupFiles(const int isVideo, const int group_index, const QString& file);
    void openProjectFile();

    void on_playItem_changed();
    void on_audioFilterButton_clicked(bool active);
    void closeProject();
    inline int checkStandardCompliance();
    void deleteSonicVisualiserProcess();


private:

    bool hasIndexChanged;

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
    FProgressBar *progress, *progress2, *progress3;
    QToolButton *mkdirButton= new QToolButton;
    QToolButton *removeButton= new QToolButton;
    QToolButton *playItemButton= new QToolButton;

    QVBoxLayout *mainLayout= new QVBoxLayout;
    QVBoxLayout *progressLayout= new QVBoxLayout;
    QVBoxLayout *managerLayout= new QVBoxLayout;
    QHBoxLayout *allLayout= new QHBoxLayout;

    void assignVariables();
    void clearProjectData();
    QStringList createCommandLineString(int commandLineType);
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
    void showFilenameOnly();
    void updateIndexInfo();
    void updateIndexChangeInfo();
    void displayTotalSize();

    void DomParser(QIODevice*);
    void refreshProjectManagerValues(int= refreshAllZones );
    bool refreshProjectManager();
    void msg (const QString & text);

    StandardComplianceProbe  *probe;
    QProcess *sonicVisualiserProcess=nullptr;

    QProcess resampleProcess;
    int applyFunctionToSelectedFiles(int (dvda::*f)( )) ;
    inline int resample();
    void printDiscSize(qint64 new_value);
    void printMsg(qint64 new_value, const QString &str);
    void printFileSize(qint64 new_value);
    void printBurnProcess(qint64 new_value);
   qint64 getCdrecordProcessedOutput(const QString& ="", const QString& ="");
   void resetCdRecordProcessedOutput();



 protected:

    QString     outputType, sourceDir;
    unsigned int maxRange=0;

signals:

  void hasIndexChangedSignal();
  void is_signalList_changed(int);


};


class FProgressBar : public QWidget
{
   Q_OBJECT

    typedef  qint64 (dvda::*MeasureFunction)(const QString &, const QString &) ;
    typedef void (dvda::*DisplayFunction)(qint64 );
    typedef void (dvda::*SlotFunction)();

public:
    FProgressBar(dvda* parent,
                                     MeasureFunction measureFunction,
                                     DisplayFunction displayMessageWhileProcessing,
                                     SlotFunction  killFunction=nullptr,
                                     const QString & fileExtensionFilter="*.AOB",
                                     const QString&  measurableTarget="",
                                     const qint64 referenceSize=1);

    QHBoxLayout* layout=new QHBoxLayout;

    void show()
    {
        start();
        bar->reset();
        killButton->show();
        bar->show();
    }

    void start(int timeout=0)
    {
        timer->start(timeout);
        killButton->setEnabled(true);
    }

    void stop()
    {
        if (parent->process.state() == QProcess::Running) return;
        timer->stop();
        killButton->setDisabled(true);
    }

    void hide()
    {
        stop();
        bar->hide();
        killButton->hide();
        bar->reset();
    }

    void setToolTip(const QString & tip) { bar->setToolTip(tip); }
    void setTarget(const QString&  t) { target=t; }
    void setReference(qint64  r) { reference=r; }
    qint64 updateProgressBar();

 private:
    QToolButton* killButton=new QToolButton;
    QString   target;
    QString   filter;
    qint64 reference;
    QTimer *timer= new QTimer(this);
    QProgressBar *bar=new QProgressBar ;
    qint64 new_value=0;
    dvda* parent;

    MeasureFunction engine ;

};


#endif // DVDA_H
