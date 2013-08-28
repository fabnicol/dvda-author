
#include "browser.h"

browser::browser(const QUrl &urlPath,  QWidget *parent) :  QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);

    url=urlPath;

    homeButton->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    homeButton->setToolTip(tr("Home"));
    backButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    backButton->setToolTip(tr("Back (Ctrl + <)"));
    backButton->setShortcut(QKeySequence("Ctrl+<"));

    forwardButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    forwardButton->setToolTip(tr("Forward (Ctrl + >)"));
    forwardButton->setShortcut(QKeySequence("Ctrl+>"));

    closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    closeButton->setToolTip(tr("Close (Ctrl + Q)"));
    closeButton->setShortcut(QKeySequence("Ctrl+Q"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(homeButton);
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(forwardButton);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(textBrowser);
    setLayout(mainLayout);

    connect(homeButton, SIGNAL(clicked()), this, SLOT(home()));
    connect(backButton, SIGNAL(clicked()), textBrowser, SLOT(back()));
    connect(forwardButton, SIGNAL(clicked()), textBrowser, SLOT(forward()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(textBrowser, SIGNAL(urlChanged(const QUrl &)), this, SLOT(updateWindowTitle()));

    textBrowser->load(url);
    const QIcon dvdaIcon=QIcon(QString::fromUtf8( ":/images/dvda-author.png"));
    setWindowIcon(dvdaIcon);

}

void browser::home()
{
    textBrowser->load(url);
}

void browser::updateWindowTitle()
{
    setWindowTitle(textBrowser->url().toString());
}

void browser::showPage(const QUrl &url)
{
    browser *app = new browser(url);
    app->resize(1000, 500);
    app->show();
}



#ifndef BROWSER_H
#define BROWSER_H

#include <QtWidgets>
#include <QtWebKitWidgets>
#include <QWebView>
#include "common.h"

class QToolButton;
class QWebView;


class browser : public QWidget
{
  Q_OBJECT
public:
   browser(const QUrl & url,  QWidget *parent = 0);
   static void showPage(const QUrl &url);

 private:
   QWebView *textBrowser  = new QWebView;
   QToolButton *homeButton = new QToolButton ;
   QToolButton *backButton = new QToolButton ;
   QToolButton *forwardButton = new QToolButton ;
   QToolButton *closeButton =new QToolButton;
   QUrl url;

public slots:
   void updateWindowTitle();
   void home();

};

#endif // BROWSER_H
#include "common.h"


QStringList common::extraAudioFilters=QStringList();
FString common::htmlLogPath;
QString common::tempdir=QDir::homePath ()+QDir::separator()+"tempdir";  // should be equal to main app globals.settings.tempdir=TEMPDIR;



bool common::remove(const QString &path)
{
    if (QFileInfo(path).isDir())
    {
        return  removeDirectory(path) ;
    }
    return false;
}

bool common::removeDirectory(const QString &path)
{
    if (path.isEmpty()) return false;

    QDir dir(path);

    foreach (QFileInfo fileinfo, dir.entryInfoList(QDir::AllEntries|QDir::System|QDir::Hidden))
    {
        if (fileinfo.fileName() == "." || fileinfo.fileName() == "..") continue;


        if (fileinfo.isFile()||fileinfo.isSymLink())
            dir.remove(fileinfo.absoluteFilePath());
        else
            if (fileinfo.isDir())
            {
                QString p;
                if (dir.rmpath(p=fileinfo.absoluteFilePath()) == false)
                    removeDirectory(p);
            }
    }

    dir.rmdir(path);
    return true;

}

qint64 common::recursiveDirectorySize(const QString &path, const QString &extension)
{

    QDir dir(path);
    qint64 size=0;

    QStringList filters;
    filters+=extension;

    foreach (QString file, dir.entryList(filters, QDir::Files))
        size+=QFileInfo(dir, file).size();

    foreach (QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        size+=recursiveDirectorySize(path+QDir::separator()+subDir, extension);

    return size;

}

void common::writeFile(QString & path, const QStringList &list, QFlags<QIODevice::OpenModeFlag> flag)
{
    QFile data(path);
    if (data.open(flag))
    {
        QTextStream out(&data);
        QStringListIterator i(list);
        while (i.hasNext())
            out << i.next() << "\n";
    }
    data.close();
}


int common::readFile(QString &path, QStringList &list, int start, int stop, int width)
{
QFile file(path);
int j=0;
if (file.open(QIODevice::ReadOnly | QIODevice::Text))
{
  QTextStream in(&file);
  in.seek(0);
  while (++j < start)  in.readLine();
  while (!in.atEnd() )
  {
      QString line = in.readLine(width);
      list << line;
      if  (j == stop) break;
      j++;
  }
}
else QMessageBox::warning(this, tr("Warning"), tr("WhatsThis file could not be opened: ") + path );
file.close();
return j;

}

QString common::readFile(QString &path,  int start, int stop, int width)
{
QFile file(path);
QStringList L=QStringList();
readFile(path, L, start, stop, width);
QString string=L.join("\n");
return string;
}

QString common::generateDatadirPath(QString &path)
{
  QString pathstr= QDir::cleanPath(  QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + path);
  return pathstr;
}

QString common::generateDatadirPath(const char* path)
{
  const QString str= QString(path);
  QString pathstr= QDir::cleanPath(  QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + str);
  return pathstr;
}

void common::setWhatsThisText(QWidget* widget, int start, int stop)
{
  widget->setWhatsThis("<html>"+readFile(whatsThisPath, 2, 2)+readFile(whatsThisPath, start, stop)+"</html>");
}

void common::openDir(QString path)
{
   if (path.isEmpty()) return;
  if (!QFileInfo(path).isDir())
    {
      QMessageBox::warning(this, "", path + " is not a directory");
      return;
    }

QUrl url("file:///" + path);
QDesktopServices::openUrl(url);
}

// dynamic allocation is obligatory
//  ImageViewer *v = new ImageViewer(videoMenuLineEdit->setXmlFromWidget());
//  v->show();



bool common::checkVideoStandardCompliance(QString &filename)
{
  return true;
  //return  (fileinfo->compliance == isDVDVideoCompliant);
}

bool common::checkAudioStandardCompliance(QString &filename)
{
  return true;
  //return (fileinfo->compliance == isDVDAudioCompliant);
}

void common::getAudioCharacteristics(QString &filename)
{
  fileinfo = new fileinfo_t;
  fileinfo->filename=filename.toLocal8Bit();

  /* filepath is non-locale char compliant, so that dvda-author cannot be used for input filename format reasons */
  if (QString(fileinfo->filename) != filename) fileinfo->compliance=isNonCompliant;


  QProcess process;
  QStringList args=QStringList() << "--test" << filename;
  process.start("dvda-author", args);
  connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(assignAudioCharacteristics(int, QProcess::ExitStatus )));

  /*  */

}

void common::assignAudioCharacterisics(int exitcode, QProcess::ExitStatus status)
{

  /* ecode dvda-author --test filename exit status as : channels << 4 [byte 1] | samplerate << 1 [byte 2-4] | bitspersample [byte 5] */

  if (status != QProcess::NormalExit) return;
  fileinfo->bitspersample = exitcode & 0xFF;
  fileinfo->samplerate = (exitcode >> 1) & 0xFFFFFF;
  fileinfo->channels = (exitcode >> 4) & 0xFF;
  if (((fileinfo->channels ==0) || (fileinfo->samplerate == 0) || (fileinfo->bitspersample == 0)) ||
      (fileinfo->channels > 6) ||
      ((fileinfo->bitspersample != 16) || (fileinfo->bitspersample != 24)))
    {
      fileinfo->compliance=isNonCompliant;
      return;
    }


  if ((fileinfo->samplerate = 96000) || (fileinfo->samplerate == 48000) || (fileinfo->samplerate == 44100) || (fileinfo->samplerate == 88200) || (fileinfo->samplerate == 176400) || (fileinfo->samplerate == 192000))
         fileinfo->compliance=isDVDAudioCompliant;
  else
    if ((fileinfo->samplerate == 96000) || (fileinfo->samplerate == 48000))
       fileinfo->compliance=isDVDVideoCompliant;
  else
    fileinfo->compliance=isNonCompliant;

  return;
}
#ifndef COMMON_H
#define COMMON_H

#include <QtWidgets>

#include "fwidgets.h"


#define AFMT_WAVE 1
#define AFMT_FLAC 2
#define AFMT_OGG_FLAC 3
#define NO_AFMT_FOUND 4
#define AFMT_WAVE_GOOD_HEADER 10
#define AFMT_WAVE_FIXED 11

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifndef VERSION
#define VERSION "Crystal echo"
#endif

#define VIDEO 1
#define AUDIO 0


#define Max(X,Y) ((X>Y)? X : Y)
#define Q(X) QMessageBox::about(NULL, "", X);
#define q(X) QMessageBox::about(NULL, "", QString::number(X));
#define v(X) *FString(#X)



class common : public QDialog, public flags
{
  Q_OBJECT

 private:
    QString whatsThisPath;

protected slots:
    void assignAudioCharacterisics(int exitcode, QProcess::ExitStatus status);

public:

  common()   {    whatsThisPath=generateDatadirPath("whatsthis.info");  }

  static QString tempdir;
  static QString generateDatadirPath(const char* path);
  static QString generateDatadirPath(QString &path);

  qint64 recursiveDirectorySize(const QString &path, const QString &extension);

  bool removeDirectory(const QString &path);
  bool remove(const QString &path);

  int readFile(QString &path, QStringList &list, int start=0, int stop=-1, int width=0);
  int readFile(const char* path, QStringList &list, int start=0, int stop=-1, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, list, start, stop, width);
  }
  QString readFile(QString &path,  int start=0, int stop=-1, int width=0);
  QString readFile(const char* path,  int start=0, int stop=-1, int width=0)
  {
    QString pathstr=QString(path);
    return readFile(pathstr, start, stop, width);
  }
static void writeFile(QString & path, const QStringList &list, QFlags<QIODevice::OpenModeFlag> flag= QFile::WriteOnly | QFile::Truncate) ;
void setWhatsThisText(QWidget* widget, int start, int stop);
void openDir(QString path);


protected :
  QString  videoFilePath;
  static FString    htmlLogPath;
  static QStringList extraAudioFilters;
  enum audioCharacteristics   { isWav=AFMT_WAVE, isFlac=AFMT_FLAC, isOggFlac=AFMT_OGG_FLAC, isDVDAudioCompliant,  isDVDVideoCompliant, isNonCompliant};
  struct fileinfo_t
  {
     quint8 header_size;
     quint8 type;
     quint8 bitspersample;
     quint8 channels;
     audioCharacteristics compliance;
     quint32 samplerate;
     quint64 numsamples;
     quint64 numbytes; // theoretical file size
     quint64 file_size; // file size on disc
     QByteArray filename;
  };

  fileinfo_t *fileinfo;

  bool checkVideoStandardCompliance(QString &filename);
  bool checkAudioStandardCompliance(QString &filename);
  void getAudioCharacteristics(QString &filename);



};

#endif // COMMON_H
#include "console.h"

Console::Console(MainWindow* p): QDialog(0)
      {
            QGridLayout* consoleLayout=new QGridLayout;
            hide();
            setSizeGripEnabled(true);
            setWindowTitle("Console");
            setMinimumSize(800,600);
            QToolButton *closeConsoleButton=new QToolButton;
            closeConsoleButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
            closeConsoleButton->setToolTip(tr("Close (Ctrl + Q)"));
            closeConsoleButton->setShortcut(QKeySequence("Ctrl+Q"));
            const QIcon clearOutputText = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
            QToolButton *clearConsoleButton=new QToolButton;
            clearConsoleButton->setIcon(clearOutputText);
            clearConsoleButton->setShortcut(QKeySequence("Ctrl+N"));
            clearConsoleButton->setToolTip(tr("Clear console (Ctrl + N)"));
            consoleLayout->addWidget(textWidget,0,0);
            consoleLayout->addWidget(closeConsoleButton, 1,0,Qt::AlignRight);
            consoleLayout->addWidget(clearConsoleButton, 2,0,Qt::AlignRight);
            setLayout(consoleLayout);
            // [=] not [&]
            connect(closeConsoleButton, &QToolButton::clicked, [=]{on_displayConsoleButton_clicked(p);});
            connect(clearConsoleButton, &QToolButton::clicked, [this]{textWidget->clear();});
     }


void Console::detachConsole(bool isDetached, MainWindow* parent)
{
    if (isDetached)
    {
        hide();
        parent->bottomTabWidget->addTab(parent->consoleDialog, tr("Console"));
        parent->bottomTabWidget->setCurrentIndex(1);
    }
    else
    {
        show();
        parent->bottomTabWidget->removeTab(1);
    }
}

void Console::on_displayConsoleButton_clicked(MainWindow* parent)
    {
        static bool isDetached;
        detachConsole(isDetached, parent);
        isDetached=!isDetached;
    }

#ifndef CONSOLE_H
#define CONSOLE_H
#include <QtWidgets>
#include <QObject>

#include "dvda-author-gui.h"

class MainWindow;

class Console : public QDialog
{

  public:
    Console(MainWindow* ) ;
    void detachConsole(bool, MainWindow*);
    void on_displayConsoleButton_clicked(MainWindow*);
    void appendHtml(const QString &s) const {textWidget->insertHtml(s); textWidget->moveCursor(QTextCursor::End);}

  private:
    QTextEdit* textWidget=new QTextEdit;

};

#endif // CONSOLE_H
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QProcess>
#include <QMediaPlayer>
#include <QDomNode>
#include <QFile>
#include <QtXml>

#include "options.h"
#include "browser.h"
#include "common.h"
#include "highlighter.h"
#include "enums.h"
#include "console.h"

#define PARAMETER_HTML_TAG "<img src=\":/images/configure.png\"  height=\"16\" width=\"16\"/> "
#define MSG_HTML_TAG "<img src=\":/images/msg.png\"/> "
#define ERROR_HTML_TAG "<img src=\":/images/error.png\"/> "
#define WARNING_HTML_TAG "<img src=\":/images/warning.png\"/> "
#define INFORMATION_HTML_TAG "<img src=\":/images/information.png\"/> "
#define HTML_TAG(X) "<span style=\"color: " #X ";\"> "



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
class Console;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public :
   MainWindow(char*);
   options* dialog;
   QSettings  *settings;

   enum { MaxRecentFiles = 5 };
   QStringList recentFiles;
   void updateRecentFileActions();
   QString strippedName(const QString &fullFuleName);
   void on_clearOutputTextButton_clicked();
   FCheckBox *defaultSaveProjectBehavior;
   QTabWidget *bottomTabWidget;
   QTextEdit *consoleDialog;

  private slots:

   void on_displayFileTreeViewButton_clicked(bool);
   void on_displayFileTreeViewButton_clicked();
   void on_editProjectButton_clicked();
   void on_optionsButton_clicked();
   void showMainWidget();
   void showMainWidget(bool);
   void configure();
   void configureOptions();
   void on_activate_lplex(bool);

  private :

   QHash<QString, QAction*> actionHash;
   void feedConsole(bool);
   bool readFile(const QString &fileName);
   dvda *dvda_author;
   QMainWindow *editWidget;
   QTimer *timer;
   void createActions();
   void createMenus();
   void createToolBars();
   void createFontDataBase();
   void loadFile(const QString &fileName);
   void saveProjectAs(QFile* file);

   QDockWidget* fileTreeViewDockWidget;
   QDockWidget* bottomDockWidget;
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
   QAction *saveAsAction;
   QAction *saveAction;
   QAction *closeAction;
   QAction *burnAction;
   QAction *configureAction;
   QAction *encodeAction;
   QAction *decodeAction;
   QAction *aboutAction;
   QAction *optionsAction;
   QAction *exitAction;
   QAction *helpAction;
   QAction *displayAction;
   QAction *displayManagerAction;
   QAction *displayConsoleAction;
   QAction *editProjectAction;
   QAction *displayOutputAction;
   QAction *displayFileTreeViewAction;
   QAction *clearOutputTextAction;
   QList<QAction*>  actionList;

   QDialog *contentsWidget;
   QDialogButtonBox *closeButton;

   FCheckBox *defaultFullScreenLayout,
                        *defaultLplexActivation,
                        *defaultConsoleLayoutBox,
                        *defaultProjectManagerWidgetLayoutBox,
                        *defaultFileManagerWidgetLayoutBox,
                        *defaultMessageLayoutBox,
                        *defaultOutputTextEditBox,
                        *defaultLoadProjectBehavior;

   QList<FCheckBox*> displayWidgetList, behaviorWidgetList;
   QTextEdit *editor;
   Highlighter *highlighter;
   Console *console;

};


#endif


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

   void saveProject(bool=false);
   void on_openManagerWidgetButton_clicked(bool );
   void on_frameTab_changed(int index);
   void on_openProjectButton_clicked();

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
    void on_playItemButton_clicked();
    void on_playItem_changed();
    void on_audioFilterButton_clicked(bool active);
    void closeProject();



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

    void addSelectedFileToProject();
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

 protected:

    QString     outputType, sourceDir;
    unsigned int maxRange=0;

signals:

  void hasIndexChangedSignal();
  void is_signalList_changed(int);


};

#endif // DVDA_H
#ifndef ENUMS_H
#define ENUMS_H

class flags
{
public:
    enum {flush=0xF00};
    enum {importFiles, importNames, typeIn, isEmbedded};
    enum font {boldTitle, regularTitle, italicTitle};
    enum commandLineType {dvdaCommandLine, createDisc, createIso, dvdaExtract, lplexFiles, hasListCommandLine, noCommandLine};
    enum status {
        defaultStatus,
        defaultCommandLine,
        commandLineMask=0xF,
        commandLinewidgetDepthMask=0xF,
        untoggledCommandLine=0x00,
        toggledCommandLine=0x10,
        commandLineToggleMask=0xF0,
        enabled=0x000,
        disabled=0x100,
        enabledMask=0xF00,
        widgetMask=0xF000,
        checked=0x0000,
        unchecked=0x1000,
        multimodal=0x2000
    };
    static int lplexRank;

};

enum actionType {Select, OpenFolder, BrowseFile};

enum  {
  hasUnsavedOptions=0x0,
  hasSavedOptions=0x1,
  hasSavedOptionsMask=0xF,
  UpdateTree=0x0010,
  UpdateTreeMask=0x00F0,
  SaveTree=0x0100,
  SaveTreeMask=0x0F00,
  UpdateMainTabs=0x1000,
  UpdateTabMask=0x7000,
  UpdateOptionTabs=0x2000,
  KeepOptionTabs=0x4000,
  ParseXml=0xF000,
  ParseXmlMask=0xF000
};

enum {
    refreshProjectManagerFlag=0x000,
    refreshProjectAudioZoneMask=0x00F,
    refreshProjectVideoZoneMask=0x0F0,
    refreshProjectSystemZoneMask=0xF00,
    refreshProjectInteractiveMask=0xF000,
    refreshAudioZone=0x001,
    refreshVideoZone=0x010,
    refreshSystemZone=0x100,
    refreshProjectInteractiveMode=0x1000,
    refreshAllZones=refreshAudioZone|refreshVideoZone|refreshSystemZone
};


#endif // ENUMS_H
#ifndef FCOLOR_H
#define FCOLOR_H

#include <QtWidgets>
#include <QColorDialog>
#include <QPainter>


#define COLOR_LABEL_WIDTH 150
#define COLOR_LABEL_HEIGHT 20
//#FF0000" red
//#00FF00" green
//#0000FF" blue
#define DEFAULT_COLOR_0 "red"
#define DEFAULT_COLOR_1 "#00FF00"
#define DEFAULT_COLOR_2 "turquoise"

QString RGB2YCrCbStr(QColor& color);
QString RGBStr2YCrCbStr(const QString  &color);
QColor YCrCbStr2QColor(QString str);

class colorRect : public QWidget
{
private:

    QLabel *colorLabel;

public:
    colorRect(QColor color=Qt::red);
    void setWidth(int w);
    void setBrush(const QColor &b);
    virtual bool isAbstractEnabled() ;
};


/*
    see files in doc/
*/


inline QString CONV(qreal X)
 { return QString::number(qFloor(X), 16).rightJustified(2, '0', true);}

inline qreal normalise(qreal X)
{
    if (X<0)   return 0;
    return (X>255)?255:X;
}

inline QString  RGB2YCrCbStr(QColor& color)
{
    qreal red=color.red();
    qreal green=color.green();
    qreal blue=color.blue();

    qreal Y = normalise(0.299 * red + 0.587 * green + 0.114 * blue) ;
    qreal Cb = normalise(-0.1687 * red  - 0.3313 * green + 0.5 * blue + 128);
    qreal Cr = normalise(0.5 * red - 0.4187 * green - 0.0813 * blue + 128) ;

    return CONV(Y) + CONV(Cr) + CONV(Cb);
}

inline QString  RGBStr2YCrCbStr(const QString&  s)
{
    QColor color;
    color.setNamedColor(QString(s));
    return RGB2YCrCbStr(color);
}

inline QColor YCrCbStr2QColor(QString str)
{

    if (str.length() < 6) return QColor(0,0,0);

    qreal Y =  str.mid(0,2).toInt(NULL, 16);
    qreal Cr = str.mid(2,2).toInt(NULL, 16);
    qreal Cb = str.mid(4,2).toInt(NULL, 16);

    qreal red = normalise(Y + 1.402 * (Cr-128));
    qreal green = normalise( Y - 0.34414 * (Cb-128) - 0.71414 * (Cr-128));
    qreal blue = normalise( Y + 1.772 * (Cb-128));

    return QColor(qFloor(red), qFloor(green),qFloor(blue));
}





#endif // FCOLOR_H
#include "flistframe.h"
#include "common.h"

#include <QMessageBox>

FListFrame::FListFrame(QObject* parent,  QAbstractItemView* tree, short import_type, const QString &hashKey,
                         const QStringList &description, const QString &command_line, int cli_type, const QStringList &separator, const QStringList &xml_tags,
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
           embeddingTabWidget->setTabToolTip(mainTabWidgetRank, description.at(0));
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

 connect(addGroupButton,
               &QToolButton::clicked,
               [this]{
                      if (Hash::wrapper[frameHashKey]->last().isEmpty()) return;
                      Hash::wrapper[frameHashKey]->append(QStringList());
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

  if (Hash::wrapper[frameHashKey]->count() < currentIndex+1) return;

  widgetContainer[currentIndex]->clear();

  /* warning : use *[], not ->value, to modifie any list content, even subordinate */

  (*Hash::wrapper[frameHashKey])[currentIndex].clear();

  for (int j=1; currentIndex +j < cumulativePicCount.count() ; j++)
      if  (cumulativePicCount.count() >= currentIndex+j+1)
          cumulativePicCount[currentIndex+j] -= Hash::wrapper[frameHashKey]->at(currentIndex).count();

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

   (*Hash::wrapper[frameHashKey])[currentIndex].swap(row, row-1);
}

void FListFrame::on_deleteItem_clicked()
{
  updateIndexInfo();

  if (Hash::wrapper[frameHashKey]->at(currentIndex).isEmpty()) return;
  if (row <0) return;

  QModelIndexList L=fileListWidget->currentListWidget->selectionModel()->selectedRows();
  int size=L.size();
  int  rank=0, localrow;
  while (rank < size)
  {
      localrow=L[rank].row() - rank;
      fileListWidget->currentListWidget->takeItem(localrow);
      (*Hash::wrapper[frameHashKey])[currentIndex].removeAt(localrow);
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

   (*Hash::wrapper[frameHashKey])[currentIndex].swap(row, row+1);
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


void FListFrame::addNewTab()
{
    mainTabWidget->insertTab(getRank() ,widgetContainer.at(getRank()) , tags[1] + " "+ QString::number(getRank()+1));
    mainTabWidget->setCurrentIndex(getRank());
}

void FListFrame::addGroup()
{
        int size=Hash::wrapper[frameHashKey]->size();
        slotListSize=(slotList)? slotList->size() : 0;

        // do not create an new group over an empty group (strict behaviour)

       if (size < 2 ||Hash::wrapper[frameHashKey]->at(size-2).isEmpty()) return;
      //  if ((slotListSize) && (getRank() >= slotListSize-1)) return;

        if (cumulativePicCount.count() <  slotListSize+1) cumulativePicCount.append(cumulativePicCount[getRank()]+Hash::wrapper[frameHashKey]->at(getRank()).count());

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

     widgetContainer[j]->addItems((*Hash::wrapper[frameHashKey])[j]);
      *signalList << Hash::wrapper.value(frameHashKey)->at(j);
   }
}


void FListFrame::deleteGroup()
{
 updateIndexInfo();

 if (getRank() < 1) return;

 mainTabWidget->removeTab(currentIndex);

 Hash::wrapper[frameHashKey]->removeAt(currentIndex);

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
     Hash::wrapper[frameHashKey]->removeAt(j);
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


bool FListFrame::addStringToListWidget(const QString& filepath, int index)
{
  // normaly it should be useless to call updateIndexInfo() here
 updateIndexInfo();
 if ((filepath.isEmpty()) || (currentIndex >= (*Hash::wrapper[frameHashKey]).count() ) || (signalList == NULL)) return false;


 if  (!filepath.isEmpty())
 {
     fileListWidget->currentListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
     fileListWidget->currentListWidget->addItem(filepath);
     fileListWidget->currentListWidget->setCurrentRow(row+1);
     fileListWidget->currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

     (*Hash::wrapper[frameHashKey])[currentIndex] << filepath;

     *(fileListWidget->signalList) << filepath;
     *signalList << filepath; //make a copy. Necessary to avoid losing dragged and dropped files to list widget directly.
 }

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

 void addGroup();

 int row, currentIndex,  slotListSize;

public:

 QToolButton *importFromMainTree=new QToolButton,
                        *moveDownItemButton=new QToolButton,
                        *moveUpItemButton=new QToolButton,
                        *retrieveItemButton=new QToolButton,
                        *clearListButton=new QToolButton,
                        *addGroupButton=new QToolButton,
                        *deleteGroupButton=new QToolButton;

 QTabWidget *mainTabWidget, *embeddingTabWidget;
 QAbstractItemView *fileTreeView;
 QStringList* slotList= new QStringList;
 QStringList *signalList= new QStringList;
 QList<int> cumulativePicCount;
 QLabel* fileLabel=new QLabel;
 QString fileLabelText;
 QFileSystemModel *model=new QFileSystemModel;
 QGroupBox *controlButtonBox=new QGroupBox, *tabBox=new QGroupBox;

 /* accessors */
 int getRank() {return widgetContainer.count()-1;}
 const QString &getHashKey() const {return frameHashKey;}
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
inline QListWidget*  getCurrentWidget() { return widgetContainer[this->mainTabWidget->currentIndex()];}

inline int getCurrentIndex() {return this->mainTabWidget->currentIndex();}
inline int getCurrentRow() {return getCurrentWidget()->currentRow();}

void addDirectoryToListWidget(const QFileInfo&, int);
void addNewTab();
bool addStringToListWidget(const QString& , int );


FListFrame(QObject* parent,  QAbstractItemView * fileTreeView, short import_type, const QString &hashKey,
            const QStringList &description, const QString &command_line, int commandLineType, const QStringList &separator, const QStringList &xml_tags,
            int mainTabWidgetRank=-1, QIcon* icon=NULL, QTabWidget* parentTabWidget=NULL,
           QStringList* terms=NULL, QStringList* translation=NULL, QStringList* slotL=NULL);


public slots:

    void deleteGroup();
    void on_deleteItem_clicked();
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
#include "forms.h"



FRichLabel::FRichLabel(const QString &title, const QString &path, int flag) : QWidget()
{
  QHBoxLayout* mainLayout=new QHBoxLayout;
  QLabel *label=new QLabel;
  QLabel *label2=new QLabel;
  switch (flag)
    {
      case flags::regularTitle:
        label->setText(title);break;

      case flags::boldTitle:
        label->setText("<b>"+title+"</b>"); break;

      case flags::italicTitle:
        label->setText("<i>"+title+"</i>"); break;
    }

  label->setFixedHeight(20);
  label2->setPixmap(QPixmap(path).scaledToWidth(48, Qt::SmoothTransformation));

  mainLayout->addStretch(100);
  mainLayout->addWidget(label, Qt::AlignRight);
  mainLayout->addWidget(label2, Qt::AlignRight);
  setLayout(mainLayout);
}
#ifndef FORMS_H
#define FORMS_H

#include <QLabel>
#include <QHBoxLayout>
#include "enums.h"

class FRichLabel :public QWidget
{

public:
  FRichLabel(const QString& title, const QString& path, int flag=flags::boldTitle);

};

#endif // FORMS_H
#include "fstring.h"

QHash<QString,QStringList>    Hash::description;
QHash<QString, FStringList* >   Hash::wrapper;


FString   FString::operator & (FString  s)
{
  if (x * s.x == 1) return "yes";
  else return "no";
}


FString   FString::operator & (bool  s)
{
  if (x * s ==1) return "yes";
  else return "no";
}

void   FString::operator &= (bool  s)
{
  x = x & s;
  if (x == 1) p="yes";
  else p="no";
}

void   FString::operator &= (FString  s)
{
  x = x & s.x;
  if (x ==1) p="yes";
  else p="no";
}


FString   FString::operator | (FString  s)
{
  if ((x == 1) || (s.x == 1) )return "yes";
  else return "no";
}

FString   FString::operator ! ()
{
  switch (x)
    {
      case  1:  return "no"; break;
      case  0:  return "yes"; break;
      default:  return "no";
    }
}

QString FString::toQString() const
{
  return p;
}


QString& FString::toQStringRef()
{
  return p;
}

FString  FString::operator * ()
{
  return Hash::wrapper[p]->toFString();
}

short FString::toBool()
{
  if ( x > 1) return 0;
  else return x;
}

bool FString::isFilled()
{
  return (!p.isEmpty());
}

const FString  FString::fromBool(bool value)
{
  x=value;
  if (value) p="yes"; else p="no";
  return FString(p);
}

bool FString::isTrue()
{
  return (x == 1);
}

bool FString::isMultimodal()
{
  return (x == flags::multimodal);
}

void FString::setMultimodal()
{
  x = flags::multimodal;
}

bool FString::isFalse()
{
  return (x == 0);
}

bool FString::isBoolean()
{
  return ((x == 0) | (x == 1));
}

const FStringList& FString::split(const QString &sep) const
{
   QStringList Q=QStringList();
  for (int i=0; i < sep.size(); i++)
       Q << QString(sep[i]) ;
  return split(Q);
}


const FStringList& FString::split(const QStringList &separator) const
{
  if (this->isEmpty()) return FStringList();

  short length=separator.size();

  // switch cannot be used here because of QListIterator declaration

  switch (length)
    {

    case 1:
      return this->toQString().split(separator[0]); break; //Beware of calling toQString() to avoid infinite loop.

    case 2:
      if (this->length() >= 3)
        {
            QListIterator<QString> i=QListIterator<QString>(this->toQString().split(separator[1]));
            QList<QStringList> L=QList<QStringList>();
            while (i.hasNext())
            L << i.next().split(separator[0]);
            return L;
        }

    default:
      return FStringList(this);
    }
}



const QStringList& FStringList::join()
{
  // Flattens FStringList into QStringList

  FStringListIterator i(this);
  QStringList S=QStringList();
  while (i.hasNext())
    {
      S << i.next();
    }
  return S;
}

const FString  FStringList::join(const QStringList &separator) const
{
  if (this == NULL) return "";
  if (this->size() == 0) return "";

  QStringList S;
  QString sep, str;
  FStringListIterator i(this);

   switch(separator.length())
    {
     case 0:
      for (int i=0; i < size() ; i++)
        str += this->at(i).join("");
      return FString(str);

    case 1:
       if (size() == 1)
          return this->at(0).join(separator[0]);
      return "";

   case 2:
      sep=QString(separator[0]);
      if (size() == 1)
        {
          return this->at(0).join(sep);
        }
      S=QStringList();
      while (i.hasNext())
        S  << i.next().join(sep);
      sep=QString(separator[1]);
      return FString(S.join(sep));
   }

   return ("");
}


inline QStringList setDistributedTags(const QString & tag,const QStringList &properties, const QStringList &tagged)
{
  QStringList taggedList;
  taggedList =  (tagged.isEmpty()) ? QStringList("") : tagged;
  QStringListIterator i(taggedList);
  QStringList S=QStringList();
  while (i.hasNext())
    {
      QString item=i.next();
      S << QString("<") + tag+ (properties.isEmpty() ? "": " ")+  properties.join(" ") + QString(">")+ item +QString("</")+tag+QString(">");
    }
  return S;
}


 inline QString FStringList::setEmptyTags(const QStringList & tags) const
{
  QStringListIterator i(tags);
  QString S="";
  while (i.hasNext())
    {
      QString tag=i.next();
      S =  "<" + tag + ">" + S + "</" + tag + ">" ;
    }
  return S;
}


const QString FStringList::setTags(const QStringList  &tags, const FStringList *properties ) const
{
  if ((this == NULL) ||  this->hasNoString())
  {
      // should trigger a crash
      return setEmptyTags(tags);
  }


    // deactivated

  if ((properties) && (properties->size() != 2) )
      return setEmptyTags(tags);
   // deactivated

  QStringList S;

  if (tags.length() >= 3 ) return setEmptyTags(tags);

  for (int i=0; i < size(); i++)
    {
      QStringList tagged=this->at(i);
      if  (tagged.isEmpty()) continue;
      QString str;
      if  (properties)
        str="     "+ setDistributedTags(tags[0], properties->at(0), tagged).join("\n     ");
      else
        str="     "+ setDistributedTags(tags[0], QStringList(), tagged).join("\n     ");
      S << "\n"   + str   + "\n   ";
    }

  QString str;

  if (tags.length() == 1)
    {
      return S.join("");
    }
  else
    if (tags.length() == 2)
      {

      if (properties)
          str=setDistributedTags(tags[1],  properties->at(1),  S).join("\n   ");
     else
          str=setDistributedTags(tags[1],  QStringList(),  S).join("\n   ");
      }

   return (str);
}
#ifndef FSTRING_H
#define FSTRING_H

#include <QtCore>
#include "enums.h"

class FString;
class FStringList;


class Hash : public QHash<QString, QString>
{
public:

  /* Hash::description converts a string like "targetDir" into its (sentence-like) description for display in project manager (first column)*/
  static QHash<QString,QStringList> description;

  /* Hash::wrapper  is used for storing information for xml project parsing/writing.
   *It converts a string label like "audioMenu" into a pointer to an FStringList object that contains a set of file paths
   * (or more generally, text phrases) grouped into a QStringList for each associated file in a list of files */
  static QHash<QString, FStringList *> wrapper;

};

class FString : public QString
{
private:
 int x;
 QString p;
 void testBool(QString &s, flags::status flag=flags::defaultStatus)
 {
 if (s.isEmpty())
   x=2;
 else
   {
     if (s == QString("yes"))
       x=1;
     else
     if (s == QString("no"))
       x=0;
     else
       // Preserving flagged status
         x= (flag != flags::defaultStatus)? flag : 2;
    }
 }

public:

  FString  operator | (FString   );
  FString  operator ! ();
  FString  operator & (FString  );
  FString  operator & (bool );
  void  operator &= (FString  );
  void  operator &= (bool );
  FString  operator * ();

  FString()
  {
    x=2;
    p="";
  }

  FString(QString s, flags::status flag=flags::defaultStatus):QString(s)
  {
    p=s;
    testBool(s, flag);
  }

  FString(const char* s):FString(QString(s))  {  }

  FString(bool value)
  {
    x=value;
    if (value) p="yes"; else p="no";
    this->append(p); // caution! does not work otherwise
  }

  /* copy constructor */
  FString(const FString  & v):QString(v.p)
  {
    x=v.x;
    p=v.p;
  }


  const FStringList& split(const QString &) const;
  const FStringList& split(const QStringList &separator) const;

  short toBool();
  bool isBoolean();
  bool isFalse();
  bool isTrue();
  bool isMultimodal();
  void setMultimodal();
  bool isFilled();
  const FString fromBool(bool);
  QString toQString() const;
  QString& toQStringRef();
};


class FStringList : public QList<QStringList>
{
private:
  QStringList q;

public:

  FStringList( ) : QList<QStringList>() { }
  FStringList(const QString& s) : QList<QStringList>()  { this->append(QStringList(s)); }
  FStringList(const FString &s):FStringList(s.toQString()) {}
  FStringList(const FString *s):FStringList(s->toQString()) {}
  FStringList(const QStringList &L): QList<QStringList>() {this->append(L);}
  FStringList(const  QList<QStringList> &s) : QList<QStringList>(s) {}
  FStringList(const  QList<QVariant> &s) : QList<QStringList>()
  {
      QListIterator<QVariant> i(s);
      while (i.hasNext())
      {
          QVariant v=i.next();
          if (!v.isValid()) continue;
          if (v.canConvert(QMetaType::QStringList))
                        this->append(v.toStringList());
      }
  }

  FStringList(const QString& a, const QString& b, const QString& c):QList<QStringList>()  { this->append(QStringList() << a << b << c);}
  const FString join(const QStringList &) const ;
  const FString& join(const char* s) const {return join(QStringList((QString(s)))); }
  const QStringList& join() ;
  QString setEmptyTags(const QStringList &)const;
  const QString setTags(const QStringList &tags, const FStringList *properties=NULL) const;
  FString toFString() const { return ((this->isEmpty()) || this->at(0).isEmpty())?  "" : FString(this->at(0).at(0)); }
  FString toQString() const { return ((this->isEmpty()) || this->at(0).isEmpty())?  "" : QString(this->at(0).at(0)); }
  int toInt() const {return ((this->isEmpty() || this->at(0).isEmpty())? 0: this->at(0).at(0).toInt());}
  bool hasNoString() const { return (isEmpty() || (this->at(0).isEmpty()) || (this->at(0).at(0).isEmpty())); }
  bool  isFilled() const { return (!isEmpty() && (!this->at(0).isEmpty()) && (!this->at(0).at(0).isEmpty())); }

  /* copy constructor */
  FStringList(const FStringList  & L):QList<QStringList>(L)  { }
};

class FStringListIterator : public QListIterator<QStringList>
{
public:
  FStringListIterator(const FStringList& list) : QListIterator(list) {}
  FStringListIterator(FStringList *list) : QListIterator(*list) {}
  FStringListIterator(const FStringList *list) : QListIterator(*list) {}
 };


#endif // FSTRING_H
#include "fwidgets.h"
#include "common.h"
#include "fcolor.h"


/* using above function with controlled object encapsulation */

#define FCore3(defaultValue, hashKey, description, optionLabel, status, controlledObjects)\
{\
  if (controlledObjects == NULL)\
   {FCore( defaultValue, hashKey, description, optionLabel, status,NULL,NULL)}\
  else\
    {\
      switch (controlledObjects->size())\
        {\
        case 1:\
         {FCore(defaultValue, hashKey, description, optionLabel, status,  &(*(new Q2ListWidget) << controlledObjects[0]),  NULL)}\
          break;\
\
        case 2:\
          if (controlledObjects[0][0][0] == NULL)\
              {FCore(defaultValue, hashKey, description, optionLabel, status, NULL,  &(*(new Q2ListWidget) << controlledObjects[1]))}\
          else\
            {\
              Q2ListWidget *L1=new Q2ListWidget, *L2=new Q2ListWidget;\
              *L1 << controlledObjects[0];\
              *L2 << controlledObjects[1];\
              { FCore(defaultValue, hashKey, description, optionLabel, status, L1, L2)}\
            }\
          break;\
\
        default:\
          break;\
        }\
    }\
}

#define FCore2(defaultStatus, hK, desc, opt, stat)\
{\
this->commandLineList= QList<FString>() << defaultStatus;\
if (((stat) & flags::widgetMask) == flags::multimodal) { this->commandLineList[0].setMultimodal(); }\
this->hashKey=hK;\
    this->setToolTip(desc.at(0));\
this->commandLineType=stat | flags::multimodal;\
if (!hK.isEmpty())\
{\
    if (!desc.isEmpty())\
    {\
        this->description=desc;\
        Hash::description[hK]=desc;\
    }\
}\
\
this->optionLabel=opt;\
\
this->setEnabled((((stat) | flags::multimodal)& flags::enabledMask) ==  flags::enabled);\
Hash::wrapper[hK] = new FStringList;\
    *Hash::wrapper[hK]  << (QStringList() << QString());\
\
Abstract::abstractWidgetList.append(this);\
\
FAbstractConnection::meta_connect(this, this->enabledObjects, this->disabledObjects);\
}


#define FCore(defaultStatus, hashKey, decription, option, status, EO, DO)\
{\
this->enabledObjects=EO; \
this->disabledObjects=DO;\
FCore2(defaultStatus, hashKey, decription, option, status)\
}


void applyHashToStringList(QStringList *L, QHash<QString, QString> *H,  const QStringList *M)
{
    if ((H == NULL) || (M == NULL) || (L == NULL)) return;
    QStringListIterator j(*M);

    while (j.hasNext())
        *L << (*H) [j.next()];
}

template <typename T, typename U> void createHash(QHash<T, U > *H, const QList<T> *L, const QList<U> *M)
{
    if ((H == NULL) || (L == NULL) || (M == NULL)) return;
    QListIterator<T> i(*L);
    QListIterator<U> j(*M);

    while ((j.hasNext()) && (i.hasNext()))
        (*H)[i.next()]=j.next();
}

void FAbstractConnection::meta_connect(const FAbstractWidget* w,  const Q2ListWidget *enabledObjects,  const Q2ListWidget *disabledObjects)
{
    if ((enabledObjects != NULL) &&  (!enabledObjects->isEmpty()) )
    {

        QListIterator<QWidget*> componentlistIterator(w->getComponentList());
        Q2ListIterator objectlistIterator(*enabledObjects);
        while ((componentlistIterator.hasNext()) && (objectlistIterator.hasNext()))
        {
            QWidget* component=componentlistIterator.next();
            QListIterator<QWidget*> i(objectlistIterator.next());
            while (i.hasNext())
            {
                QWidget* item=i.next();
                if ((item == NULL) || (component==NULL)) continue;
                // This does not always work automatically through Qt parenting as it normally should, so it is necessary to reimplement enabling dependencies
                // e.g. for QLabels

                if (!component->isEnabled())
                    item->setEnabled(false);


                if (component->metaObject()->className() == QString("FListFrame"))
                    connect(component, SIGNAL(is_signaList_changed(bool)), item, SLOT(setEnabled(bool)));
                else
                {
                    connect(component, SIGNAL(toggled(bool)), item, SLOT(setEnabled(bool)));
                }

                // this connection must follow the general enabling adjustments above
                // A way to avoid this default behaviour is to group the enabled boxes into a  flat (invisible) GroupBox

                if (item->metaObject()->className() == QString("FCheckBox"))
                    connect(component, SIGNAL(toggled(bool)), item, SLOT(uncheckDisabledBox()));
                else
                    if (item->metaObject()->className() == QString("FRadioBox"))
                        connect(component, SIGNAL(toggled(bool)), item, SLOT(resetRadioBox(bool)));


            }
        }
    }

    if ((disabledObjects != NULL) &&  (!disabledObjects->isEmpty()))
    {

        QListIterator<QWidget*> newcomponentlistIterator(w->getComponentList());
        Q2ListIterator newobjectlistIterator(*disabledObjects);
        while ((newcomponentlistIterator.hasNext()) && (newobjectlistIterator.hasNext()))
        {

            QWidget* component=newcomponentlistIterator.next();
            QListIterator<QWidget*> j(newobjectlistIterator.next());
            while (j.hasNext())
            {
                QWidget* item=j.next();

                if ((item == NULL) || (component==NULL)) continue;

                connect(component, SIGNAL(toggled(bool)), item , SLOT(setDisabled(bool)));

                // this connection must follow the general enabling adjustments above
                if (item->metaObject()->className() == QString("FCheckBox"))
                    connect(component, SIGNAL(toggled(bool)), item, SLOT(uncheckDisabledBox()));
            }
        }
    }
}


const QStringList& FAbstractWidget::commandLineStringList()
{
    /* If command line option is ill-formed, or if a corresponding checkbox is unchecked (or negatively checked)
  * or if an argument-taking option has no-argument, return empty */

    if ((optionLabel.isEmpty())
            ||  (commandLineList[0].isFalse())
            ||  (commandLineList[0].toQString().isEmpty())
            ||  (this->isAbstractDisabled())) return {};


if (commandLineList[0].isTrue() | commandLineList[0].isMultimodal())
return  ((optionLabel.size() == 1)?
             QStringList( "-"+optionLabel):
             QStringList ("--" +optionLabel));

if ((commandLineType & flags::commandLinewidgetDepthMask) == flags::hasListCommandLine)
{
    QStringList strL;
    QListIterator<FString> i(commandLineList);
    strL << ((optionLabel.size() == 1)? "-":"--") +optionLabel;
    while (i.hasNext())
        strL <<  i.next();
    return strL;
}
else
{
if (optionLabel.size() == 1)
return (QStringList("-"+optionLabel+" "+commandLineList[0].toQString()));
else
return (QStringList("--"+optionLabel+"="+commandLineList[0].toQString()));
}

}

/* caution : abstractWidgetList must have its first two elements as respectively being with "DVD-A" and "DVD-V" hashKeys. */

QList<FAbstractWidget*> Abstract::abstractWidgetList= QList<FAbstractWidget*>();


void Abstract::refreshOptionFields()
{
    QListIterator<FAbstractWidget*>  j(Abstract::abstractWidgetList);
    while (j.hasNext())
    {
        j.next()->refreshWidgetDisplay();
    }
}

FListWidget::FListWidget(const QString& hashKey,
                         int status,
                         const QStringList& description,
                         const QString& commandLine,
                         const QStringList& sep,
                         const QStringList &taglist,
                         const QList<QString> *terms,
                         const QList<QString>*translation,
                         QWidget* controlledWidget)

{
    setAcceptDrops(true);
    componentList=QList<QWidget*>() << this;
    widgetDepth="2";

    Abstract::initializeFStringListHash(hashKey);

    setObjectName(hashKey+" "+description.join(" "));
    currentListWidget=new QListWidget;
    currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    currentListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    Q2ListWidget *controlledListWidget=new Q2ListWidget;
    *controlledListWidget << (QList<QWidget*>() << controlledWidget);

    FCore3("", hashKey, description, commandLine, status, controlledListWidget)
    separator=sep;

    tags=taglist;
    signalList=new QStringList;

    /* if a Hash has been activated, build the terms-translation Hash table so that translated terms
   * can be translated back to original terms later on, so as to get the correct command line string chunks */

    if ((terms == NULL) || (translation == NULL))
        listWidgetTranslationHash=NULL;
    else
    {
        listWidgetTranslationHash=new QHash<QString, QString>;
        createHash(listWidgetTranslationHash, translation, terms);
    }

    connect(currentListWidget, SIGNAL(currentRowChanged(int)), SIGNAL(is_signalList_changed(int)));

}



const FString& FListWidget::translate(const FStringList &s)
{
    FStringListIterator i(s)  ;
    FStringList L=FStringList();
    int j=0;
    while (i.hasNext())
    {
        L << QStringList();
        QStringList translation=QStringList();
        QStringList terms=  QStringList();

        translation=i.next();
        applyHashToStringList(&terms, listWidgetTranslationHash, &translation) ;

        L[j++]=terms;

    }

    return commandLineList[0]=L.join(separator);
}


void FListWidget::setWidgetFromXml(const FStringList &s)
{
    /* for display */

    if (s.isFilled())
    {
        int size=s.size()-1;

        /* add as many groups as there are QStringLists in excess of 1 and fill in the tabs with files */
         emit(open_tabs_signal(size)) ;
    }
    else
    {
        commandLineList={""};
        return;
    }

    /* for command-line */
    /* if a Hash has been activated, strings are saved in Xml projects
    * as "translated" items to be displayed straightaway in list widgets
    * command lines, in this case, need to be translated back to original terms */

    if (listWidgetTranslationHash)
        commandLineList= QList<FString>() << translate(s);
    else
    {
        if ((commandLineType & flags::commandLinewidgetDepthMask) == flags::hasListCommandLine)
        {
            if (separator.size() < 2) commandLineList=QList<FString>();
            FStringListIterator i(Hash::wrapper[hashKey]);
            while (i.hasNext())
            {
                commandLineList << separator[1] ;
                QStringListIterator j(i.next());
                while (j.hasNext())
                    commandLineList << j.next();
            }
        }
        else
            commandLineList[0]= Hash::wrapper[hashKey]->join(separator);
    }
}

const FString FListWidget::setXmlFromWidget()
{
    if (!Hash::wrapper.contains(hashKey)) return FStringList().setEmptyTags(tags);

    if (listWidgetTranslationHash)
        commandLineList=QList<FString>() << translate(*Hash::wrapper[hashKey]);
    else
    {
        if ((commandLineType & flags::commandLinewidgetDepthMask)  == hasListCommandLine)
        {
            if (separator.size() < 2) commandLineList=QList<FString>();
            FStringListIterator i(Hash::wrapper[hashKey]);
            while (i.hasNext())
            {
                commandLineList << separator[1] ;
                QStringListIterator j(i.next());
                while (j.hasNext())
                    commandLineList << j.next();
            }
        }

        else
            commandLineList[0]=Hash::wrapper[hashKey]->join(separator);
    }

    return Hash::wrapper[hashKey]->setTags(tags);
}


void FListWidget::refreshWidgetDisplay()
{
//    currentListWidget->clear();
//    if ((Hash::wrapper.contains(hashKey)) && (Hash::wrapper[hashKey]->count() > rank ))
//         currentListWidget->addItems(Hash::wrapper[hashKey]->at(getank));
}


FCheckBox::FCheckBox(const QString &boxLabel, int status,const QString &hashKey, const QStringList &description,
                     const QString &commandLineString, const Q2ListWidget* controlledObjects) : QCheckBox(boxLabel)
{
    componentList={this};
    widgetDepth="0";
    bool mode= ((status & flags::widgetMask) == flags::checked) ;
    FCore3(FString(mode), hashKey, description, commandLineString, status, controlledObjects)
}


FCheckBox::FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QStringList &description,
                     const QList<QWidget*> &enabledObjects, const QList<QWidget*> &disabledObjects) : QCheckBox(boxLabel)
{
    componentList={this};
    widgetDepth="0";
    bool mode= ((status & flags::widgetMask) == flags::checked) ;

    Q2ListWidget *dObjects=new Q2ListWidget, *eObjects=new Q2ListWidget;
    if (enabledObjects.isEmpty()) eObjects=NULL;
    else
        *eObjects << enabledObjects;
    if (disabledObjects.isEmpty()) dObjects=NULL;
    else
        *dObjects << disabledObjects;
    FCore(FString(mode), hashKey, description, "", status, eObjects, dObjects);
}

void FCheckBox::uncheckDisabledBox()
{
    if (!this->isEnabled()) this->setChecked(false);
}


void FCheckBox::refreshWidgetDisplay()
{
    bool checked=commandLineList[0].isTrue();

    setChecked(checked);

    if ((enabledObjects) && (enabledObjects->size()))
    {
        QListIterator<QWidget*> i(enabledObjects->at(0));

        while (i.hasNext())
        {
            QWidget *item=i.next();
            if (item == NULL) continue;
            item->setEnabled(checked);
        }
    }

    if ((disabledObjects) && (disabledObjects->size()))
    {
        QListIterator<QWidget*> i(disabledObjects->at(0));
        while (i.hasNext())
        {
            QWidget* item=i.next();
            if (item == NULL) continue;
            item->setDisabled(checked);
        }
    }
}

const FString FCheckBox::setXmlFromWidget()
{
    *Hash::wrapper[getHashKey()]=FStringList(commandLineList[0].fromBool(this->isChecked()));
    return commandLineList[0].toQStringRef();
}

void FCheckBox::setWidgetFromXml(const FStringList &s)
{
    QString st=s.toFString();
    commandLineList[0]=FString(st);
    this->setChecked( commandLineList[0].isTrue());
}



FRadioBox::FRadioBox(const QStringList &boxLabelList, int status,const QString &hashKey, const QStringList &description,
                     const QStringList &stringList, const Q2ListWidget *enabledObjects,  const Q2ListWidget *disabledObjects)
{
    /* button 0 should have special controlling properties (either enabling or disabling) over subordinate widgets */
    widgetDepth="0";
    optionLabelStringList=stringList;
    size=optionLabelStringList.size();
    if (size <  status) return;
    rank=0;

    if (boxLabelList.size() != (size+1)) return;
    QStringListIterator i(boxLabelList);
    radioButtonList=QList<QRadioButton*> ();
    componentList= QList<QWidget*>();
    radioGroupBox=new QGroupBox(i.next());
    QRadioButton *button;

    QVBoxLayout* mainLayout=new QVBoxLayout;
    QVBoxLayout *radioBoxLayout=new QVBoxLayout;

    while(i.hasNext())
    {
        radioButtonList << (button=new QRadioButton(i.next(), this));
        componentList << button;
        radioBoxLayout->addWidget(button);
    }


    FCore("0", hashKey, description, optionLabelStringList[status], status | flags::multimodal, enabledObjects, disabledObjects)


    connect(this->radioButtonList.at(0), SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));

    for (int i=0; i < size; i++)
    {
        connect(this->radioButtonList.at(i), SIGNAL(toggled(bool)), this, SLOT(toggledTo(bool )));
    }

    /* This is a nice trick to activate button 0 default controlling properties over subordinate widgets *
  * radioButtonList[0] will be set checked by refreshWidgetDisplay on GUI launch *
  * Then FAbstractWidget::meta_connect will activate  controlling slots of button 0 */

    radioButtonList[1]->toggle();

    radioGroupBox->setLayout(radioBoxLayout);
    mainLayout->addWidget(radioGroupBox);
    setLayout(mainLayout);
}

void FRadioBox::resetRadioBox(bool value)
{
    if (value == true)  return;
    rank=0;
    radioButtonList[0]->setChecked(true);
}

void FRadioBox::toggledTo(bool value)
{
    if (value == false) return;
    for (rank=0; rank < size && !radioButtonList[rank]->isChecked(); rank++) ;

    if (rank < size) optionLabel=optionLabelStringList[rank];
}

void FRadioBox::refreshWidgetDisplay()
{
    rank=commandLineList[0].toQString().toUInt();

    for (int i=0 ; i < size ; i++)
        radioButtonList[i]->setChecked(rank == i);

    if ((enabledObjects != NULL) && (!enabledObjects->isEmpty()))
    {
        QListIterator<QWidget*> i(enabledObjects->at(0));

        while (i.hasNext())
        {
            QWidget *item=i.next();
            if (item == NULL) continue;
            item->setEnabled(rank ==0);
        }
    }

    if ((disabledObjects != NULL) && (!disabledObjects->isEmpty()))
    {
        QListIterator<QWidget*> i(disabledObjects->at(0));
        while (i.hasNext())
        {
            QWidget* item=i.next();
            if (item == NULL) continue;
            item->setDisabled(rank ==0);
        }
    }
}

const FString FRadioBox::setXmlFromWidget()
{
    commandLineList[0]=FString(QString::number(rank), flags::multimodal) ;
    *Hash::wrapper[getHashKey()]=FStringList(commandLineList[0]);
    return commandLineList[0].toQStringRef();
}

void FRadioBox::setWidgetFromXml(const FStringList &s)
{
    QString st=s.toFString();
    commandLineList[0]=FString(st, flags::multimodal);
    rank=st.toUInt();
    for (int i=0; i < size ; i++)
        radioButtonList[i]->setChecked(i == rank);
}


FComboBox::FComboBox(const QStringList &labelList,
                     const QStringList &translation,
                     int status,
                     const QString &hashKey,
                     const QStringList &description,
                     const QString &commandLine,
                     QList<QIcon> *iconList) : QComboBox()
{

    widgetDepth="0";
    addItems(labelList);
    if (labelList.isEmpty())
        return;
    enabledObjects=NULL;
    disabledObjects=NULL;

    FCore2( labelList.at(0), hashKey, description, commandLine, status)

    if (iconList)
    {
        int j=0;
        QListIterator<QIcon> i(*iconList);
        while (i.hasNext())
            setItemIcon(j++, i.next());
    }
    setIconSize(QSize(48,24));

    signalList=new QStringList;
    *signalList=QStringList() << labelList.at(0);

    /* if a Hash has been activated, build the terms-translation Hash table so that translated terms
   * can be translated back to original terms later on, so as to get the correct command line string chunks */

    if ((labelList.isEmpty()) || (translation.isEmpty())) comboBoxTranslationHash=NULL;
    else
    {
        comboBoxTranslationHash=new QHash<QString, QString>;
        createHash(comboBoxTranslationHash, &labelList, &translation);
    }

    connect(this, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(fromCurrentIndex(const QString &)));
    connect(this, SIGNAL(currentIndexChanged(int)), this, SIGNAL(is_signalList_changed(int )));
}

void FComboBox::fromCurrentIndex(const QString &text)
{
    commandLineList[0]=FString(text);
    signalList->clear();
    for (int i=0; i < text.toInt() ; i++)
        *signalList << QString::number(i+1);
}


void FComboBox::refreshWidgetDisplay()
{
    if (commandLineList[0].isFilled())
    {
        if (findText(commandLineList[0]) != -1)
            setCurrentIndex(findText(commandLineList[0]));
        else
            if (isEditable())
            {
                addItem(commandLineList[0]);
            }
    }
}

const FString FComboBox::setXmlFromWidget()
{
    QString str=currentText();
    *Hash::wrapper[getHashKey()]=FStringList(str);
    commandLineList[0]= (comboBoxTranslationHash)? comboBoxTranslationHash->value(str) : str;
    if (commandLineList[0].isEmpty()) commandLineList[0]="none";
    return commandLineList[0].toQStringRef();
}


void FComboBox::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();
    refreshWidgetDisplay();
}


FLineEdit::FLineEdit(const QString &defaultString, int status, const QString &hashKey, const QStringList &description, const QString &commandLine):QLineEdit()
{
    widgetDepth="0";
    FCore(defaultString, hashKey, description, commandLine, status, NULL, NULL);
}


void FLineEdit::refreshWidgetDisplay()
{
    this->setText(commandLineList[0].toQString());
}

const FString FLineEdit::setXmlFromWidget()
{
    commandLineList[0]=FString(this->text());
    *Hash::wrapper[getHashKey()]=FStringList(this->text());
    if (commandLineList[0].isEmpty()) commandLineList[0]="none";
    return commandLineList[0].toQStringRef();
}

void FLineEdit::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();
    this->setText(commandLineList[0].toQString());
}


FColorButton::FColorButton(const char* text, const QString  &color)
{
    widgetDepth="0";
    QGridLayout *newLayout=new QGridLayout;
    QString strtext=QString(text);
    button=new QPushButton(strtext);

    newLayout->addWidget(button, 0, 0,  Qt::AlignHCenter);

    colorLabel = new QLabel;

    colorLabel->setFixedWidth(COLOR_LABEL_WIDTH);
    colorLabel->setFixedHeight(COLOR_LABEL_HEIGHT);
    colorLabel->setAutoFillBackground(true);
    colorLabel->setPalette(QPalette(color));
    colorLabel->setFrameShape(QFrame::StyledPanel);

    newLayout->addWidget(colorLabel,  1, 0, Qt::AlignTop);

    newLayout->setContentsMargins(0,0,0,0);
    newLayout->setColumnMinimumWidth(0, 150);
    newLayout->setRowMinimumHeight(0, 40);
    setLayout(newLayout);
    commandLineList=QList<FString>() << RGBStr2YCrCbStr(color);
    connect(button, SIGNAL(clicked()), this, SLOT(changeColors()));

    colorLabel->update();
}


void FColorButton::changeColors()
{
    QColor color=QColorDialog::getColor(Qt::green, this, tr("Select Color"));
    colorLabel->setPalette(QPalette(color)) ;
    commandLineList[0]=RGB2YCrCbStr(color);
}

int FColorButton::buttonWidth() const
{
    return button->width();
}

void FColorButton::setMinimumButtonWidth(const int w)
{
    button->setMinimumWidth(w);
}


const FString FColorButton::setXmlFromWidget()
{
     return commandLineList[0].toQStringRef();
}

void FColorButton::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();
    colorLabel->setPalette(QPalette(YCrCbStr2QColor(commandLineList[0].toQString())));
}

void FColorButton::refreshWidgetDisplay()
{
    colorLabel->setPalette(QPalette(YCrCbStr2QColor(commandLineList[0].toQString())));
}


FPalette::FPalette(const char* textR,
                   const char* textG,
                   const char* textB,
                   int status,
                   const QString &hashKey,
                   const QStringList &description,
                   const QString &commandLine,
                   int buttonWidth)
{
    widgetDepth="1";
    button[0]=new FColorButton(textR, DEFAULT_COLOR_0); // red RGB
    button[1]=new FColorButton(textG, DEFAULT_COLOR_1); // green RGB
    button[2]=new FColorButton( textB, DEFAULT_COLOR_2); //blue RGB

    Abstract::initializeFStringListHash(hashKey);

    FCore( "000000:000000:000000",  hashKey, description, commandLine, status, NULL, NULL);
    refreshPaletteHash();
    setMinimumButtonWidth(buttonWidth);
}

void FPalette::refreshPaletteHash()
{
    FStringList L=FStringList(button[0]->setXmlFromWidget(), button[1]->setXmlFromWidget(), button[2]->setXmlFromWidget());
    if (Hash::wrapper.contains(hashKey))  *Hash::wrapper[hashKey]=L;
    commandLineList[0]=L[0][0] + ":"+ L[0][1] + ":"+ L[0][2];

}

const FString FPalette::setXmlFromWidget()
{
    refreshPaletteHash();
    return Hash::wrapper[hashKey]->setTags({ "YCrCb"});
}

void FPalette::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0]=s[0].join(":");
    refreshWidgetDisplay();
}

void FPalette::refreshComponent(short i)
{
    if (!Hash::wrapper.contains(hashKey)) return;
    if (Hash::wrapper[hashKey]->size() != 1) return;
    if  (Hash::wrapper[hashKey]->at(0).size() != 3) return;

    FString str=Hash::wrapper[hashKey]->at(0).at(i);

    if ((str.size()) < 6) return;
    button[i]->colorLabel->setPalette(QPalette(YCrCbStr2QColor(str)));
}

void FPalette::refreshWidgetDisplay()
{
    refreshComponent(0);
    refreshComponent(1);
    refreshComponent(2);
}

void FPalette::setToolTip(const QString &tipstr)
{
    button[0]->setToolTip(tipstr);
    button[1]->setToolTip(tipstr);
    button[2]->setToolTip(tipstr);
}

void FPalette::setMinimumButtonWidth(const int w)
{
    button[0]->setMinimumButtonWidth( w);
    button[1]->setMinimumButtonWidth( w);
    button[2]->setMinimumButtonWidth( w);
}
#ifndef FWIDGETS_H
#define FWIDGETS_H

#include <QtWidgets>
#include "fcolor.h"
#include "fstring.h"

#define Q2ListWidget QList<QList<QWidget*> >
#define Q2ListIterator QListIterator<QList<QWidget*> >

class FStringList;
class common;
class FAbstractWidget;

class QToolDirButton : public QToolButton
{
public:
    QToolDirButton(actionType type=actionType::Select)
    {
        switch (type)
        {
        case actionType::Select :
            setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
            break;

        case actionType::OpenFolder :
            setIcon(QIcon(":images/64x64/system-file-manager.png"));
            break;

        case actionType::BrowseFile :
            setIcon(QIcon(":images/document-open.png"));
        }
    }

    QToolDirButton(const QString&  st, const actionType  type=actionType::Select):QToolDirButton(type){setToolTip(st);}

};



class FAbstractConnection : QObject
{
  Q_OBJECT

public:

  static void meta_connect(const FAbstractWidget* w,  const Q2ListWidget *enabledObjects,  const Q2ListWidget *disabledObjects=NULL);
  static void meta_connect(const FAbstractWidget* w,  const QList<QWidget*> *enabledObjects=NULL,  const QList<QWidget*> *disabledObjects=NULL)
  {
     meta_connect(w, &(*(new Q2ListWidget) << *enabledObjects),  &(*(new Q2ListWidget) << *disabledObjects));
  }
};

/* Note :
 *     Windows instantiation request non-recursive abstract class pointer lists, ie,
 *     a list of pointers to FAbstractWidget cannot be a member of FAbstractWidget
 *     This compiles but leads to severe runtime crashes (Qt5.0.2 + mingw-g++4.7 + windows XP or 7)
 *     Both compiles and runs OK under Linux however (Qt5.0.2 + g++4.7 or 4.8 + Ubuntu 13.04).
 *     The following one-member abstract structure works out this intriguing issue that remains poorly understood  */


struct Abstract
{
    static QList<FAbstractWidget*> abstractWidgetList;
    static void refreshOptionFields();
    static void initializeFStringListHash(const QString &hashKey)
    {
        Hash::wrapper[hashKey]=new FStringList;
        *Hash::wrapper[hashKey] << QStringList();
    }

    static void initializeFStringListHashes()
    {
        for (const QString& hashKey: Hash::wrapper.keys()) initializeFStringListHash(hashKey);
    }

};

class FAbstractWidget : public flags
{

public:
 const Q2ListWidget* enabledObjects;
 const Q2ListWidget* disabledObjects;


  /* is used for .dvp Xml project writing: refresh Widget information and injects current Widget state into Hash::qstring as left-valued of <...hashKey=...> */
 virtual const FString setXmlFromWidget()=0 ;

  /* does the reverse of setXmlFromWidget : reads left value of <...hashKey=...> and injects it into commandLineList. Refreshes Widget state accordingly */
  virtual void setWidgetFromXml(const FStringList& )=0;

  /* Refreshes widget state from current value of commandLineList member to ensure coherence betwenn internal object state and on-screen display */
 virtual void refreshWidgetDisplay()=0 ;

  /* accessor to privale hashKey value */
 virtual const QString& getHashKey() const=0;
 virtual const QList<QWidget*> & getComponentList() const=0;
 virtual const QString& getDepth() const=0;
 virtual const QStringList& getDescription() const=0;

  /* command-line interface maker */
  virtual const QStringList& commandLineStringList();

  /* command-line interface type */
  int commandLineType;

  // isEnabled() cannot be used as it would trigger lexical ambiguity with QWidget-inherited isEnabled() in subclasses
  // yet using virtual derivation makes it possible to invoke the QWidget-inherited isEnabled().
  virtual bool isAbstractEnabled() =0;
  bool isAbstractDisabled() {return !isAbstractEnabled();}

protected:
//  QString hashKey;
//  QString widgetDepth;
//  QString description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;

};


class FListWidget : public QWidget, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:
  FListWidget()  {}

  FListWidget(const QString& hashKey,int status,const QStringList& description,const QString& commandLine,const QStringList& sep,
              const QStringList &taglist,  const QList<QString> *terms=NULL, const QList<QString> *translation=NULL, QWidget* controlledWidget=NULL);

  FListWidget(const QString& hashKey,int status,const QStringList& description,const QString& commandLine,const QStringList& sep,
              const QStringList &taglist, const QList<QWidget*> &enabledObjects, const QList<QString> *terms=NULL, const QList<QString> *translation=NULL, QWidget* controlledWidget=NULL);

  void setWidgetFromXml(const FStringList & );
  const FString setXmlFromWidget();

  void refreshWidgetDisplay();
  bool isAbstractEnabled() {return this->isEnabled();}

  QStringList *signalList;
  QListWidget* currentListWidget;
  const QString& getHashKey() const {return hashKey; }
  const QList<QWidget*>& getComponentList() const { return componentList;}
  const QString& getDepth() const {return widgetDepth; }
  const QStringList& getDescription() const { return description; }

private:
  QStringList separator;
  QStringList tags;

  template <typename T, typename U> friend void createHash(QHash<T, U > *H, QList<T> *L, QList<U> *M);
  friend  void applyHashToStringList(QStringList *L, QHash<QString, QString> *H,  const QStringList *M);

  QHash<QString, QString> *listWidgetTranslationHash;
  const FString& translate(const FStringList &s);
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;

signals:
  void  open_tabs_signal(int);
  void is_signalList_changed(int);

};


class FCheckBox : public QCheckBox, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:

  FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QStringList &description,
                       const QList<QWidget*> &enabledObjects, const QList<QWidget*> &disabledObjects=QList<QWidget*>());

  FCheckBox(const QString &boxLabel, const QString &hashKey, const QStringList& description,
            const QList<QWidget*> &enabledObjects, const QList<QWidget*> &disabledObjects=QList<QWidget*>()):
    FCheckBox(boxLabel, flags::defaultStatus|flags::unchecked|flags::defaultCommandLine, hashKey, description,
                         enabledObjects, disabledObjects){}

  FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QStringList &description,
            const QString &commandLineString,  const Q2ListWidget* controlledObjects =NULL) ;

  FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QString &description,
            const QString &commandLineString,  const Q2ListWidget* controlledObjects =NULL) :
       FCheckBox(boxLabel,  status, hashKey, QStringList(description), commandLineString,  controlledObjects ) {}

  FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QStringList &description,
            const Q2ListWidget* controlledObjects=NULL):
    FCheckBox(boxLabel,  status | flags::noCommandLine, hashKey, description, "",  controlledObjects){}

  FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QString &description,
            const Q2ListWidget* controlledObjects=NULL):
    FCheckBox(boxLabel,  status | flags::noCommandLine, hashKey, QStringList(description), "",  controlledObjects){}

   FCheckBox(const QString &boxLabel, const QString &hashKey, const QStringList &description,
             const Q2ListWidget* controlledObjects =NULL):
    FCheckBox(boxLabel,  flags::defaultStatus | flags::noCommandLine|flags::unchecked, hashKey, description, "",  controlledObjects){}

    FCheckBox(const QString &boxLabel, const QString &hashKey, const QStringList &description,
            const QString &commandLineString,  const Q2ListWidget* controlledObjects =NULL):
                FCheckBox(boxLabel, flags::defaultStatus| flags::unchecked|flags::defaultCommandLine, hashKey, description,  commandLineString, controlledObjects){}


  void setWidgetFromXml(const FStringList& );
  const FString setXmlFromWidget();
  void refreshWidgetDisplay();
  bool isAbstractEnabled() {return this->isEnabled();}
  const QString& getHashKey() const {return hashKey; }
  const QList<QWidget*>& getComponentList() const { return componentList;}
  const QString& getDepth()  const {return widgetDepth; }
  const QStringList& getDescription() const { return description; }

private slots:
  void uncheckDisabledBox();

private:
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;

};


class FRadioBox : public QWidget, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:
   FRadioBox(const QStringList &boxLabelList, int status, const QString &hashKey, const QStringList &description,
                     const QStringList &optionLabelStringList, const Q2ListWidget* enabledObjects=NULL,  const Q2ListWidget* disabledObjects=NULL) ;

   FRadioBox(const QStringList &boxLabelList, const QString &hashKey, const QStringList &description,
                     const QStringList &optionLabelStringList, const Q2ListWidget* enabledObjects=NULL,  const Q2ListWidget* disabledObjects=NULL) :
     FRadioBox(boxLabelList, flags::defaultStatus,hashKey, description,  optionLabelStringList, enabledObjects,  disabledObjects) {}


  void setWidgetFromXml(const FStringList& );
  const FString setXmlFromWidget();
  void refreshWidgetDisplay();
  bool isAbstractEnabled() { return this->radioGroupBox->isEnabled();}
  void setToolTip(const QString & description) {this->radioGroupBox->setToolTip(description);}
  void setEnabled(bool enabled) {this->radioGroupBox->setEnabled(enabled);}
  const QString& getHashKey() const {return hashKey; }
  const QList<QWidget*>& getComponentList() const { return componentList;}
  const QString& getDepth() const {return widgetDepth; }
  const QStringList& getDescription() const { return description; }


private:
  int size;
  QList<QRadioButton*> radioButtonList;
  QStringList optionLabelStringList;
  QGroupBox* radioGroupBox;
  int rank;
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;

private slots:
  void toggledTo(bool);
  void resetRadioBox(bool value);

signals:
  void toggled(bool value);
};




class FComboBox : public QComboBox, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:

  FComboBox(const QStringList &labelList, const QStringList &translation, int status, const QString &hashKey, const QStringList &description, const QString &commandLine, QList<QIcon> *iconList);
  FComboBox(const QStringList &labelList, int status, const QString &hashKey, const QStringList &description, const QString &commandLine,  QList<QIcon> *iconList=NULL):
    FComboBox(labelList, QStringList(), status, hashKey, description, commandLine, iconList){}
  FComboBox(const QStringList &labelList, const QString &hashKey, const QStringList &description, const QString &commandLine,  QList<QIcon> *iconList=NULL):
      FComboBox(labelList, flags::defaultStatus|flags::defaultCommandLine, hashKey, description, commandLine,  iconList){}


  FComboBox(const char* str, int status, const QString &hashKey, const QStringList &description, const QString &commandLine,  QList<QIcon> *iconList=NULL):
    FComboBox(QStringList(str),  status, hashKey, description, commandLine,  iconList){}

  void setWidgetFromXml(const FStringList&);
  const FString setXmlFromWidget();
  void refreshWidgetDisplay();
  bool isAbstractEnabled() {return this->isEnabled();}
  const QString& getHashKey() const {return hashKey; }
  const QString& getDepth() const {return widgetDepth; }
  const QStringList& getDescription() const { return description; }
  const QList<QWidget*>& getComponentList() const { return componentList;}
  QStringList *signalList;

private slots:
  void fromCurrentIndex(const QString&);

private:
  QHash<QString, QString> *comboBoxTranslationHash;
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;


signals:
  void is_signalList_changed(int );

};

class FLineEdit : public QLineEdit, virtual public FAbstractWidget
{
  Q_OBJECT
  friend class FAbstractWidget;

public:
  FLineEdit(const QString &defaultstring, int status, const QString &hashKey, const QStringList &description, const QString &commandLine);
  FLineEdit(const QString &defaultstring, const QString &hashKey, const QStringList &description, const QString &commandLine):
  FLineEdit(defaultstring, flags::defaultStatus|flags::defaultCommandLine, hashKey, description, commandLine){}

  void setWidgetFromXml(const FStringList&);
  const FString setXmlFromWidget();
  void refreshWidgetDisplay();
  bool isAbstractEnabled() {return this->isEnabled();}
  const QStringList& getDescription() const { return description; }
  const QString& getHashKey() const {return hashKey; }

private:
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;

  const QList<QWidget*>& getComponentList() const { return componentList;}
  const QString& getDepth() const {return widgetDepth; }

};



class FColorButton :  public QWidget, virtual public FAbstractWidget
{
 Q_OBJECT

private:

  QPushButton *button;
  QString hashKey;
  QString widgetDepth;
  QStringList description;
  QString optionLabel;
  QList<FString> commandLineList;
  QList<QWidget*> componentList;


public:
  FColorButton( const char* text, const QString & color);
  QLabel *colorLabel;
  int buttonWidth()   const ;
  void setMinimumButtonWidth(const int w);
  void setWidgetFromXml(const FStringList&);
  void refreshWidgetDisplay();
  const FString setXmlFromWidget();
  bool isAbstractEnabled() {return this->isEnabled();}
  const QString& getHashKey() const {return hashKey; }
  const QList<QWidget*>& getComponentList() const { return componentList;}
  const QString& getDepth() const {return widgetDepth; }
  const QStringList& getDescription() const { return description; }

public slots:
  void changeColors();

};

class FPalette :  public QWidget, virtual public FAbstractWidget
{
  Q_OBJECT
  friend class FAbstractWidget;

  public:
    FPalette(const char* textR, const char* textG, const char* textB, int status , const QString &hashKey,const QStringList &description, const QString &commandLine, int buttonWidth=150);
    FPalette(const char* textR, const char* textG, const char* textB,  const QString &hashKey,const QStringList &description, const QString &commandLine):
      FPalette(textR, textG, textB, flags::defaultStatus|flags::defaultCommandLine,hashKey,description,commandLine) {}
    void setWidgetFromXml(const FStringList&);
    void refreshWidgetDisplay();
    void refreshComponent(short i);
    void setToolTip(const QString &);

    const FString setXmlFromWidget();
    void setMinimumButtonWidth(const int w);
    bool isAbstractEnabled() {return (this->isEnabled());}
    const QString& getHashKey() const {return hashKey; }
    const QList<QWidget*>& getComponentList() const { return componentList;}
    const QString& getDepth() const {return widgetDepth; }
    const QStringList& getDescription() const { return description; }

    FColorButton *button[3];

  private:
   QString hashKey;
   QString widgetDepth;
   QStringList description;
   QString optionLabel;
   QList<FString> commandLineList;
   QList<QWidget*> componentList;
   void refreshPaletteHash();
};


#endif
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "highlighter.h"
#include "fwidgets.h"

inline void Highlighter::createRulePattern(const QColor& color,  QFont::Weight weight,  const QStringList& list)
{
    HighlightingRule rule;
    QTextCharFormat classFormat;

    classFormat.setForeground(color);
    classFormat.setFontWeight(weight);
    rule.format = classFormat;

    for (const QString& a : list)
    {
        rule.pattern = QRegExp(a);
        highlightingRules.append(rule);
    }
}


Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::darkMagenta);
    keywordFormat.setFontWeight(QFont::Bold);

    for (FAbstractWidget* a : Abstract::abstractWidgetList)
    {
        rule.pattern = QRegExp("\\b"+a->getHashKey()+"\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    createRulePattern(Qt::darkBlue, QFont::Bold, {"\\bwidgetDepth\\b"});

    createRulePattern(QColor("maroon"), QFont::Bold, {"\\bgroup\\b", "\\btitleset\\b", "\\btrack\\b", "\\bmenu\\b", "\\bYCrCb\\b"});

    createRulePattern(QColor("orange"), QFont::Bold, {"\\bfile\\b", "\\bslide\\b"});

    createRulePattern(Qt::darkGreen, QFont::Black, {"\".*\""});

    createRulePattern(Qt::red, QFont::Black, {"<[/]?", ">", "data>", "system>", "recent>", "project>", "<\\?xml"});
}

void Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule: highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

}

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;

    void createRulePattern(const QColor& color,  QFont::Weight weight,  const QStringList& list);
};

#endif // HIGHLIGHTER_H

#include <QFile>
#include "dvda-author-gui.h"
#include "options.h"
#include "forms.h"


lplexPage::lplexPage()
{


  int lplexComboBoxWidth=100;
  int lplexLineEditWidth=250;

  QLabel *lplexVideoLabel = new QLabel(tr("TV standard"));

  lplexVideoType = new FComboBox({ "ntsc" , "pal" , "secam"},
                                 lplexFiles,
                                 "lplexVideoType",
                                 {"Lplex","TV standard"},
                                  "video" );

  lplexVideoType->setMinimumWidth(lplexComboBoxWidth);

  QLabel *lplexBackgroundLabel = new QLabel(tr("Path to background"));
  QToolButton *lplexBackgroundButton=new QToolDirButton;
  /* connect to ... */
  lplexBackgroundLineEdit = new FLineEdit(generateDatadirPath("black.jpg"),
                                          lplexFiles,
                                          "lplexJpegPath",
                                         {"Lplex", "Path to lplex background jpeg"},
                                          "jpeg");

  lplexBackgroundLineEdit->setMinimumWidth(lplexLineEditWidth);

  lplexScreenParameterBox = new FCheckBox("Use DVD-Audio display parameters",
                                          "lplexDisplayIsDVD-Audio",
                                         {"Lplex", "Use DVD-Audio display parameters"},
                                            {NULL},
                                            {
                                              lplexBackgroundLineEdit,
                                              lplexBackgroundLabel,
                                              lplexBackgroundButton,
                                              lplexVideoType,
                                              lplexVideoLabel
                                            });

  QLabel *lplexCreateLabel = new QLabel(tr("Authoring"));

  lplexCreateType = new FComboBox({ "lpcm" , "m2v" , "dvdstyler" , "mpeg" , "dvd" , "iso"} ,
                                  lplexFiles,
                                  "lplexCreateType",
                                 {"Lplex","Authoring type"},
                                  "create" );

  lplexCreateType->setMinimumWidth(lplexComboBoxWidth);

  QLabel *lplexMediaLabel = new QLabel(tr("Disc size"));

  lplexMediaType = new FComboBox( {"dvd+r" ,  "dvd-r" ,  "dl" ,  "none"},
                                  lplexFiles,
                                  "lplexMediaType",
                                 {"Lplex","Maximum disc size"},
                                  "media");

  lplexMediaType->setMinimumWidth(lplexComboBoxWidth);

  lplexDirLineEdit = new FLineEdit(tempdir,
                                   lplexFiles,
                                   "lplexDirPath",
                                   {"Lplex","Output everything to this directory"},
                                   "dir" );

  lplexDirLineEdit->setMinimumWidth(lplexLineEditWidth);
  QLabel *lplexDirLabel = new QLabel(tr("Path to output directory"));
  QToolButton *lplexDirButton=new QToolDirButton;
  lplexDirButton->setMaximumWidth(40);

  QGroupBox *lplexBox = new QGroupBox(tr("Video options"));
  QGridLayout *lplexLayout=new QGridLayout;

  lplexLayout->addWidget(lplexScreenParameterBox , 1,0);
  lplexLayout->addWidget(lplexVideoLabel, 2,0, Qt::AlignLeft);
  lplexLayout->addWidget(lplexVideoType, 2,0, Qt::AlignHCenter);
  lplexLayout->addWidget(lplexBackgroundLabel, 3, 0, Qt::AlignLeft);
  lplexLayout->addWidget(lplexBackgroundLineEdit , 3, 0, Qt::AlignRight);
  lplexLayout->addWidget(lplexBackgroundButton, 3,1,Qt::AlignLeft);
  lplexLayout->addWidget(lplexCreateLabel, 4, 0, Qt::AlignLeft);
  lplexLayout->addWidget(lplexCreateType,  4, 0, Qt::AlignHCenter);
  lplexLayout->addWidget(lplexMediaLabel,  5, 0, Qt::AlignLeft);
  lplexLayout->addWidget(lplexMediaType,   5, 0, Qt::AlignHCenter);
  lplexLayout->addWidget(lplexDirLabel, 6, 0, Qt::AlignLeft);
  lplexLayout->addWidget(lplexDirLineEdit, 6, 0, Qt::AlignRight);
  lplexLayout->addWidget(lplexDirButton, 6, 1, Qt::AlignLeft);

  lplexLayout->setColumnMinimumWidth(0, 450);

  lplexBox->setLayout(lplexLayout);


  /******    advanced ********/

  QLabel *lplexSpliceLabel = new QLabel(tr("Splice"));

  lplexSpliceType = new FComboBox({"seamless" , "discrete" , "padded" , "none"},
                                  flags::enabled|lplexFiles,
                                  "lplexSpliceType",
                                 {"Lplex","Physically structure track transition point"},
                                  "splice" );

  lplexSpliceType->setMinimumWidth(lplexComboBoxWidth);

  QLabel *lplexShiftLabel = new QLabel(tr("Move startpoints"));

  lplexShiftType = new FComboBox({"backward" , "forward" , "nearest" },
                                 flags::enabled|lplexFiles,
                                 "lplexShiftType",
                                 {"Lplex","Move seamless startpoints"},
                                 "shift");

  lplexShiftType->setMinimumWidth(lplexComboBoxWidth);


  lplexMd5AwareBox = new FCheckBox("Generate MD5 tags",
                                    flags::enabled|flags::lplexFiles,
                                   "lplexMd5Aware",
                                   {"Lplex","Generate MD5 tags"},
                                   "md5aware");

  lplexRescaleBox = new FCheckBox("Rescale images to TV standard",
                                  flags::enabled|lplexFiles,
                                  "lplexRescale",
                                { "Lplex","If jpegs sized for ntsc [pal] are being used\nto create a pal [ntsc] dvd, rescale them"},
                                  "rescale") ;

  lplexInfoDirLineEdit = new FLineEdit(tempdir,
                                       flags::enabled|lplexFiles,
                                       "lplexInfoDirPath",
                                       {"Lplex","Path to directory to be added\n to Lplex-made XTRA directory"},
                                       "infodir");

  lplexInfoDirLineEdit->setMinimumWidth(lplexLineEditWidth);
  QLabel *lplexInfoDirLabel = new QLabel(tr("Path to Info directory"));

  QToolButton *lplexInfoDirButton=new QToolDirButton;
  lplexInfoDirButton->setMaximumWidth(40);

  /* connect to  QFileDialog::getExistingDirectory(this, "Browse Info directory"); */

  lplexInfoDirBox = new FCheckBox("Add Info directory to disc",
                                  disabled,
                                  "lplexInfoDir",
                                  {"Lplex","Add Info directory\nto disc XTRA directory"},
                                    {
                                      lplexInfoDirLabel,
                                      lplexInfoDirLineEdit,
                                      lplexInfoDirButton
                                    });


  lplexInfofilesBox = new FCheckBox("Generate info files",
                                    "lplexInfoFiles",
                                    {"Lplex", "Make an 'XTRA' info folder"},
                                    {lplexInfoDirBox}) ;

  QGroupBox *lplexAdvancedBox= new QGroupBox(tr("Advanced parameters"));
  QGridLayout *lplexAdvancedLayout=new QGridLayout;

  lplexAdvancedLayout->addWidget(lplexSpliceLabel, 1, 0, Qt::AlignLeft);
  lplexAdvancedLayout->addWidget(lplexSpliceType,  1, 0, Qt::AlignHCenter);
  lplexAdvancedLayout->addWidget(lplexShiftLabel,   2, 0, Qt::AlignLeft);
  lplexAdvancedLayout->addWidget(lplexShiftType,    2, 0, Qt::AlignHCenter);
  lplexAdvancedLayout->addWidget(lplexMd5AwareBox, 3, 0);
  lplexAdvancedLayout->addWidget(lplexRescaleBox, 4, 0);
  lplexAdvancedLayout->addWidget(lplexInfofilesBox, 5, 0);
  lplexAdvancedLayout->addWidget(lplexInfoDirBox, 6, 0);
  lplexAdvancedLayout->addWidget(lplexInfoDirLabel, 7, 0, Qt::AlignLeft);
  lplexAdvancedLayout->addWidget(lplexInfoDirLineEdit, 7, 0, Qt::AlignRight);
  lplexAdvancedLayout->addWidget(lplexInfoDirButton, 7, 1, Qt::AlignLeft);

  lplexAdvancedLayout->setColumnMinimumWidth(0, 450);
  lplexAdvancedBox->setLayout(lplexAdvancedLayout);

  QVBoxLayout* mainLayout = new QVBoxLayout;
  FRichLabel *lplexLabel=new FRichLabel("Lplex options", ":/images/64x64/lplex.png");
  mainLayout->addWidget(lplexLabel);
  mainLayout->addWidget(lplexBox);
  mainLayout->addSpacing(20);
  mainLayout->addWidget(lplexAdvancedBox);
  mainLayout->addStretch(1);
  mainLayout->setMargin(20);
  setLayout(mainLayout);

  connect(lplexDirButton, SIGNAL(clicked(bool)), this, SLOT(on_lplexDirButton_clicked()));
  connect(lplexInfoDirButton, SIGNAL(clicked(bool)), this, SLOT(on_lplexInfoDirButton_clicked()));
  connect(lplexBackgroundButton, SIGNAL(clicked()), this, SLOT(on_lplexBackgroundButton_clicked()));
}

void lplexPage::on_lplexInfoDirButton_clicked()
{
    lplexInfoDirLineEdit->setText(QFileDialog::getExistingDirectory(this, lplexInfoDirBox->getDescription().join(": ")));
}

void lplexPage::on_lplexDirButton_clicked()
{
    lplexDirLineEdit->setText(QFileDialog::getExistingDirectory(this, lplexDirLineEdit->getDescription().join(": ")));
}

void lplexPage::on_lplexBackgroundButton_clicked()
{
    lplexBackgroundLineEdit->setText(QFileDialog::getOpenFileName(this, lplexBackgroundLineEdit->getDescription().join(": ")));
}









#ifndef LPLEX_H
#define LPLEX_H


#include "common.h"

class lplexPage : public common
{
    Q_OBJECT

public:
    lplexPage();

private slots:
   void on_lplexDirButton_clicked();
   void on_lplexInfoDirButton_clicked();
   void on_lplexBackgroundButton_clicked();

private:
    FComboBox *lplexVideoType, *lplexSpliceType, *lplexShiftType, *lplexCreateType, *lplexMediaType;
    FLineEdit *lplexBackgroundLineEdit, *lplexInfoDirLineEdit, *lplexDirLineEdit;
    FCheckBox *lplexScreenParameterBox, *lplexMd5AwareBox, *lplexInfofilesBox, *lplexRescaleBox, *lplexInfoDirBox;
};


#endif // LPLEX_H
/*

main.cpp  - Author a DVD-Audio DVD with dvda-author

This application uses Qt4.8 . Check Qt's licensing details on http://qt.nokia.com


Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> Feb 2009,2012

The latest version can be found at http://dvd-audio.sourceforge.net

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/



#include <QApplication>

#include "dvda-author-gui.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    char* s;
    if (argc > 1) s=argv[1];
    else s=(char*)"";
    MainWindow *mainWin=new MainWindow(s);

    mainWin->show();
    return app.exec();
}
#include "dvda.h"


/* AUTHOR NOTE




mainWindow.cpp  - Main Window for dvda-author-gui

This application uses Qt5.1 . Check Qt's licensing details on http://qt.nokia.com


Copyright Fabrice Nicol <fabnicol@users.sourceforge.net> Feb 2009,2012

The latest version can be found at http://dvd-audio.sourceforge.net

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


// createFontDataBase looks to be fast enough to be run on each launch.
// Should it slow down application launch on some platform, one option could be to launch it just once then on user demand


void MainWindow::createFontDataBase()
{
    QFontDatabase database;

    QDir dir;
    if (! dir.mkpath(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation)))) return;

    QString fontPath=common::generateDatadirPath("fonts");
    if (QFileInfo(fontPath).exists()) return;

    QStringList fontList=database.families();
    int rank=0;
    foreach (const QString &family, fontList)
    {
        QString style;
        QStringListIterator i(QStringList()<< "Normal" << "Regular" << "Light" << "Italic" << "Medium" << "Bold");
        while ((i.hasNext()) && (!database.styles(family).contains(style=i.next(), Qt::CaseInsensitive)));
        if (!style.isEmpty())
        {
            QStringList sizeList;

            foreach (int points, database.smoothSizes(family, style))
                sizeList << QString::number(points) ;

            if (!sizeList.isEmpty())
            {
                QString fontSizes=common::generateDatadirPath(QString(family+".sizes").toUtf8());
                common::writeFile(fontSizes, sizeList);
            }
            else
                fontList.removeAt(rank);
        }
        else
            fontList.removeAt(rank);

        rank++;
     }

     common::writeFile(fontPath, fontList);

}


MainWindow::MainWindow(char* projectName)
{
  createFontDataBase();

  setGeometry(QRect(200, 200,1000,400));
  recentFiles=QStringList()<<QString("default") ;

  dvda_author=new dvda;

  dialog=new options(dvda_author);

  dialog->setParent(dvda_author, Qt::Window);
  dvda_author->parent=this;
  dvda_author->projectName=projectName;

  console=new Console(this);

  timer = new QTimer(this);

  connect(timer, &QTimer::timeout, [this]() {feedConsole(true);});

  connect(&(dvda_author->process),
                 &QProcess::readyReadStandardOutput,
                      [&]
                          {
                              feedConsole(false);
                              timer->start(50);
                          });

  createActions();
  createMenus();
  createToolBars();

  settings = new QSettings("dvda-author", "Free Software Inc");

  if ((settings->value("default").isValid())
                &&
     (!settings->value("default").toString().isEmpty()))
        dvda_author->setCurrentFile(settings->value("default").toString());
  else
    {
        dvda_author->setCurrentFile(projectName);
        settings->setValue("default",projectName);
    }


  setCentralWidget(dvda_author);

  dvda_author->mainTabWidget->addActions(actionList);
  dvda_author->setContextMenuPolicy(Qt::ActionsContextMenu);
  dvda_author->mainTabWidget->setContextMenuPolicy(Qt::ActionsContextMenu);

  bottomDockWidget=new QDockWidget;
  bottomTabWidget=new QTabWidget;
  consoleDialog=  new QTextEdit;
  //consoleDialog->setMinimumSize(800,600);
  bottomTabWidget->addTab(dvda_author->outputTextEdit, tr("Messages"));

  fileTreeViewDockWidget= new QDockWidget;
  fileTreeViewDockWidget->setWidget(dvda_author->fileTreeView);
  fileTreeViewDockWidget->setMinimumHeight((unsigned) (height()*0.3));
  fileTreeViewDockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
  fileTreeViewDockWidget->hide();
  addDockWidget(Qt::LeftDockWidgetArea, fileTreeViewDockWidget);


  Abstract::initializeFStringListHashes();
                        //dvda_author->RefreshFlag =0
  Abstract::refreshOptionFields();
                      //dvda_author->RefreshFlag = ParseXml for poorly understood reasons
  // do not put before as we want to control RefreshFlag in a connection generated by configureOptions
   configureOptions();
  // NOTE: Using only FCheckBoxes. Change this if other FAbstractWidget subclasses are to be used


  for (FCheckBox* a : displayWidgetList+behaviorWidgetList)
      {
         if (settings->value(a->getHashKey()).isValid())
             a->setChecked(settings->value(a->getHashKey()).toBool());
      }

  dvda_author->initializeProject();
  bottomTabWidget->setCurrentIndex(0);

  QToolButton *clearBottomTabWidgetButton=new QToolButton;
  const QIcon clearOutputText = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
  clearBottomTabWidgetButton->setIcon(clearOutputText);

  connect(clearBottomTabWidgetButton, &QToolButton::clicked, [this] { on_clearOutputTextButton_clicked();});

  QGroupBox *stackedBottomWidget=new QGroupBox;
  QHBoxLayout *stackedBottomWidgetLayout=new QHBoxLayout;
  stackedBottomWidgetLayout->addWidget(clearBottomTabWidgetButton);
  stackedBottomWidgetLayout->addWidget(bottomTabWidget);
  stackedBottomWidget->setLayout(stackedBottomWidgetLayout);
  bottomDockWidget->setWidget(stackedBottomWidget);

  addDockWidget(Qt::BottomDockWidgetArea, bottomDockWidget);

  setWindowIcon(QIcon(":/images/dvda-author.png"));
  setWindowTitle("dvda-author interface  "+ QString(VERSION) +" version");
}

void MainWindow::on_clearOutputTextButton_clicked()
{
    if (console->isVisible())
    {
        console->raise(); // to refocus if triggering main app button otherwise redundant
    }

    qobject_cast<QTextEdit*>(bottomTabWidget->currentWidget())->clear();
 }


void MainWindow::updateRecentFileActions()
{
QMutableStringListIterator i(recentFiles);

 while (i.hasNext())
 {
   if (!QFile::exists(i.next())) i.remove();
 }


 for (int j=0 ; j<MaxRecentFiles ; ++j)
 {
   if (j < recentFiles.count())
   {
     QString  text = tr("&%1 %2").arg(j+1).arg(strippedName(recentFiles[j]));
     recentFileActions[j]->setText(text);
     recentFileActions[j]->setData(QVariant(recentFiles[j]));
     recentFileActions[j]->setVisible(true);
   } else

   {
    recentFileActions[j]->setVisible(false);
   }

 }

 separatorAction->setVisible(!recentFiles.isEmpty());
}



QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void MainWindow::createMenus()
{
 fileMenu = menuBar()->addMenu("&File");
 editMenu = menuBar()->addMenu("&Edit");
 processMenu = menuBar()->addMenu("&Process");
 optionsMenu = menuBar()->addMenu("&Configure");
 aboutMenu = menuBar()->addMenu("&Help");

 fileMenu->addAction(openAction);
 fileMenu->addAction(saveAction);
 fileMenu->addAction(saveAsAction);
 fileMenu->addAction(closeAction);

 separatorAction=fileMenu->addSeparator();
 for (int i=0; i<MaxRecentFiles ; ++i)
    fileMenu->addAction(recentFileActions[i]);
 fileMenu->addSeparator();
 fileMenu->addAction(exitAction);

 editMenu->addAction(displayAction);
 editMenu->addAction(displayOutputAction);
 editMenu->addAction(displayFileTreeViewAction);
 editMenu->addAction(displayManagerAction);
 editMenu->addAction(displayConsoleAction);
 editMenu->addAction(clearOutputTextAction);
 editMenu->addAction(editProjectAction);

 processMenu->addAction(burnAction);
 processMenu->addAction(encodeAction);
 processMenu->addAction(decodeAction);

 optionsMenu->addAction(optionsAction);
 optionsMenu->addAction(configureAction);

 aboutMenu->addAction(helpAction);
 aboutMenu->addAction(aboutAction);

}



void MainWindow::createActions()
{
  openAction = new QAction(tr("&Open .dvp project file"), this);
  openAction->setShortcut(QKeySequence("Ctrl+O"));
  openAction->setIcon(QIcon(":/images/open-project.png"));
  connect(openAction, SIGNAL(triggered()), dvda_author, SLOT(on_openProjectButton_clicked()));

  saveAction = new QAction(tr("&Save"), this);
  saveAction->setShortcut(QKeySequence("Ctrl+S"));
  saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  connect(saveAction, &QAction::triggered, [&] {dvda_author->saveProject(true);});

  saveAsAction = new QAction(tr("S&ave project file as..."), this);
  saveAsAction->setIcon(QIcon(":/images/document-save-as.png"));
  connect(saveAsAction, SIGNAL(triggered()), dvda_author, SLOT(requestSaveProject()));

  closeAction = new QAction(tr("&Close .dvp project file"), this);
  closeAction->setShortcut(QKeySequence("Ctrl+W"));
  closeAction->setIcon(QIcon(":/images/document-close.png"));
  connect(closeAction, SIGNAL(triggered()), dvda_author, SLOT(closeProject()));

  burnAction = new QAction(tr("&Burn files to disc"), this);
  burnAction->setShortcut(QKeySequence("Ctrl+B"));
  burnAction->setIcon(QIcon(":/images/burn.png"));
  connect(burnAction, SIGNAL(triggered()), dvda_author, SLOT(on_cdrecordButton_clicked()));

  encodeAction = new QAction(tr("Start c&reating disc files"), this);
  encodeAction->setShortcut(QKeySequence("Ctrl+R"));
  encodeAction->setIcon(QIcon(":/images/encode.png"));
  connect(encodeAction, SIGNAL(triggered()), dvda_author, SLOT(run()));

  decodeAction = new QAction(tr("&Decode disc to generate wav files"), this);
  decodeAction->setIcon(QIcon(":/images/decode.png"));
  connect(decodeAction, SIGNAL(triggered()), dvda_author, SLOT(extract()));

  optionsAction = new QAction(tr("&Processing options"), this);
  optionsAction->setShortcut(QKeySequence("Ctrl+P"));
  optionsAction->setIcon(QIcon(":/images/configure.png"));
  connect(optionsAction, SIGNAL(triggered()), this, SLOT(on_optionsButton_clicked()));

  configureAction= new QAction(tr("&Configure interface"), this);
  configureAction->setIcon(QIcon(":/images/configure-toolbars.png"));
  connect(configureAction, SIGNAL(triggered()), this, SLOT(configure()));

  helpAction = new QAction(tr("&Help"), this);
  helpAction->setShortcut(QKeySequence("Ctrl+H"));
  helpAction->setIcon(QIcon(":/images/help-contents.png"));
  connect(helpAction, SIGNAL(triggered()), dvda_author, SLOT(on_helpButton_clicked()));

  displayAction = new QAction(tr("&Show maximized/normal"), this);
  displayAction->setIcon(QIcon(":/images/show-maximized.png"));
  connect(displayAction, SIGNAL(triggered()), this, SLOT(showMainWidget()));

  displayManagerAction = new QAction(tr("Show/Close project &manager"), this);
  const QIcon iconViewList = QIcon(QString::fromUtf8( ":/images/manager.png"));
  displayManagerAction->setIcon(iconViewList);
  connect(displayManagerAction, SIGNAL(triggered()), dvda_author, SLOT(on_openManagerWidgetButton_clicked()));

  displayConsoleAction = new QAction(tr("Show/Close console"), this);
  const QIcon consoleIcon = QIcon(QString::fromUtf8( ":/images/console.png"));
  displayConsoleAction->setIcon(consoleIcon);
  connect(displayConsoleAction, &QAction::triggered, [&] {console->on_displayConsoleButton_clicked(this);});

  editProjectAction=new QAction(tr("Edit current project"), this);
  editProjectAction->setShortcut(QKeySequence("Ctrl+E"));
  editProjectAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
  connect(editProjectAction, SIGNAL(triggered()), this, SLOT(on_editProjectButton_clicked()));

  displayOutputAction  = new QAction(tr("Show/Close messages"), this);
  const QIcon displayOutput = QIcon(QString::fromUtf8( ":/images/display-output.png"));
  displayOutputAction->setIcon(displayOutput);
  connect(displayOutputAction, &QAction::triggered,  [this] {bottomDockWidget->setVisible(!bottomDockWidget->isVisible());});

  displayFileTreeViewAction  = new QAction(tr("Show/Close file manager"), this);
  const QIcon displayFileTreeView = QIcon(QString::fromUtf8( ":/images/view-list-tree.png"));
  displayFileTreeViewAction->setIcon(displayFileTreeView);
  connect(displayFileTreeViewAction, SIGNAL(triggered()), this, SLOT(on_displayFileTreeViewButton_clicked()));

  clearOutputTextAction = new QAction(tr("Clear message/console &window"), this);
  const QIcon clearOutputText = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
  clearOutputTextAction->setIcon(clearOutputText);
  connect(clearOutputTextAction, &QAction::triggered,  [this] {on_clearOutputTextButton_clicked();});

  exitAction = new QAction(tr("&Exit"), this);
  exitAction->setIcon(QIcon(":/images/application-exit.png"));
  exitAction->setShortcut(QKeySequence("Ctrl+Q"));
  connect(exitAction, &QAction::triggered,  [this] { exit(1);});

  aboutAction=new QAction(tr("&About"), this);
  aboutAction->setIcon(QIcon(":/images/about.png"));

  connect(aboutAction, &QAction::triggered,  [this]  {
                                                                                          QUrl url=QUrl::fromLocalFile( dvda_author->generateDatadirPath("about.html") );
                                                                                           browser::showPage(url);
                                                                                         });

  for (int i=0; i < MaxRecentFiles ; i++)
  {
    recentFileActions[i] = new QAction(this);
    recentFileActions[i]->setVisible(false);
    connect(recentFileActions[i], SIGNAL(triggered()), dvda_author, SLOT(openProjectFile()));
  }

  QAction* separator[3];
  for (int i=0; i < 3; i++)
    {
      separator[i] = new QAction(this) ;
      separator[i]->setSeparator(true);
    }

  actionList << openAction << saveAction << saveAsAction << closeAction << exitAction << separator[0] <<
                burnAction << encodeAction << decodeAction << separator[1] <<
                displayOutputAction << displayFileTreeViewAction << displayManagerAction << displayConsoleAction <<
                clearOutputTextAction <<  editProjectAction << separator[2] << configureAction <<
                optionsAction << helpAction << aboutAction;

}

void MainWindow::configure()
{
     contentsWidget->setVisible(true);
}

void MainWindow::on_optionsButton_clicked()
{
  dialog->setVisible(!dialog->isVisible());
}



void MainWindow::on_displayFileTreeViewButton_clicked(bool isHidden)
{
   fileTreeViewDockWidget->setVisible(isHidden);
   dvda_author->on_frameTab_changed(dvda_author->mainTabWidget->currentIndex());
   dvda_author->project[AUDIO]->importFromMainTree->setVisible(isHidden);
   dvda_author->project[VIDEO]->importFromMainTree->setVisible(isHidden);
 }

void MainWindow::on_displayFileTreeViewButton_clicked()
{
  bool isHidden=fileTreeViewDockWidget->isHidden();
  on_displayFileTreeViewButton_clicked(isHidden);
}


void MainWindow::createToolBars()
{
 fileToolBar = addToolBar(tr("&File"));
 fileToolBar->setIconSize(QSize(22,22));

 editToolBar=addToolBar(tr("&Edit"));
 editToolBar->setIconSize(QSize(22,22));

 processToolBar = addToolBar(tr("&Process"));
 processToolBar->setIconSize(QSize(22,22));

 optionsToolBar = addToolBar(tr("&Options"));
 optionsToolBar->setIconSize(QSize(22,22));

 aboutToolBar = addToolBar(tr("&Help"));
 aboutToolBar->setIconSize(QSize(22,22));

 fileToolBar->addAction(openAction);
 fileToolBar->addAction(saveAction);
 fileToolBar->addAction(saveAsAction);
 fileToolBar->addAction(closeAction);
 fileToolBar->addAction(exitAction);
 fileToolBar->addSeparator();

 editToolBar->addAction(displayAction);
 editToolBar->addAction(displayOutputAction);
 editToolBar->addAction(displayFileTreeViewAction);
 editToolBar->addAction(displayManagerAction);
 editToolBar->addAction(displayConsoleAction);
 editToolBar->addAction(editProjectAction);

 processToolBar->addAction(burnAction);
 processToolBar->addAction(encodeAction);
 processToolBar->addAction(decodeAction);

 optionsToolBar->addAction(optionsAction);
 optionsToolBar->addAction(configureAction);

 aboutToolBar->addAction(helpAction);
 aboutToolBar->addAction(aboutAction);


}

void MainWindow::on_editProjectButton_clicked()
{

    if (dvda_author->projectName.isEmpty()) return;
    editWidget = new QMainWindow(this);
    editWidget->setWindowTitle(tr("Editing project ")+dvda_author->projectName.left(8)+"..."+dvda_author->projectName.right(12));
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    editWidget->menuBar()->addMenu(fileMenu);

     const char* keys[]={"New", "Open", "Save", "Save as...", "Refresh project", "Save and exit", "Exit"};
     const char* seq[]={"Ctrl+N","Ctrl+O","Ctrl+S","Ctrl+T","Ctrl+R","Ctrl+E","Ctrl+Q"};
     int j=0;

    for (const char* k:  keys)
    {
        actionHash[k]=new QAction(tr(k), this);
        fileMenu->addAction(actionHash[k]);
        actionHash[k]->setShortcut(QKeySequence(seq[j++]));
    }

    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    editor = new QTextEdit;
    editor->setFont(font);

    highlighter = new Highlighter(editor->document());

    if (dvda_author->projectName.isEmpty()) return;
   QFile  *file=new QFile(dvda_author->projectName);

   if (file->open(QFile::ReadWrite| QFile::Text))
   {
       editor->setPlainText(file->readAll());
       file->close();
   }
   // do not capture file by reference!
   connect(actionHash["New"],
                 &QAction::triggered,
                 [this] { editor->clear();});

   connect(actionHash["Open"],
                 &QAction::triggered,
                 [file, this]
                                  {
                                     file->~QFile();
                                     dvda_author->on_openProjectButton_clicked() ;
                                     editWidget->~QMainWindow();
                                     on_editProjectButton_clicked();
                                   });

   connect(actionHash["Save"],
                 &QAction::triggered,
                 [file, this]
                                 {
                                    file->open(QFile::Truncate |QFile::WriteOnly| QFile::Text);
                                    file->write(editor->document()->toPlainText().toUtf8()) ;
                                    file->close();
                                    dvda::RefreshFlag = dvda::RefreshFlag |UpdateTree | ParseXml;
                                    dvda_author->initializeProject(true);
                                  });

   connect(actionHash["Save as..."],
                  &QAction::triggered,
                  [file, this] {saveProjectAs(file);});

   connect(actionHash["Refresh project"],
                 &QAction::triggered,
                 [file, this]
                                 {
                                    dvda_author->saveProject(true);
                                    if (file->open(QFile::ReadWrite |  QFile::Text))
                                       {
                                           editor->clear();
                                           editor->setPlainText(file->readAll());
                                           file->close();
                                       }
                                  });

   connect(actionHash["Save and exit"],
                  &QAction::triggered,
                 [this]
                         {
                            actionHash["Save"]->trigger();
                            actionHash["Exit"]->trigger();
                         });

   connect(actionHash["Exit"],
                 &QAction::triggered,
                 [file, this]
                                  {
                                     file->~QFile();
                                     editWidget->~QMainWindow() ;
                                   });
   editWidget->setCentralWidget(editor);
   editWidget->setGeometry(200,200,600,800);
   editWidget->show();

}


void MainWindow::saveProjectAs(QFile* file)
{
    QString newstr=QFileDialog::getSaveFileName(this, tr("Save project as..."), QDir::currentPath(), tr("dvp projects (*.dvp)"));
    if (newstr.isEmpty()) return;
    if (newstr == dvda_author->projectName)
    {
        actionHash["Save"]->trigger();
        return;
    }

    if  (QFileInfo(newstr).isFile())
    {
          if (QMessageBox::No == QMessageBox::warning(this, tr("Overwrite file?"), tr("File will be overwritten.\nPress Yess to confirm, No to cancel operation."), QMessageBox::Yes|QMessageBox::No))
             return;
          else
          {
                 QFile newfile(newstr);
                 newfile.remove();
          }
    }
    if (file->rename(newstr) ==false) return;
    dvda_author->projectName=newstr;
    if (file->open(QFile::WriteOnly | QFile::Truncate | QFile::Text))
    {
       file->write(editor->document()->toPlainText().toUtf8()) ;
    }
    file->close();
    dvda::RefreshFlag = dvda::RefreshFlag |UpdateTree | ParseXml;
    dvda_author->initializeProject(true);
}

void MainWindow::configureOptions()
{
    /* plain old data types must be 0-initialised even though the class instance was new-initialised. */

    contentsWidget = new QDialog(this);
    contentsWidget->setVisible(false);

    QGroupBox *displayGroupBox =new QGroupBox(tr("Display"));

    closeButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout *layout=new QVBoxLayout;
    QVBoxLayout *dlayout=new QVBoxLayout;
    QVBoxLayout *blayout=new QVBoxLayout;

    defaultFileManagerWidgetLayoutBox=new FCheckBox("Display file manager",
                                                                                                                           flags::noCommandLine|flags::checked,
                                                                                                                           "fileManagerDisplay",
                                                                                                                           {"Interface", "Display file manager on left panel"});

    defaultProjectManagerWidgetLayoutBox=new FCheckBox("Display project manager",
                                                                                                                                 flags::noCommandLine|flags::checked,
                                                                                                                                 "projectManagerDisplay",
                                                                                                                                 {"Interface", "Display project manager on right panel"});

    defaultConsoleLayoutBox=new FCheckBox("Display console as bottom panel tab",
                                                                                                       flags::noCommandLine|flags::checked,
                                                                                                       "launchConsoleAsTab",
                                                                                                       {"Interface", "Add tab to bottom output panel\non launching console"});

    defaultFullScreenLayout=new FCheckBox("Full screen",
                                                                                                      flags::noCommandLine|flags::unchecked,
                                                                                                      "fullScreenDisplay",
                                                                                                      {"Interface", "Display interface full screen on launch"});


    defaultLplexActivation=new FCheckBox("Activate video zone editing using Lplex",
                                                                                                  flags::noCommandLine|flags::checked,
                                                                                                  "activateLplex",
                                                                                                  {"Interface", "Create DVD-Video zone\nusing Lplex"});

    defaultOutputTextEditBox=new FCheckBox("Display message panel",
                                                                                                  flags::noCommandLine|flags::checked,
                                                                                                  "outputTextEdit",
                                                                                                  {"Interface", "Display message panel"});

    QGroupBox *behaviorGroupBox =new QGroupBox(tr("Save/Launch"));

    defaultSaveProjectBehavior=new FCheckBox("Save .dvp project automatically",
                                                                                                      flags::noCommandLine|flags::checked,
                                                                                                      "saveProjectBehavior",
                                                                                                      {"Interface", "Saves project if a tab content is changed\nand on exiting the interface"});

    defaultLoadProjectBehavior=new FCheckBox("Load .dvp project on launch",
                                                                                                      flags::noCommandLine|flags::checked,
                                                                                                      "loadProjectBehavior",
                                                                                                      {"Interface", "Load latest .dvp project on launch"});

   displayWidgetList <<  defaultFileManagerWidgetLayoutBox
                       << defaultProjectManagerWidgetLayoutBox
                       << defaultConsoleLayoutBox
                       << defaultOutputTextEditBox
                       << defaultFullScreenLayout
                       << defaultLplexActivation;

    behaviorWidgetList   << defaultSaveProjectBehavior
                                         << defaultLoadProjectBehavior;

    for (FCheckBox* a : displayWidgetList)     dlayout->addWidget(a);
    for (FCheckBox* a : behaviorWidgetList)   blayout->addWidget(a);

    displayGroupBox->setLayout(dlayout);
    behaviorGroupBox->setLayout(blayout);
    layout->addWidget(displayGroupBox);
    layout->addWidget(behaviorGroupBox);
    layout->addWidget(closeButton);

    contentsWidget->setLayout(layout);
    connect(closeButton, &QDialogButtonBox::accepted,
                        [this]  {
                                        for (FCheckBox* a : displayWidgetList + behaviorWidgetList)
                                            settings->setValue(a->getHashKey(), a->isChecked());
                                        contentsWidget->accept();
                                }
                  );

    /* note on connection syntax
     * Here the new Qt5 connection syntax should be used with care and disregarded when both an action button and an FCheckBox activate a slot as the slots
     * are overloaded (which could possibly be rewritten) and a) the action button uses the argumentless slot whilst
     * b) the boolean version of slots must be used by the FcheckBox. The new Qt5 syntax cannot work this out as it does not manage overloading. */

    connect(closeButton, &QDialogButtonBox::rejected, contentsWidget, &QDialog::reject);
    connect(closeButton, &QDialogButtonBox::accepted, [this]
                                                                                                         {
                                                                                                               if (    (defaultSaveProjectBehavior->isChecked())
                                                                                                                    || (QMessageBox::Yes == QMessageBox::warning(this, tr("Save project"), tr("Project has not been saved.\nPress Yes to save current .dvp project file now\nor No to close dialog without saving project."), QMessageBox::Yes|QMessageBox::No))
                                                                                                                   )
                                                                                                                         dvda_author->saveProject();
                                                                                                          });

    connect(defaultFileManagerWidgetLayoutBox, SIGNAL(toggled(bool)), this, SLOT(on_displayFileTreeViewButton_clicked(bool)));
    connect(defaultProjectManagerWidgetLayoutBox, SIGNAL(toggled(bool)), dvda_author, SLOT(on_openManagerWidgetButton_clicked(bool)));
    connect(defaultLplexActivation, &FCheckBox::toggled, this, &MainWindow::on_activate_lplex);
    connect(defaultConsoleLayoutBox, &FCheckBox::toggled, [this] {console->detachConsole(defaultConsoleLayoutBox->isChecked(), this);});
    connect(defaultFullScreenLayout, SIGNAL(toggled(bool)), this, SLOT(showMainWidget(bool)));
    connect(defaultOutputTextEditBox, &FCheckBox::toggled, [this] {bottomDockWidget->setVisible(defaultOutputTextEditBox->isChecked());});
    connect(defaultLoadProjectBehavior, &FCheckBox::toggled, [this] {if (defaultLoadProjectBehavior->isChecked()) dvda_author->RefreshFlag |=  ParseXml;});

    setWindowTitle(tr("Configure dvda-author GUI"));
    setWindowIcon(QIcon(":/images/dvda-author.png"));
}



void MainWindow::on_activate_lplex(bool active)
{
    dvda_author->mainTabWidget->setTabEnabled(VIDEO, active);
    dialog->contentsWidget->item(flags::lplexRank)->setHidden(!active);
    dialog->contentsWidget->setCurrentRow(0);
}

void MainWindow::showMainWidget(bool full)
{
  if (full)
  {
      setWindowState(Qt::WindowFullScreen);
      displayAction->setIcon(QIcon(":/images/show-normal.png"));
  }
  else
  {
      setWindowState(Qt::WindowNoState);
      displayAction->setIcon(QIcon(":/images/show-maximized.png"));
  }

}

void MainWindow::showMainWidget()
{
      showMainWidget(this->windowState() != Qt::WindowFullScreen);
}




void MainWindow::feedConsole(bool initialized)
{

    if (!dvda_author->process.atEnd())
    {
        char tab[30][200]={0};
        char data[6000]={0};

        if (!initialized)
        {
          uint clen=0;
            for (int i=0; i < 30; i++)
           {
               dvda_author->process.readLine(tab[i], 200*sizeof(char));
               uint len=strlen(tab[i]);
               memcpy(data+clen, tab[i], len);
               clen+=len;
           }
        }
        else
            dvda_author->process.readLine(data, 200*sizeof(char));

           QRegExp reg("\\[INF\\]([^\\n]*)\n");
           QRegExp reg2("\\[PAR\\]([^\\n]*)\n");
           QRegExp reg3("\\[MSG\\]([^\\n]*)\n");
           QRegExp reg4("\\[ERR\\]([^\\n]*)\n");
           QRegExp reg5("\\[WAR\\]([^\\n]*)\n");
           QRegExp reg6("(^.*licenses/.)");

            QString text=QString(data).replace(reg6, (QString) HTML_TAG(navy) "\\1</span><br>");
            text= text.replace(reg, (QString) INFORMATION_HTML_TAG "\\1<br>");
            text=text.replace(reg2, (QString) PARAMETER_HTML_TAG "\\1<br>");
            text=text.replace(reg3, (QString) MSG_HTML_TAG "\\1<br>");
            text=text.replace(reg4, (QString) ERROR_HTML_TAG "\\1<br>");
            text=text.replace(reg5, (QString) WARNING_HTML_TAG "\\1<br>");
            text=text.replace("Group", (QString) HTML_TAG(red) "Group</span>");
            text=text.replace("Title", (QString) HTML_TAG(green) "Title</span>");
            text=text.replace("Track", (QString) HTML_TAG(blue) "Track</span>");
            text=text.replace(QRegExp("([0-9]+)([ ]+)([0-9]*) / ([0-9]*)([ ]+)([0-9]*)"),
                              HTML_TAG(red) "\\1</span>"+QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;")
                              + HTML_TAG(green) "\\3</span>" +HTML_TAG(green) "/ <b>\\4</b></span>"
                              +QString("&nbsp;&nbsp;")+HTML_TAG(blue) "\\6</span>&nbsp;&nbsp;&nbsp;&nbsp;");
            consoleDialog->insertHtml(text=text.replace("\n", "<br>"));
            consoleDialog->moveCursor(QTextCursor::End);
            console->appendHtml(text);

    }
    else (timer->stop());

 }


#include <QFile>

#include "dvda.h"
#include "forms.h"
#include "options.h"
#include "videoplayer.h"
#include "browser.h"
#include "dvda-author-gui.h"



standardPage::standardPage()
{
    normTypeBox=new QGroupBox(tr("TV Standard"));
    aspectRatioBox=new QGroupBox(tr("Screen Size"));

    normTypeLineEdit = new FLineEdit("PAL",
                                     "normType",
    {"Screen", "TV Standard"},
    "norm");

    aspectRatioLineEdit = new FLineEdit("16:9",
                                        "aspectRatio",
    {"Screen", "Screen size"},
    {"aspect"});

    normTypeLineEdit->setAlignment(Qt::AlignCenter);
    aspectRatioLineEdit->setAlignment(Qt::AlignCenter);

    normWidget=new QListWidget;
    normWidget->setViewMode(QListView::IconMode);
    normWidget->setIconSize(QSize(78,78));
    normWidget->setMovement(QListView::Static);

    normWidget->setFixedWidth(286);
    normWidget->setFixedHeight(104);
    normWidget->setSpacing(12);
    normWidget->setCurrentRow(0);
    normWidget->setToolTip(tr("Select TV standard"));

    QListWidgetItem *ntscButton = new QListWidgetItem(normWidget);
    ntscButton->setIcon(QIcon(":/images/64x64/ntsc.png"));
    ntscButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *palButton = new QListWidgetItem(normWidget);
    palButton->setIcon(QIcon(":/images/64x64/pal.png"));
    palButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *secamButton = new QListWidgetItem(normWidget);
    secamButton->setIcon(QIcon(":/images/64x64/secam.png"));
    secamButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    aspectRatioWidget=new QListWidget;
    aspectRatioWidget->setViewMode(QListView::IconMode);
    aspectRatioWidget->setIconSize(QSize(78,78));
    aspectRatioWidget->setMovement(QListView::Static);
    aspectRatioWidget->setFixedWidth(286);
    aspectRatioWidget->setFixedHeight(112);
    aspectRatioWidget->setSpacing(12);
    aspectRatioWidget->setCurrentRow(0);
    aspectRatioWidget->setToolTip(tr("Select screen aspect ratio (Width:Height)"));

    QListWidgetItem *_16x9_Button = new QListWidgetItem(aspectRatioWidget);
    _16x9_Button->setIcon(QIcon(":/images/64x64/16x9.png"));
    _16x9_Button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *_16x10_Button = new QListWidgetItem(aspectRatioWidget);
    _16x10_Button->setIcon(QIcon(":/images/64x64/16x10.png"));
    _16x10_Button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *_4x3_Button = new QListWidgetItem(aspectRatioWidget);
    _4x3_Button->setIcon(QIcon(":/images/64x64/4x3.png"));
    _4x3_Button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QGridLayout *v1Layout = new QGridLayout;
    QGridLayout *v2Layout = new QGridLayout;
    v1Layout->addWidget(normWidget, 1,1,Qt::AlignHCenter);
    v1Layout->addWidget(normTypeLineEdit,2,1,Qt::AlignHCenter);

    normTypeBox->setLayout(v1Layout);
    v2Layout->addWidget(aspectRatioWidget,1,1, Qt::AlignHCenter);
    v2Layout->addWidget(aspectRatioLineEdit,2,1,Qt::AlignHCenter);

    aspectRatioBox->setLayout(v2Layout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    FRichLabel *mainLabel=new FRichLabel("DVD-Audio screen display", ":/images/64x64/pal.png");
    mainLayout->addWidget(mainLabel);
    mainLayout->addWidget(normTypeBox);
    mainLayout->addWidget(aspectRatioBox);
    mainLayout->setMargin(20);
    setLayout(mainLayout);


    connect(normWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changeNorm(QListWidgetItem*,QListWidgetItem*)));

    connect(aspectRatioWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changeAspectRatio(QListWidgetItem*,QListWidgetItem*)));
}

void standardPage::changeAspectRatio(QListWidgetItem *current,QListWidgetItem *previous)
{
    if (!current) current=previous;
    if (!current) return;

    switch(aspectRatioWidget->row(current))
    {
    case 0:
        aspectRatioMsg="16:9";break;

    case 1:
        aspectRatioMsg="16:10";break;

    case 2:
        aspectRatioMsg="4:3"; break;

    }

    aspectRatioLineEdit->setText(aspectRatioMsg);
}

void standardPage::changeNorm(QListWidgetItem*current,QListWidgetItem*previous)
{
    if (!current) current=previous;
    if (!current) return;

    switch(normWidget->row(current))
    {
    case 0:
        standardMsg="NTSC"; break;

    case 1:
        standardMsg="PAL"; break;

    case 2:
        standardMsg="SECAM"; break;
    }

    normTypeLineEdit->setText(standardMsg);
}


optionsPage::optionsPage()
{

    mainBox = new QGroupBox(tr("Disc options"));
    mkisofsButton = new QToolDirButton(tr("Select or type in .iso filename for disc image.\nThis file will be used for disc burning."));

    QLabel *mkisofsLabel = new QLabel(tr("Path to ISO file:"));
    mkisofsLineEdit = new FLineEdit(tempdir+QDir::separator()+"dvd.iso",
                                    createIso,
                                    "mkisofsPath",
    {"Burn", "Path to ISO image"},
    "mkisofs");


    dvdwriterComboBox = new FComboBox("",
                                      createDisc,
                                      "dvdwriterPath",
    {"Burn", "Path to DVD writer device"},
    "cdrecord");

    dvdwriterComboBox->setMinimumContentsLength(35);


    cdrecordBox= new FCheckBox("Burn to DVD-Audio/Video disc",
                               "burnDisc",
    {"Burn", "Burn disc image to DVD"},
                                {dvdwriterComboBox});

    mkisofsBox =new FCheckBox("Create ISO file",
                              flags::checked|flags::enabled|flags::dvdaCommandLine,
                              "runMkisofs",
    {"Burn", "Create disc image using mkisofs"},
                              {
                                  mkisofsButton,
                                  cdrecordBox,
                                  mkisofsLabel,
                                  mkisofsLineEdit
                              });

    playbackBox= new FCheckBox("Launch playback on loading disc",
                               "playback",
    {"Launch","Launch playback on loading"},
                               "autoplay");

    QGridLayout *gridLayout=new QGridLayout;
    FRichLabel* mainLabel= new FRichLabel("Disc options", ":/images/64x64/configure.png");

    gridLayout->addWidget(mkisofsBox,1,1);
    gridLayout->addWidget(mkisofsLabel,2,1, Qt::AlignRight);
    gridLayout->addWidget(mkisofsLineEdit,2,2);
    gridLayout->addWidget(mkisofsButton,2,3);
    gridLayout->addWidget(cdrecordBox,3,1);
    gridLayout->addWidget(dvdwriterComboBox,4,2);
    gridLayout->addWidget(playbackBox,5,1);
    gridLayout->setRowMinimumHeight(6,50);
    mainBox->setLayout(gridLayout);
    QVBoxLayout *mainLayout=new QVBoxLayout;
    mainLayout->addWidget(mainLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(mainBox);
    mainLayout->addStretch(1);
    mainLayout->setMargin(20);
    setLayout(mainLayout);

    connect(mkisofsButton, SIGNAL(clicked()), this, SLOT(on_mkisofsButton_clicked()));
    connect(cdrecordBox, SIGNAL(toggled(bool)), this, SLOT(dvdwriterCheckEditStatus(bool)));
}

void optionsPage::dvdwriterCheckEditStatus(bool checked)
{
    dvdwriterComboBox->clear();

    if (!checked)
    {
        dvdwriterComboBox->setEditable(false);
        return;
    }

    QStringList dvdwriterPaths;
    dvdwriterPaths=generateDvdwriterPaths().dvdwriterNameList;

    if ((dvdwriterPaths.isEmpty()) || dvdwriterPaths[0].isEmpty())
        dvdwriterComboBox->setEditable(true);
    else
        dvdwriterComboBox->addItems(dvdwriterPaths);

        dvdwriterComboBox->setItemIcon(0, style()->standardIcon(QStyle::SP_DriveDVDIcon));
        dvdwriterComboBox->setIconSize(QSize(32,32));
}


/* Under *nix platforms cdrecord should not be placed in a root-access directory
 * but under the default /opt/schily/bin install directory
 * unless it is locally available under ../bindir
 * For the latter method define CDRECORD_LOCAL_PATH
 * or just define any CDRECORD_PATH adding an explicit path string
 *
 * If using QtCreator under Windows, place bindir/ adjacent to the makefiles and the release/ or debug/ folder
 * under the build directory
 */

struct optionsPage::dvdwriterAddress optionsPage::generateDvdwriterPaths()
{
    QProcess process;
#ifndef CDRECORD_PATH
 #ifdef CDRECORD_LOCAL_PATH
    process.start(QDir::toNativeSeparators(QDir::currentPath ()+"/bindir/"+ QString("cdrecord.exe")),  QStringList() << "-scanbus");   //"k3b");
 #else
    #define CDRECORD_PATH "/opt/schily/bin"
     process.start(QString(CDRECORD_PATH) + QString("/cdrecord"),  QStringList() << "-scanbus");
 #endif
#endif

    QStringList dvdwriterNameList=QStringList(), dvdwriterBusList=QStringList();

    if (process.waitForFinished(800))
    {
        QByteArray array=process.readAllStandardOutput();

        int start=100, startIndex, endIndex;
        while (start < array.size())
        {
            startIndex=array.indexOf(") ", start)+2;
            endIndex=array.indexOf("\n", startIndex);

            if (array.at(startIndex)   != '*')
            {
                QString name = QString(array.mid(startIndex, endIndex-startIndex)).remove('\'');
                if (name.contains("DVD", Qt::CaseInsensitive))
                {
                    dvdwriterNameList << name;
                    dvdwriterBusList <<   QString(array.mid(startIndex-11, 5)).remove('\'');
                }

            }
            start=endIndex+1;
        }

    }
    else
    {
        QMessageBox::warning(this, tr("cdrecord"), tr("cdrecord could not be located or crashed."));
    }

    dvdwriterAddress S={dvdwriterBusList, dvdwriterNameList};
    return S;
}

void optionsPage::on_mkisofsButton_clicked()
{
    QString path=QFileDialog::getSaveFileName(this,  tr("Set mkisofs iso file"), "");
    mkisofsLineEdit->setText(path);
}


advancedPage::advancedPage()
{
    paddingBox = new FCheckBox("Pad wav files",
                               flags::disabled|flags::dvdaCommandLine,
                               "padding",
                              {"Audio processing", "Pad wav files"},
                               "padding");

    pruneBox = new FCheckBox("Cut silence at end of wav files ",
                             flags::disabled|flags::dvdaCommandLine,
                             "prune",
                            {"Audio processing", "Cut silence at end of wav files"},
                             "prune");

    Q2ListWidget controlledObjects={{paddingBox, pruneBox}} ;

    fixWavOnlyBox=new FCheckBox("Only fix wav headers,\ndo not process audio",
                                flags::disabled|flags::dvdaCommandLine,
                                "fixWavOnly",
                               {"Audio processing", "Only fix wav headers"},
                                "fixwav",
                                &controlledObjects);

    setWhatsThisText(fixWavOnlyBox, 78,79);

    fixwavBox = new FCheckBox("Fix corrupt wav headers\nand process audio",
                              "fixwav",
                            {"Audio processing", "Fix corrupt wav headers"},
                              "fixwav",
                              &controlledObjects);

    setWhatsThisText(fixwavBox, 82,90);

    startsectorLabel = new QLabel(tr("&Start sector"));
    startsectorLineEdit = new FLineEdit("281", "startsector", {"Audio processing","Start sector number"},"startsector");
    startsectorLineEdit->setMaxLength(4);
    startsectorLineEdit->setFixedWidth(50);
    startsectorLabel->setBuddy(startsectorLineEdit);
    startsectorLabel->setAlignment(Qt::AlignRight);

    setWhatsThisText(startsectorLabel, 93,96);
    startsectorLineEdit->setAlignment(Qt::AlignRight);

    QLabel *extraAudioFiltersLabel = new QLabel(tr("Display audio formats"));
    extraAudioFiltersLineEdit = new QLineEdit;
    extraAudioFiltersLabel->setBuddy(extraAudioFiltersLineEdit);
    extraAudioFiltersLabel->setAlignment(Qt::AlignRight);
    extraAudioFiltersLineEdit->setText(common::extraAudioFilters.join(","));
    extraAudioFiltersLineEdit->setMaximumWidth(120);

    soxBox= new FCheckBox("Enable multiformat input",
                          "sox",
                        {"Audio processing", "Use SoX to convert audio files"},
                          "sox") ;

    setWhatsThisText(soxBox, 31, 41);
    setWhatsThisText(extraAudioFiltersLabel, 72, 75);

    QGridLayout *advancedLayout =new QGridLayout;
    advancedLayout->addWidget(fixWavOnlyBox,0,0);
    advancedLayout->addWidget(fixwavBox,1,0);
    advancedLayout->addWidget(pruneBox,2,1, Qt::AlignLeft);
    advancedLayout->addWidget(paddingBox,3,1, Qt::AlignLeft);
    advancedLayout->setColumnMinimumWidth(2,200);
    QGroupBox *fixwavGroupBox = new QGroupBox("Fix wav files");
    fixwavGroupBox->setLayout(advancedLayout);

    QGridLayout *extraLayout =  new QGridLayout;
    extraLayout->addWidget(soxBox,1,0);
    extraLayout->setRowMinimumHeight(1, 70);
    extraLayout->addWidget(extraAudioFiltersLabel, 2,0,Qt::AlignLeft);
    extraLayout->addWidget(extraAudioFiltersLineEdit,2, 1,Qt::AlignLeft);
    extraLayout->addWidget(startsectorLabel,3,0,Qt::AlignLeft);
    extraLayout->addWidget(startsectorLineEdit,3,1,Qt::AlignLeft);
    extraLayout->setColumnMinimumWidth(2,250);


    QVBoxLayout *mainLayout = new QVBoxLayout;
    FRichLabel *mainLabel=new FRichLabel("Audio processing", ":/images/64x64/audio-processing.png");
    mainLayout->addWidget(mainLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(fixwavGroupBox);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(extraLayout);
    mainLayout->addStretch(1);
    mainLayout->setMargin(20);
    setLayout(mainLayout);

    connect(extraAudioFiltersLineEdit, SIGNAL(textEdited(const QString&)), this, SLOT(on_extraAudioFilters_changed(const QString&)));

}


void advancedPage::on_extraAudioFilters_changed(const QString &line)
{
    static QStringList keep;
    keep=common::extraAudioFilters;

    QStringList work = line.split(',');

    QStringListIterator i(work);
    QRegExp reg;

    reg.setPattern("^[\\*]\\..+$");
    QString a;
    while (i.hasNext())
    {
        if (!reg.exactMatch(a=i.next()))
        {
            common::extraAudioFilters=keep;
            return;
        }
    }

    common::extraAudioFilters=work;
}




audioMenuPage::audioMenuPage(dvda* parent, standardPage* standardTab)
{
    /* createFListFrame(parent,  List, hashKey, description, commandLine1, commandLineType, separator, tags) */


    QGroupBox *menuStyleBox = new QGroupBox(tr("DVD-Audio menu style"));
    slidesBox = new QGroupBox(tr("Menu slideshow"));

    QStringList nmenuList;
    for (int i=0; i <= 10; i++) nmenuList << QString::number(i);

    nmenuFComboBox=new FComboBox(nmenuList,
                                 "numberOfMenus",
    {"Audio menu", "Number of menus"},
                                 "nmenus");

    nmenuFComboBox->setMaximumWidth(50);

    QIcon *iconSlides=new QIcon(":/images/64x64/still.png");
    QIcon *iconSoundtracks=new QIcon(":/images/audio_file_icon.png");
    QIcon *iconScreentext=new QIcon(":/images/64x64/text-rtf.png");

    slides= new FListFrame(parent,
                           parent->fileTreeView,
                           importFiles,
                           "audioMenuSlides",
    {"Audio menu","DVD-Audio menu slides"},
                             "topmenu-slides",
                           dvdaCommandLine|flags::enabled,
                            {",", ":"},
                            {"slide" , "menu"},
                           0,
                           iconSlides);

    slides->model=parent->model;
    slides->slotList=nmenuFComboBox->signalList;

    soundtracks= new FListFrame(parent,
                                parent->fileTreeView,
                                importFiles,
                                "audioMenuTracks",
                               {"Audio menu", "DVD-Audio menu tracks"},
                                "topmenu-soundtracks",
                                dvdaCommandLine|flags::enabled,
                                {",", ":"},
                                { "track" , "menu"},
                                1,
                                iconSoundtracks,
                                slides->embeddingTabWidget);

    soundtracks->model=parent->model;
    soundtracks->slotList=nmenuFComboBox->signalList;

    screentext= new FListFrame(parent,
                               NULL,
                               typeIn,
                               "audioMenuText",
    {"Audio menu","DVD-Audio menu text"},
                               "screentext",
                               dvdaCommandLine|flags::enabled,
                                { ",", ":"},
                                {"trackname" , "group"},
                               2,
                               iconScreentext,
                               slides->embeddingTabWidget);

    screentext->slotList=nmenuFComboBox->signalList;
    screentext->importFromMainTree->setVisible(false);

    QGroupBox *audioMenuBox = new QGroupBox(tr("DVD-Audio menu"));
    QGridLayout *audioMenuLayout=new QGridLayout;

    loopVideoBox= new FCheckBox(tr("Loop menu"),
                                "loopVideo",
    {"Audio menu","Loop menu video"},
                                "loop");

    menuStyleFComboBox=new FComboBox({"standard", "hierarchical", "active"},
                                     "menuStyle",
    {"Audio menu", "Menu style"},
                                     "menustyle");

    menuStyleFComboBox->setMaximumWidth(150);

    QList<QIcon> *iconList = new QList<QIcon>;
    *iconList << QIcon(":/images/track-icon-leadingsquare.png")
              << QIcon(":/images/track-icon-underline.png")
              << QIcon(":/images/track-icon-button.png") ;

    highlightFormatFComboBox=new FComboBox({"  leading square", "  underline",  "  button box"},
                                            {"-1", "0",  "1"}, // translation into xml
                                           flags::defaultStatus,
                                           "highlightFormat",
                                          {"Audio menu", "Highlight format"},
                                           "highlightformat",
                                           iconList);

    QString fontPath=generateDatadirPath("fonts");

    fontList=QStringList();

    if (common::readFile(fontPath, fontList) == 0)
        QMessageBox::warning(this, tr("Error"), tr("Failed to open font list file in ") +fontPath);

    fontFComboBox=new FComboBox(fontList,
                                "font",
    {"Audio menu","Font"},
                                "fontname");

    fontSizeFComboBox=new FComboBox(QStringList(),
                "fontSize",
    {"Audio menu", "Font size"},
                "fontsize");

    readFontSizes(0);

    fontFComboBox->setMaximumWidth(250);

    fontSizeFComboBox->setMaximumWidth(50);
    fontSizeFComboBox->setCurrentIndex(4);

    setWhatsThisText(menuStyleBox, 4, 9);

    audioMenuLineEdit = new FLineEdit(common::tempdir+QDir::separator()+QString::fromUtf8("audiobackground.png"),
                                      "audioBackgroundPath",
                                     {"Audio menu", "Path to DVD-Audio menu background"},
                                      "blankscreen");

    QLabel *audioMenuLabel = new QLabel(tr("Menu background"));
    audioMenuButton = new QToolDirButton(tr("Select customized menu background image (*.png)"));
    openAudioMenuButton = new QToolDirButton(tr("Open menu background file in image viewer"), actionType::OpenFolder);

    setWhatsThisText(slidesBox, 12, 20);

    FPalette *palette=new FPalette("Track",
                                   "Highlight",
                                   "Album/Group",
                                   "topmenuPalette",
    {"Audio menu", "Top menu colors"},
                                   "topmenu-palette");

    QGridLayout* createMenuLayout=new QGridLayout;

    createMenuLayout->addWidget(nmenuFComboBox, 0, 1);
    createMenuLayout->addWidget(new QLabel(tr("menu(s)")), 0, 2);
    createMenuLayout->setColumnMinimumWidth(3,300);

    audioMenuLabel->setBuddy(audioMenuLineEdit);
    audioMenuLabel->setToolTip(tr("Path to image of DVD-Audio menu background"));

    audioMenuLayout->addWidget(audioMenuLabel, 1,0);
    audioMenuLayout->addWidget(audioMenuLineEdit, 1,1);
    audioMenuLayout->addWidget(audioMenuButton, 1,3);
    audioMenuLayout->addWidget(openAudioMenuButton, 2,3);
    audioMenuLayout->setColumnMinimumWidth(2,60);

    QGridLayout *menuStyleLayout=new QGridLayout;
    menuStyleLayout->setRowMinimumHeight(1, 25);
    menuStyleLayout->setRowMinimumHeight(6, 25);

    menuStyleLayout->addWidget(loopVideoBox,0,0);
    menuStyleLayout->addWidget(new QLabel(tr("Layout style")),2,0);
    menuStyleLayout->addWidget(menuStyleFComboBox,3,0);
    menuStyleLayout->addWidget(new QLabel(tr("Track selection style")),2,1);
    menuStyleLayout->addWidget(highlightFormatFComboBox,3,1);
    menuStyleLayout->addWidget(new QLabel(tr("Font")),4,0);
    menuStyleLayout->addWidget(fontFComboBox, 5, 0);
    menuStyleLayout->addWidget(new QLabel(tr("Font size")),4,1);
    menuStyleLayout->addWidget(fontSizeFComboBox, 5, 1);

    QGroupBox *paletteGroupBox=new QGroupBox("Color palette");
    QGridLayout *paletteLayout=new QGridLayout;

    paletteLayout->addWidget(palette->button[0], 1, 0, Qt::AlignHCenter);
    paletteLayout->addWidget(palette->button[1], 1, 1, Qt::AlignHCenter);
    paletteLayout->addWidget(palette->button[2], 1, 2, Qt::AlignHCenter);
    paletteGroupBox->setLayout(paletteLayout);
    paletteGroupBox->setFixedHeight(100);

    // Take care to add palettes in enabled/disabled widget lists. This is caused by the fact that palette layout is controlled from superordinate layout,
    // not internally to contructor. So a disabled QGroupBox only disables palette display, not status, as opposed to fwidgets with internal layouts.


    audioMenuCheckBox = new FCheckBox("Create",
                                      "audioMenu",
    {"Audio menu","Create DVD-Audio menu"},
                                       {
                                          nmenuFComboBox,
                                          audioMenuLineEdit,
                                          audioMenuButton,
                                          openAudioMenuButton,
                                          menuStyleBox,
                                          slidesBox,
                                          paletteGroupBox,
                                          palette,
                                          standardTab
                                      });

    createMenuLayout->addWidget(audioMenuCheckBox, 0,0);
    QGridLayout *slidesLayout=new QGridLayout;
    slidesLayout->setColumnMinimumWidth(1, 300);
    slidesLayout->addWidget(slides->tabBox, 1, 1);
    slidesLayout->addWidget(slides->fileLabel, 0,1,1,1,Qt::AlignHCenter);
    slidesLayout->addWidget(slides->controlButtonBox, 1,3,1,1,Qt::AlignLeft);
    slidesLayout->addWidget(slides->importFromMainTree, 1,0,1,1,Qt::AlignRight);
    slidesLayout->addWidget(soundtracks->tabBox, 1, 1);
    slidesLayout->addWidget(soundtracks->fileLabel, 0, 1,1,1,Qt::AlignHCenter);
    slidesLayout->addWidget(soundtracks->controlButtonBox, 1, 3,1,1,Qt::AlignLeft);
    slidesLayout->addWidget(soundtracks->importFromMainTree, 1,0,1,1,Qt::AlignRight);
    slidesLayout->addWidget(screentext->tabBox, 1, 1);
    slidesLayout->addWidget(screentext->fileLabel, 0, 1,1,1,Qt::AlignHCenter);
    slidesLayout->addWidget(screentext->controlButtonBox, 1, 3,1,1,Qt::AlignLeft);

    // Avoir resizing and flickering on hiding import button
    slidesLayout->setColumnMinimumWidth(0, 40);
    slidesLayout->setColumnStretch(0,1);

    QVBoxLayout* audioMenuBoxLayout=new QVBoxLayout;
    audioMenuBoxLayout->addLayout(createMenuLayout);
    audioMenuBoxLayout->addLayout(audioMenuLayout);
    audioMenuBox->setLayout(audioMenuBoxLayout);
    menuStyleBox->setLayout(menuStyleLayout);
    slidesBox->setLayout(slidesLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    FRichLabel *mainLabel=new FRichLabel("DVD-Audio menu", ":/images/64x64/audio-menu.png");
    mainLayout->addWidget(mainLabel);
    mainLayout->addWidget(audioMenuBox);
    mainLayout->addWidget(menuStyleBox);
    mainLayout->addWidget(paletteGroupBox, Qt::AlignHCenter);

    slidesBox->setVisible(false);
    slidesButton=new QPushButton(style()->standardIcon(QStyle::SP_ArrowForward), "Open slideshow");
    slidesButton->setMaximumSize(150,40);

    mainLayout->addWidget(slidesButton, Qt::AlignLeft);
    mainLayout->addStretch(1);
    mainLayout->setMargin(20);

    setLayout(mainLayout);

    on_frameTab_changed(0);
    connect(openAudioMenuButton, SIGNAL(clicked(bool)), this, SLOT(launchImageViewer()));
    connect(audioMenuButton, SIGNAL(clicked()), this, SLOT(on_audioMenuButton_clicked()));
    connect(slides->embeddingTabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_frameTab_changed(int )));
    connect(slidesButton, SIGNAL(clicked()), this, SLOT(on_slidesButton_clicked()));
    connect(audioMenuCheckBox, SIGNAL(clicked(bool)), this, SLOT(setMinimumNMenu(bool)));
    connect(fontFComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(readFontSizes(int)));

}

void audioMenuPage::readFontSizes(int rank)
{
    QString sizeFileName =fontList.at(rank) + QString(".sizes");
    QString sizeFilePath = common::generateDatadirPath(sizeFileName);
    QStringList fontSizeList;
    common::readFile(sizeFilePath, fontSizeList);
    if (!fontSizeList.isEmpty())
    {
        fontSizeFComboBox->clear();
        fontSizeFComboBox->addItems(fontSizeList);
    }

    fontSizeFComboBox->setCurrentIndex(4);
}

void audioMenuPage::setMinimumNMenu(bool value)
{
    nmenuFComboBox->setCurrentIndex((int) value);
}

void audioMenuPage::on_slidesButton_clicked()
{
    static int counter;

    if (counter == 0)
    {
        newWidget= new QDialog(this, Qt::Window);
        newLayout=new QVBoxLayout;
        newLayout->addWidget(slidesBox);
        newWidget->setLayout(newLayout);
        newWidget->setWindowTitle("DVD-Audio menu");

    }

    counter++;
    slidesButton->setText((counter % 2)?"Close slideshow":"Open slideshow");
    slidesBox->setVisible(counter % 2);
    newWidget->setVisible(counter % 2);
    newWidget->raise();

}



void audioMenuPage::on_frameTab_changed(int index)
{
    slides->controlButtonBox->setVisible(index == 0);
    soundtracks->controlButtonBox->setVisible(index == 1);
    screentext->controlButtonBox->setVisible(index == 2);
    slides->fileLabel->setVisible(index == 0);
    soundtracks->fileLabel->setVisible(index == 1);
    screentext->fileLabel->setVisible(index == 2);
    slides->importFromMainTree->setVisible(index == 0);
    soundtracks->importFromMainTree->setVisible(index == 1);
}


void audioMenuPage::launchImageViewer()
{
    v = new ImageViewer(audioMenuLineEdit->text());
    v->setGeometry(400,300, 200/3*16,600);
    v->show();
}



void audioMenuPage::on_audioMenuButton_clicked()
{
    QString path=QFileDialog::getOpenFileName(this,  tr("Set DVD-Audio background image"), "*.png",tr("Image files (*.png)"));
}

videoMenuPage::videoMenuPage()
{
    int buttonwidth=70;
    QGroupBox *videoImportBox = new QGroupBox(tr("DVD-Video import"));
    QGridLayout *videoImportLayout=new QGridLayout;

    videoImportLineEdit = new FLineEdit(tempdir+QDir::separator()+QString::fromUtf8("VIDEO_TS"),
                                        "videoImport",
                                       {"Video menu", "Path to DVD-Video directory"},
                                        "videodir");

    QLabel *videoImportLabel = new QLabel(tr("Video directory"));
    QToolDirButton *videoImportButton;

    videoImportButton = new QToolDirButton(tr("Import DVD-Video directory to project"));
    videoImportButton->setFixedWidth(buttonwidth);

    QToolDirButton *openVideoImportButton = new QToolDirButton(tr("Open DVD-Video directory "), actionType::OpenFolder);
    openVideoImportButton->setFixedWidth(buttonwidth);

    videoImportCheckBox= new FCheckBox("Import DVD-Video",
                                       "videoMenu",
    {"Video menu","Import DVD-Video "},
    {
                                           videoImportLineEdit,
                                           videoImportButton,
                                           openVideoImportButton,
                                           videoImportLabel
                                       });

    videoImportLayout->addWidget(videoImportCheckBox, 1,0);
    videoImportLayout->addWidget(videoImportLabel, 2,0);
    videoImportLayout->addWidget(videoImportLineEdit, 2,1);
    videoImportLayout->addWidget(videoImportButton, 2,3);
    videoImportLayout->addWidget(openVideoImportButton, 3,3);
    videoImportLayout->setColumnMinimumWidth(2,60);
    videoImportBox->setLayout(videoImportLayout);

    QLabel *videoMenuImportLabel = new QLabel(tr("Authored DVD-Video menu"));
    QToolDirButton *videoMenuImportButton = new QToolDirButton(tr("Import DVD-Video menu"));
    videoMenuImportButton->setFixedWidth(buttonwidth);

    videoMenuImportLineEdit = new FLineEdit(tempdir+QDir::separator()+QString::fromUtf8("VIDEO_TS/VIDEO_TS.VOB"),
                                            flags::disabled,
                                            "videoMenuImport",
    {"Video menu","Import DVD-Video menu"},
                                            "videomenu");

    Q2ListWidget *audioExportRadioBoxEnabledObjects = new Q2ListWidget ;
    *audioExportRadioBoxEnabledObjects=
    {
        {NULL},
        { videoMenuImportLineEdit,  videoMenuImportButton, videoMenuImportLabel },
        {NULL}
    };

    audioExportRadioBox =  new FRadioBox(
    {"Hybrid disc", "No DVD-Video menu" ,"Import authored menu", "Export DVD-Audio menu" },
                "hybridate",
               { "Video menu","Create DVD-Audio/Video hybrid"},
    {"hybridate", "hybridate", "hybridate-export-menu"},
                audioExportRadioBoxEnabledObjects);

    QGroupBox *audioExportBox = new QGroupBox(tr("DVD-Audio/Video hybrid"));
    QGridLayout *audioExportLayout=new QGridLayout;

    audioExportCheckBox = new FCheckBox("Create DVD-Audio/Video hybrid disc",
                                        "createHybrid",
    {"Video menu","Create hybrid DVD-Audio/Video disc"},
    {audioExportRadioBox},
    {videoImportBox});

    audioExportLayout->addWidget(audioExportCheckBox, 1,0);
    audioExportLayout->addWidget(audioExportRadioBox, 2,0, Qt::AlignHCenter);
    audioExportLayout->setColumnMinimumWidth(1,250);

    audioExportBox->setLayout(audioExportLayout);

    QGridLayout *videoMenuImportLayout=new QGridLayout;
    videoMenuImportLayout->addWidget(videoMenuImportLabel, 1,0);
    videoMenuImportLayout->addWidget(videoMenuImportLineEdit, 1,1);
    videoMenuImportLayout->addWidget(videoMenuImportButton, 1,3);
    videoMenuImportLayout->setColumnMinimumWidth(2,60);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    FRichLabel *mainLabel=new FRichLabel("DVD-Video menu", ":/images/64x64/video-menu.png");
    mainLayout->addWidget(mainLabel);

    mainLayout->addWidget(videoImportBox);
    mainLayout->addStretch(1);
    mainLayout->addWidget(audioExportBox);
    mainLayout->addLayout(videoMenuImportLayout);
    mainLayout->addStretch(1);
    mainLayout->setMargin(20);
    setLayout(mainLayout);

    connect(openVideoImportButton, SIGNAL(clicked(bool)), this, SLOT(on_openVideoImportButton_clicked()));
    connect(videoImportButton, SIGNAL(clicked()), this, SLOT(on_videoImportButton_clicked()));
    connect(videoMenuImportButton, SIGNAL(clicked()), this, SLOT(on_videoMenuImportButton_clicked()));
}

void videoMenuPage::on_videoMenuImportButton_clicked()
{
    QString path=QFileDialog::getOpenFileName(this, tr("Select DVD-Video menu"),  QDir::currentPath(),  tr("VOB files (*.VOB)"));
    videoMenuImportLineEdit->setText(path);
}

void videoMenuPage::on_openVideoImportButton_clicked()
{
    openDir(videoImportLineEdit->text());
}

void videoMenuPage::on_videoImportButton_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,  tr("Import DVD-Video directory"));
    videoImportLineEdit->setText(path);
}

videolinkPage::videolinkPage()
{


    QStringList range=QStringList();
    for (int i=0; i < 100; i++) range<<QString::number(i);

    videolinkSpinBox = new FComboBox(range,
                                     "videolinkRank",
                                    {"Video link", "Rank of linked video titleset"},
                                     "T");

    videolinkSpinBox->setEnabled(true);

    videolinkSpinBox->setMaximumWidth(50);

    setWhatsThisText(videolinkSpinBox, 44 ,46);

    QLabel *videolinkLabel = new QLabel(tr("Rank of video titleset "));
    videolinkLabel->setBuddy(videolinkSpinBox);

    videoZoneLineEdit = new FLineEdit(tempdir + QDir::separator()+QString::fromUtf8("VIDEO_TS"),
                                      "videoZonePath",
    {"Video link","Path to VIDEO_TS linked to"},
                                      "V");

    videoZoneButton = new  QToolDirButton(tr("Select customized menu background image (*.png)"));

    videolinkCheckBox = new FCheckBox("Link Audio zone\nto Video zone",
                                      "videolink",
                                     {"Video link", "Link Audio zone to Video zone"},
                                    {
                                          videolinkSpinBox,
                                          videoZoneLineEdit,
                                          videoZoneButton
                                      });

    QLabel *videoZoneLabel = new QLabel(tr("Video directory"));
    videoZoneLabel->setBuddy(videoZoneLineEdit);

    QGridLayout *videolinkLayout=new QGridLayout;

    videolinkLayout->addWidget(videolinkCheckBox, 1,0);
    videolinkLayout->addWidget(videolinkLabel, 2,0);
    videolinkLayout->addWidget(videolinkSpinBox, 2,1);
    videolinkLayout->addWidget(videoZoneLabel, 4,0);
    videolinkLayout->addWidget(videoZoneLineEdit, 4,1);
    videolinkLayout->addWidget(videoZoneButton, 4,2);
    videolinkLayout->setColumnMinimumWidth(2,60);
    videolinkLayout->setRowMinimumHeight(0, 50);
    videolinkLayout->setVerticalSpacing(50);

    mainBox =new QGroupBox(tr("Video linking"));
    setWhatsThisText(mainBox,23,28);

    QVBoxLayout  *allLayout = new QVBoxLayout;
    mainBox->setLayout(videolinkLayout);
    FRichLabel *mainLabel=new FRichLabel("Video linking", ":/images/64x64/link.png");
    allLayout->addWidget(mainLabel);
    allLayout->addSpacing(30);
    allLayout->addWidget(mainBox);
    allLayout->addStretch(1);
    allLayout->setMargin(20);
    setLayout(allLayout);

    connect(videoZoneButton, SIGNAL(clicked()), this, SLOT(on_videolinkButton_clicked()));
#if 0
    connect(videolinkSpinBox, SIGNAL(currentIndexChanged(int)), this, SLOT(titlesetLink(int)));
   #endif
}


void videolinkPage::on_videolinkButton_clicked()
{
    QString path;
    do
        path=QFileDialog::getExistingDirectory(this, "Select VIDEO_TS", QDir::currentPath(),   QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    while ((path.midRef(path.lastIndexOf(QDir::separator())+1) != "VIDEO_TS") && (!path.isEmpty()))  ;

    videoZoneLineEdit->setText(path);

    int count=0;
    QDir dir(path);
    foreach (QString file, dir.entryList({"*.VOB"}, QDir::Files)) count++;

    QStringList range=QStringList();
    for (int i=0; i < count; i++) range<<QString::number(i);

    videolinkSpinBox->clear();
    videolinkSpinBox->insertItems(0, range);

}

#if 0
void videolinkPage::titlesetLink(int x)
{
    x=2;
    //common::videolinkRank=QString::number(x);
}

#endif

outputPage::outputPage(options* parent)
{
    QGroupBox *logGroupBox = new QGroupBox(tr("dvda-author log parameters"));
    QGridLayout *logLayout=new QGridLayout;
    logButton = new QToolDirButton(tr("Browse or type in filename for dvda-author log file."));


    openLogButton = new QToolDirButton(tr("  &Read log"), actionType::BrowseFile);

    openLogButton->setToolTip(tr("Open log text file in text editor."));

    openHtmlLogButton = new QToolDirButton(tr("  Browse &Html log"), actionType::BrowseFile);

    openHtmlLogButton->setToolTip(tr("Open log.html file in browser."));

    debugBox = new FCheckBox(tr("Debugging-level verbosity"),
                             flags::disabled|flags::dvdaCommandLine,
                             "debug",
                            {"Console","Use debug-level verbosity"},
                             "debug" );

    veryverboseBox = new FCheckBox(tr("Increased verbosity"),
                                   flags::disabled|flags::dvdaCommandLine,
                                   "veryverbose",
                                  {"Console","Use enhanced verbosity"},
                                   "veryverbose");

    htmlFormatBox = new FCheckBox(tr("Html format"),
                                  flags::disabled|flags::dvdaCommandLine,
                                  "htmlFormat",
                                {"Console","Output html log"},
                                  "loghtml");

    logrefreshBox=new FCheckBox(tr("Refresh log"),
                                flags::disabled|flags::dvdaCommandLine,
                                "logrefresh",
                                {"Console","Erase prior logs on running"},
                                "logrefresh");


    logBox = new FCheckBox(tr("&Log file"),
                           "log",
                            {"Console", "Create log file"},
                           {
                               logButton,
                               openLogButton,
                               openHtmlLogButton,
                               debugBox,
                               veryverboseBox,
                               htmlFormatBox,
                               logrefreshBox
                           }) ;

    setWhatsThisText(logBox, 99,101);

    logLayout->addWidget(logBox,1,1);
    logLayout->addWidget(logButton,1,1,1,1,Qt::AlignRight);
    logLayout->addWidget(openLogButton,2,1,1,1,Qt::AlignRight);
    logLayout->addWidget(debugBox,3,1);
    logLayout->addWidget(veryverboseBox,4,1);
    logLayout->addWidget(htmlFormatBox,5,1);
    logLayout->addWidget(openHtmlLogButton,5,1,1,1,Qt::AlignRight);
    logLayout->addWidget(logrefreshBox,6,1);

    logLayout->setRowMinimumHeight(0,20);
    logLayout->setRowMinimumHeight(5,20);
    logLayout->setRowMinimumHeight(6,30);
    logLayout->setColumnMinimumWidth(0,30);
    logLayout->setColumnMinimumWidth(2,300);
    logGroupBox->setLayout(logLayout);

    targetDirLabel = new QLabel(tr("Output directory"));
    targetDirButton = new QToolDirButton(tr("Browse output directory for DVD-Audio disc files."));
    openTargetDirButton = new QToolDirButton(tr("Open output directory for DVD-Audio disc files."), actionType::OpenFolder);
    targetDirLineEdit = new FLineEdit(tempdir+QDir::separator()+"output", "targetDir", {"DVD-A file directory"}, "o");

    Q2ListWidget *createDVDFilesEnabledObjects= new Q2ListWidget;
    Q2ListWidget *createDVDFilesDisabledObjects=new Q2ListWidget;
    *createDVDFilesEnabledObjects =
    {
        { targetDirButton ,
          openTargetDirButton ,
          targetDirLabel,
          targetDirLineEdit ,
          parent->optionsTab ,
          parent->advancedTab->fixwavBox ,
          parent->advancedTab->soxBox ,
          parent->audioMenuTab ,
          parent->videoMenuTab ,
          parent->videolinkTab ,
          parent->standardTab ,
          parent->stillTab,
          parent->lplexTab,
        },
        {
            parent->advancedTab->fixWavOnlyBox
        }
    };

    *createDVDFilesDisabledObjects =
    {
        {
            parent->advancedTab->fixWavOnlyBox
        },
        {
            NULL//parent->advancedTab->fixwavBox : useless
        }
    };

    createDVDFilesRadioBox = new FRadioBox({ "Output mode" , "Create DVD files", "No output"},
                                           "createDVDFiles",
                                           {"Output", "Create DVD Files"},
                                           { "" , "no-output"},
                                           createDVDFilesEnabledObjects,
                                           createDVDFilesDisabledObjects);

    parent->advancedTab->fixWavOnlyBox->disabledObjects=new Q2ListWidget;
    //parent->advancedTab->fixWavOnlyBox->disabledObjects->append(createDVDFilesRadioBox);

    setWhatsThisText(createDVDFilesRadioBox, 104, 105);

    QHBoxLayout *targetLayout = new QHBoxLayout;
    targetLayout->addWidget(createDVDFilesRadioBox);
    targetLayout->addStretch(4);

    QGridLayout *targetDirLineEditLayout=new QGridLayout;
    targetDirLineEditLayout->addWidget(targetDirLabel,1,0);
    targetDirLineEditLayout->addWidget(targetDirLineEdit,1,1);
    targetDirLineEditLayout->addWidget(targetDirButton, 1,2);
    targetDirLineEditLayout->addWidget(openTargetDirButton, 1,3, Qt::AlignLeft);
    targetDirLineEditLayout->setColumnMinimumWidth(0,150);

    QVBoxLayout *mainTargetLayout=new QVBoxLayout;
    mainTargetLayout->addLayout(targetLayout);
    mainTargetLayout->addLayout(targetDirLineEditLayout);

    QGroupBox *outputGroupBox = new QGroupBox(tr("DVD-Audio disc files"));
    outputGroupBox->setLayout(mainTargetLayout);

    QLabel* workDirLabel = new QLabel(tr("Working directory"));
    QToolDirButton *openWorkDirButton = new QToolDirButton;
    workDirLineEdit = new FLineEdit(QDir::currentPath (), "workDir", {"Folders", "Working directory"}, "workdir");
    workDirLabel->setBuddy(workDirLineEdit);

    QLabel* tempDirLabel = new QLabel(tr("Temporary directory"));
    tempDirLineEdit = new FLineEdit(common::tempdir, "tempDir", {"Folders","Temporary directory"}, "tempdir");
    tempDirLabel->setBuddy(tempDirLineEdit);
    QToolDirButton *openTempDirButton = new QToolDirButton;

    QLabel* binDirLabel = new QLabel(tr("Binary directory"));
    QToolDirButton *openBinDirButton = new QToolDirButton;
    binDirLineEdit = new FLineEdit(QDir::currentPath ()+QDir::separator()+"bindir", "binDir", {"Folders","Binary directory"}, "bindir");
    binDirLabel->setBuddy(binDirLineEdit);

    QGroupBox *auxdirGroupBox = new QGroupBox(tr("Auxiliary directories"));
    QGridLayout *auxdirLayout = new QGridLayout;

    auxdirLayout->addWidget(workDirLabel,1,0);
    auxdirLayout->addWidget(workDirLineEdit, 1,1);
    auxdirLayout->addWidget(openWorkDirButton, 1,2);

    auxdirLayout->addWidget(tempDirLabel, 2,0);
    auxdirLayout->addWidget(tempDirLineEdit, 2,1);
    auxdirLayout->addWidget(openTempDirButton, 2,2);

    auxdirLayout->addWidget(binDirLabel, 3,0);
    auxdirLayout->addWidget(binDirLineEdit, 3,1);
    auxdirLayout->addWidget(openBinDirButton, 3,2);

    auxdirLayout->setColumnMinimumWidth(0,150);
    auxdirGroupBox->setLayout(auxdirLayout);

    QVBoxLayout *mainLayout =new QVBoxLayout;
    FRichLabel *mainLabel = new FRichLabel("Output options", ":/images/64x64/system-file-manager.png");
    mainLayout->addWidget(mainLabel);
    mainLayout->addWidget(logGroupBox);
    mainLayout->addWidget(outputGroupBox);
    mainLayout->addWidget(auxdirGroupBox);
    mainLayout->setMargin(20);
    setLayout(mainLayout);

    connect(logButton, SIGNAL(clicked()), this,  SLOT(on_logButton_clicked()));
    connect(targetDirButton, SIGNAL(clicked()), this, SLOT(selectOutput()));
    connect(openTargetDirButton, SIGNAL(clicked()), this, SLOT(on_openTargetDirButton_clicked()));
    connect(openLogButton, SIGNAL(clicked()), this, SLOT(on_openLogButton_clicked()));
    connect(openWorkDirButton, SIGNAL(clicked()), this, SLOT(on_openWorkDirButton_clicked()));
    connect(openTempDirButton, SIGNAL(clicked()), this, SLOT(on_openTempDirButton_clicked()));
    connect(openBinDirButton, SIGNAL(clicked()), this, SLOT(on_openBinDirButton_clicked()));
    connect(openHtmlLogButton, SIGNAL(clicked()), this, SLOT(on_openHtmlLogButton_clicked()));

}

void outputPage::on_openWorkDirButton_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,  tr("Set working directory"), workDirLineEdit->text());
    if (!path.isEmpty()) workDirLineEdit->setText(path);
}

void outputPage::on_openTempDirButton_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,  tr("Set temporary directory"), tempDirLineEdit->text());
    if (!path.isEmpty()) tempDirLineEdit->setText(path);
}

void outputPage::on_openBinDirButton_clicked()
{
    QString path=QFileDialog::getExistingDirectory(this,  tr("Set binary directory"), binDirLineEdit->text());
    if (!path.isEmpty()) binDirLineEdit->setText(path);
}

void outputPage::on_logButton_clicked()
{
    logPath = QFileDialog::getSaveFileName(this,  tr("Set log file"), "dvda-author.log",tr("Log files (*.log)"));
    if (logPath.isFilled()) htmlLogPath = logPath+".html";
}


void outputPage::on_openLogButton_clicked()
{
    if (!QFileInfo(logPath).exists()) return;
    browser::showPage(QUrl::fromLocalFile(logPath));
}

void outputPage::on_openHtmlLogButton_clicked()
{
      if (!QFileInfo(htmlLogPath).exists()) return;
      browser::showPage(QUrl::fromLocalFile(htmlLogPath));
}

void outputPage::on_openTargetDirButton_clicked()
{
    openDir(targetDirLineEdit->text());
}


void outputPage::selectOutput()
{

    QString path=QFileDialog::getExistingDirectory(this, QString("Open Directory"),
                                                   QDir::currentPath(),
                                                   QFileDialog::ShowDirsOnly
                                                   | QFileDialog::DontResolveSymlinks);
    if (path.isEmpty()) return;

    /* It is recommended to clean the directory, otherwise ProgressBar is flawed. A Warning pops up for confirmation. I eschewed Qt here */
    qint64 size=recursiveDirectorySize(path, "*");
    /* you may have to run as root or sudo to root depending on permissions */

    if (size)
    {
        if (QMessageBox::warning(0, QString("Directory scan"), QString("Directory %1 is not empty (size is %2B). Erase and recreate? ").arg(path,QString::number(size)), QMessageBox::Ok | QMessageBox::Cancel)
                == QMessageBox::Ok)
        {


            if (!remove(path))    QMessageBox::information(0, QString("Remove"),
                                                           QString("Failed to remove %1").arg(QDir::toNativeSeparators(path)));

            QDir targetDirObject(path);
            if (targetDirObject.mkpath(path) == false)
            {
                QMessageBox::warning(0, QString("Directory view"), QString("Failed to create %1").arg(path), QMessageBox::Ok);
                return;
            }

        }
    }

    targetDirLineEdit->setText(path);
}


stillPage::stillPage(dvda* parent, standardPage* standardTab)
{
    parentLocal=parent;
    slides= new FListFrame(parent,
                           parent->fileTreeView,
                           importFiles,
                           "trackSlides",
                          {"Slides","Track slides"},
                           "stillpics",
                           dvdaCommandLine|flags::enabled,
                            {",", "-"},
                            {"slide" , "track"},
                           -1,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           parent->project[AUDIO]->signalList);

    slides->model=parent->model;

    stilloptionListLabel = new QLabel("Available transition effects");
    stilloptionListWidget =new QListWidget;
    stilloptionListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    QStringList stilloptionListText= { "manual browse" , "active browse" , "fade start" ,  "dissolve start" ,  "top-wipe start" ,  "bottom-wipe start" ,
                                       "left-wipe start" ,   "right-wipe start" ,  "fade end" ,  "dissolve end" ,  "top-wipe end" ,  "bottom-wipe end" ,
                                       "left-wipe end" , "right-wipe end" , "short lag" , "medium lag" , "long lag" , "no wait on start",
                                       "short wait on start" , "medium wait on start" , "long wait on start"};

    QStringList stillClChunks = {"manual" , "active" , "starteffect=fade" , "starteffect=dissolve" , "starteffect=top-wipe" ,"starteffect=bottom-wipe" ,
                                 "starteffect=left-wipe" , "starteffect=right-wipe",
                                 "endeffect=fade" , "endeffect=dissolve" , "endeffect=top-wipe" , "endeffect=bottom-wipe" , "endeffect=left-wipe" , "endeffect=right-wipe",
                                 "lag=3" , "lag=6" , "lag=10" , "start=0" , "start=1" , "start=2" , "start=4"};

    selectoptionListWidget = new FListFrame(slides,
                                            stilloptionListWidget,
                                            importNames,
                                            "slideOptions",
    {"Slides","Slide options"},
                                            "stilloptions",
                                            dvdaCommandLine|flags::enabled,
                                            {",", "-"},
                                            {"option" , "slide"},
                                            -1,
                                            NULL,
                                            NULL,
                                            &stillClChunks,
                                            &stilloptionListText,
                                            slides->signalList);

    stilloptionListWidget->addItems(stilloptionListText);

    setWhatsThisText(stilloptionListWidget, 49, 69);

    nextStep = new QToolButton;
    applyAllEffects = new QToolButton;
    applyEffectsToOneFile = new QToolButton;

    const QIcon applyAll =QIcon(QString::fromUtf8(":/images/apply.png"));
    applyAllEffects->setIcon(applyAll);
    applyAllEffects->setIconSize(QSize(22,22));
    applyAllEffects->setText(tr("Apply to all"));
    applyAllEffects->setToolTip(tr("Apply transition effects to all slides for this track."));

    applyEffectsToOneFileUntoggledIcon =QIcon(QString::fromUtf8(":/images/applyEffectsToOneTrackUntoggledIcon.png"));
    applyEffectsToOneFileToggledIcon=QIcon(QString::fromUtf8(":/images/applyEffectsToOneTrackToggledIcon.png"));
    applyEffectsToOneFile->setIcon(applyEffectsToOneFileUntoggledIcon);
    applyEffectsToOneFile->setIconSize(QSize(22,22));
    applyEffectsToOneFile->setText(tr("Apply to slide"));
    applyEffectsToOneFile->setToolTip(tr("Apply transition effects only to selected slides in previous view."));

    on_nextStep_clicked();

    FPalette *palette=new FPalette("Track", "Highlight", "Album/Group", "activemenuPalette", {"Active menu colors"}, "activemenu-palette");

    QGridLayout  *stillLayout=new QGridLayout;
    QGridLayout  *headerLayout=new QGridLayout;
    QVBoxLayout  *mainVLayout = new QVBoxLayout;
    QGridLayout  *paletteLayout = new QGridLayout;

    headerLayout->addWidget(slides->fileLabel, 0,1,1,1, Qt::AlignHCenter);
    headerLayout->addWidget(stilloptionListLabel ,0,1,1,1,Qt::AlignLeft);
    headerLayout->setContentsMargins(15,0,0,0);

    headerLayout->addWidget(selectoptionListWidget->fileLabel, 0,3);

    stillLayout->addWidget(slides->importFromMainTree, 0,0);
    stillLayout->addWidget(slides->tabBox, 0,1);
    stillLayout->addWidget(slides->controlButtonBox, 0,2);
    stillLayout->addWidget(selectoptionListWidget->importFromMainTree, 0,3);
    stillLayout->addWidget(selectoptionListWidget->tabBox, 0,4);
    stillLayout->addWidget(selectoptionListWidget->controlButtonBox, 0,5);
    stillLayout->addWidget(stilloptionListWidget, 0,1,1,Qt::AlignRight);
    stillLayout->addWidget(applyAllEffects, 3,2,1,1,Qt::AlignVCenter);
    stillLayout->addWidget(applyEffectsToOneFile, 3,3,1,1,Qt::AlignLeft);
    stillLayout->addWidget(nextStep, 4,5,1,1,Qt::AlignRight);
    stillLayout->setRowMinimumHeight(0, 400);


    QGroupBox *paletteGroupBox = new QGroupBox("Active menu palette");
    FCheckBox *addPaletteCheckBox = new FCheckBox("Change default text color",
                                                  "addPalette",
    {"Slides","User active menu palette"},
                                                 {
                                                      paletteGroupBox,
                                                      palette,
                                                      standardTab
                                                  });



    paletteLayout->addWidget(palette->button[0], 0,0, Qt::AlignHCenter);
    paletteLayout->addWidget(palette->button[1], 0,1, Qt::AlignHCenter);
    paletteLayout->addWidget(palette->button[2], 0,2, Qt::AlignHCenter);
    paletteGroupBox->setLayout(paletteLayout);

    mainVLayout->addLayout(headerLayout);
    mainVLayout->setSpacing(1);
    mainVLayout->addLayout(stillLayout);
    mainVLayout->addStretch();
    mainVLayout->addWidget(addPaletteCheckBox);
    mainVLayout->addWidget(paletteGroupBox);
    mainVLayout->addSpacing(10);

    // En fait il faudrait  autant de lignes d'importation que de slideshows soit de tracks (au moins)  background_still_k.mpg
    videoFilePath=common::tempdir+QDir::separator()+"background_still_0.mpg"; // yields AUDIO_SV.VOB later on in amg2.c

    videoPlayerButton= new QPushButton;

    videoPlayerButton->setText(tr("Play slideshow"));
    videoPlayerButton->setEnabled( (QFileInfo(videoFilePath).exists()));

    QPushButton *importSlideShowButton= new QPushButton;
     importSlideShowButton->setText(tr("Import  slideshow"));


     videoFileLineEdit = new FLineEdit(videoFilePath,
                                     flags::dvdaCommandLine,
                                     "background-mpg",                          //TODO: check this is the right dvda option!
                                     {"Slides","Path to .mpg slideshow"},
                                     "background slideshow");


    videoPlayerButton->setMaximumSize(videoPlayerButton->sizeHint());
    importSlideShowButton->setMaximumSize(importSlideShowButton->sizeHint());

    mainVLayout->addWidget(videoPlayerButton);
    QHBoxLayout *bottomHLayout=new QHBoxLayout;
    bottomHLayout->addWidget(importSlideShowButton);
    bottomHLayout->addWidget(videoFileLineEdit);
    bottomHLayout->addSpacing(100);

    QVBoxLayout  *mainLayout = new QVBoxLayout;
    FRichLabel *stillLabel = new FRichLabel("Track slideshow options", ":/images/64x64/still.png");
    mainLayout->addWidget(stillLabel);
    mainLayout->addLayout(mainVLayout);
    mainLayout->addLayout(bottomHLayout);
    mainLayout->setMargin(20);

    setLayout(mainLayout);

    connect(nextStep, SIGNAL(clicked()), this,  SLOT(on_nextStep_clicked()));
    connect(applyAllEffects, SIGNAL(clicked()), this,  SLOT(on_applyAllEffects_clicked()));
    connect(applyEffectsToOneFile, SIGNAL(clicked()), this,  SLOT(on_applyAllEffectsToOneFile_clicked()));
    connect(selectoptionListWidget->clearListButton, SIGNAL(clicked()), this, SLOT(on_clearList_clicked()));
    connect(videoPlayerButton, SIGNAL(clicked()), this, SLOT(launchVideoPlayer()));
    connect(importSlideShowButton, SIGNAL(clicked()), this, SLOT(importSlideshow()));
}

void stillPage::importSlideshow()
{
    QString importedVideoFilePath= QFileDialog::getOpenFileName(this, "Import slideshow", QDir::currentPath(), "(*.mpg)" );
    if (!importedVideoFilePath.isEmpty()) videoFilePath=importedVideoFilePath;
    if (QFileInfo(videoFilePath).exists())
    {
        videoPlayerButton->setEnabled(true);
        videoFileLineEdit->setText(videoFilePath);
    }

}

void stillPage::launchVideoPlayer()
{
    VideoPlayer *player=new VideoPlayer(videoFilePath);
    player->resize(800, 450);
    player->show();
}

void stillPage::on_applyAllEffectsToOneFile_clicked()
{
    /* retrieve selected slide list and build CL sring corresponding to selected slides and effects  */
    //optionClChunkList[slides->fileNumber] =QStringList();

    //  QListIterator<QListWidgetItem*> w(selectedEffects);
    //  QStringList rankedOptions=QStringList();

    //  while (w.hasNext())
    //    rankedOptions << listWidgetTranslationHash[w.next()->text()] ;

    //  for (int r=0;  r < Hash::wrapper[slides->frameHashKey]->at(slides->fileNumber).count();  r++)
    //    optionClChunkList[slides->fileNumber] << "rank=" + QString::number(slides->cumulativePicCount[slides->fileNumber] + r)
    //        + "," + rankedOptions.join(",");
    //  applyEffectsToOneFile->setIcon(applyEffectsToOneFileToggledIcon);
}

void stillPage::on_applyAllEffects_clicked()
{
    /* retrieve All slide list whatever the selections and build CL string corresponding to this list and selected effects  */

}

/* on changing page validate all strings into one CL string */

bool  stillPage::selectFilesView;

void stillPage::on_clearList_clicked()
{
    if (!selectFilesView)
    {
        applyEffectsToOneFile->setIcon(applyEffectsToOneFileUntoggledIcon);
    }
}

void stillPage::on_nextStep_clicked()
{
    bool filesView=(selectFilesView == false);
    slides->tabBox->setVisible(filesView);
    slides->fileLabel->setVisible(filesView);
    slides->controlButtonBox->setVisible(filesView);
    slides->importFromMainTree->setVisible(filesView);

    stilloptionListWidget->setVisible(!filesView);
    stilloptionListLabel->setVisible(!filesView);

    applyAllEffects->setVisible(!filesView);
    applyEffectsToOneFile->setVisible(!filesView);

    selectoptionListWidget->tabBox->setVisible(!filesView);
    selectoptionListWidget->fileLabel->setVisible(!filesView);
    selectoptionListWidget->controlButtonBox->setVisible(!filesView);
    selectoptionListWidget->importFromMainTree->setVisible(!filesView);

    if (!filesView)
    {
        nextStep->setText("Browse files");
        nextStep->setArrowType(Qt::LeftArrow);
        nextStep->setToolTip(tr("Step 1: Select slide files"));
        selectoptionListWidget->clearListButton->setToolTip(tr("Erase selected effect list"));
    }
    else
    {
        nextStep->setText("Effects");
        nextStep->setArrowType(Qt::RightArrow);
        nextStep->setToolTip(tr("Step 2: Select transition effects for slides"));
        slides->clearListButton->setToolTip(tr("Erase file list"));
    }

    selectFilesView=filesView;
}

void stillPage::refreshApplyEffectsIcon()
{

    if (Hash::wrapper[slides->getHashKey()][slides->mainTabWidget->currentIndex()].isEmpty())
        applyEffectsToOneFile->setIcon(applyEffectsToOneFileUntoggledIcon);
    else
        applyEffectsToOneFile->setIcon(applyEffectsToOneFileToggledIcon);
}


int options::RefreshFlag;

options::options(dvda* parent)
{
    /* plain old data types must be 0-initialised even though the class instance was new-initialised. */

    options::RefreshFlag=UpdateOptionTabs;
    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(64,64));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setFixedWidth(116);
    contentsWidget->setFixedHeight(690);
    contentsWidget->setSpacing(13);

    pagesWidget = new QStackedWidget;

    optionsTab = new optionsPage;
    advancedTab = new advancedPage;
    standardTab = new standardPage;
    audioMenuTab = new audioMenuPage(parent, standardTab);
    videoMenuTab = new videoMenuPage;
    videolinkTab = new videolinkPage;
    stillTab = new stillPage(parent, standardTab);
    lplexTab = new lplexPage;
    // outputTab must be created after all those that it enables e.g all DVD-A tabs
    outputTab= new outputPage(this);

    pagesWidget->addWidget(outputTab);
    pagesWidget->addWidget(optionsTab);
    pagesWidget->addWidget(advancedTab);
    pagesWidget->addWidget(audioMenuTab );
    pagesWidget->addWidget(videoMenuTab);
    pagesWidget->addWidget(videolinkTab);
    pagesWidget->addWidget(stillTab);
    pagesWidget->addWidget(standardTab);
    pagesWidget->addWidget(lplexTab);

    closeButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(closeButton,
            &QDialogButtonBox::accepted,
            [this,parent]
           {
                options::RefreshFlag =  hasUnsavedOptions;
                accept();
                parent->saveProject(true);
            });

    connect(closeButton, SIGNAL(rejected()), this, SLOT(reject()));

    createIcons();
    contentsWidget->setCurrentRow(0);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(50);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Options"));
    setWindowIcon(QIcon(":/images/dvda-author.png"));

}


/* implement a global clear() function for the FStringList of data in an FListFrame ; it will be used as dvda::clearData() too. Usage below is faulty. */

void options::clearOptionData()
{
    Hash::wrapper.clear();
    stillTab->slides->getCurrentWidget()->clear();
    stillTab->selectoptionListWidget->getCurrentWidget()->clear();
    audioMenuTab->slides->getCurrentWidget()->clear();
    audioMenuTab->soundtracks->getCurrentWidget()->clear();
    audioMenuTab->screentext->getCurrentWidget()->clear();
    options::RefreshFlag = UpdateOptionTabs;
}


void options::createIcon(const char* path, const char* text)
{
    QListWidgetItem *button = new QListWidgetItem(contentsWidget);
    QString strpath=QString(path);
    QString strtext=QString(text);
    button->setIcon(QIcon(strpath));
    button->setText(strtext);
    button->setTextAlignment(Qt::AlignHCenter);
    button->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (strtext !=  "lplex") flags::lplexRank++;
}


void options::createIcons()
{
    QList<const char*> iconList=QList<const char*>() <<
                                                        ":/images/64x64/configure.png" << "General" <<
                                                        ":/images/64x64/dvd-audio2.png" <<  "Disc" <<
                                                        ":/images/64x64/audio-processing.png" << "Audio\nProcessing" <<
                                                        ":/images/64x64/audio-menu.png" << "DVD-A\nMenu" <<
                                                        ":/images/64x64/video-menu.png" << "DVD-V\nMenu" <<
                                                        ":/images/64x64/link.png" << "Video\nLink" <<
                                                        ":/images/64x64/still.png" << "Track\nSlides" <<
                                                        ":/images/64x64/pal.png" << "Norm" <<
                                                        ":/images/64x64/lplex.png" << "lplex" ;


    for (int i=0; i < iconList.size()/2 ; i++) createIcon(iconList[2*i], iconList[2*i+1]);

    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

}

void options::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;
    int r;
    r=(current)? contentsWidget->row(current) : 0;
    if (current) pagesWidget->setCurrentIndex(r);
}


#ifndef OPTIONS_H
#define OPTIONS_H

#include "common.h"
#include "lplex.h"
#include "viewer.h"
#include "flistframe.h"


class options;
class dvda;
class lplexPage;

class standardPage : public common
{
    Q_OBJECT

public :
    standardPage();
    FLineEdit* aspectRatioLineEdit;
    FLineEdit* normTypeLineEdit;

private:
    QListWidget
      *normWidget,
      *aspectRatioWidget;

    QGroupBox
      *normTypeBox,
      *aspectRatioBox;

    FString
      aspectRatioMsg,
      standardMsg;

public slots:
    void changeAspectRatio(QListWidgetItem*,QListWidgetItem*);
    void changeNorm(QListWidgetItem*,QListWidgetItem*);
};


class videolinkPage :public common
{
    Q_OBJECT

public:
    videolinkPage();
    FCheckBox   *videolinkCheckBox;
    FLineEdit   *videoZoneLineEdit;
    FComboBox *videolinkSpinBox;
    QGroupBox* mainBox;

private:

    QToolDirButton   *videoZoneButton;

private slots:

    void on_videolinkButton_clicked();
  #if 0
    void titlesetLink(int);
  #endif

};


class optionsPage :  public common
{
    Q_OBJECT

public:
    optionsPage();
    FCheckBox   *mkisofsBox,  *cdrecordBox, *playbackBox;
    FComboBox   *dvdwriterComboBox;
    FLineEdit *mkisofsLineEdit ;
    QGroupBox *mainBox;

private:
    QToolDirButton   *mkisofsButton;

    struct dvdwriterAddress
    {
      const QStringList dvdwriterBusList;
      const QStringList dvdwriterNameList;
    };
    struct dvdwriterAddress generateDvdwriterPaths();

private slots:
    void on_mkisofsButton_clicked();
    void dvdwriterCheckEditStatus(bool checked);

};

class advancedPage : public common
{
    Q_OBJECT

public:
    advancedPage();
    FCheckBox  *fixwavBox;
    FCheckBox *fixWavOnlyBox;
    FCheckBox  *soxBox;
    FCheckBox  *pruneBox;
    FCheckBox  *paddingBox;
    FLineEdit *startsectorLineEdit;
    QLineEdit* extraAudioFiltersLineEdit ;

private:

    QLabel      *startsectorLabel;


private slots:

    void on_extraAudioFilters_changed(const QString&);
};

class outputPage : public common
{
    Q_OBJECT

public:
    outputPage(options* parent);

    FString
       logPath;

    FCheckBox
      *logBox,
      *debugBox,
      *htmlFormatBox,
      *veryverboseBox,
      *logrefreshBox;

    FRadioBox
      *createDVDFilesRadioBox;

private:
    QToolDirButton
      *logButton,
      *openTargetDirButton,
      *openLogButton ,
      *openHtmlLogButton,
      *targetDirButton;

    QFileDialog logDialog;

    QLabel *targetDirLabel;

    FLineEdit
      * targetDirLineEdit,
      *binDirLineEdit,
      *tempDirLineEdit,
      *workDirLineEdit;

    void selectOutput(const  QString &path);

private slots:
    void on_logButton_clicked();
    void on_openLogButton_clicked();
    void on_openHtmlLogButton_clicked();
    void on_openWorkDirButton_clicked();
    void on_openTempDirButton_clicked();
    void on_openBinDirButton_clicked();
    void on_openTargetDirButton_clicked();
    void selectOutput();
};



class audioMenuPage : public common
{
    Q_OBJECT

public:
    audioMenuPage(dvda* parent, standardPage* standardTab);

    FLineEdit   * audioMenuLineEdit;

    FComboBox
      *menuStyleFComboBox,
      *highlightFormatFComboBox,
      *fontFComboBox,
      *fontSizeFComboBox,
      *nmenuFComboBox;

    FCheckBox *loopVideoBox;
    FListFrame* slides, * soundtracks, *screentext;
    FPalette *palette;

private slots:
    void launchImageViewer();
    void on_audioMenuButton_clicked();
    void on_frameTab_changed(int);
    void on_slidesButton_clicked();
    void setMinimumNMenu(bool value);
    void readFontSizes(int rank);


private:

    QGroupBox
       *slidesBox;

    QPushButton
       *slidesButton;

    QToolDirButton
       *audioMenuButton,
       *openAudioMenuButton;

    FCheckBox *audioMenuCheckBox;
    QToolButton  *clearList ;

    int groupRank;
    QDialog *newWidget;
    QVBoxLayout *newLayout;
    ImageViewer *v;
    QStringList fontList;
};

class videoMenuPage : public common
{
    Q_OBJECT

public:
    videoMenuPage();

private slots:
    void on_openVideoImportButton_clicked();
    void on_videoImportButton_clicked();
    void on_videoMenuImportButton_clicked();

private:

    FCheckBox
      *videoImportCheckBox,
      *audioExportCheckBox;
    FRadioBox *audioExportRadioBox;
    FLineEdit
    *videoImportLineEdit,
    *videoMenuImportLineEdit;
};



class stillPage : public common
{
Q_OBJECT

public:

stillPage(dvda* parent, standardPage* page);
FListFrame* slides, * selectoptionListWidget;


private slots:

void on_nextStep_clicked();
void on_clearList_clicked();
void on_applyAllEffectsToOneFile_clicked();
void on_applyAllEffects_clicked();
void launchVideoPlayer();
void importSlideshow();

private:

    dvda* parentLocal;

    QIcon
        applyEffectsToOneFileUntoggledIcon,
        applyEffectsToOneFileToggledIcon;

    QListWidget
       *stilloptionListWidget;

    QToolButton
       *applyAllEffects,
       *applyEffectsToOneFile,
       *clearList,
       *nextStep;

    QLabel
       * stilloptionListLabel;
    FLineEdit
       * videoFileLineEdit;
    QPushButton
       *videoPlayerButton;

static bool  selectFilesView;

void refreshApplyEffectsIcon();
};

class options :  public common
{
    Q_OBJECT

public:

    options(dvda* parent=0);
    optionsPage *optionsTab;
    advancedPage *advancedTab;
    outputPage *outputTab;
    audioMenuPage *audioMenuTab;
    videoMenuPage *videoMenuTab;
    videolinkPage* videolinkTab ;
    standardPage* standardTab;
    stillPage* stillTab;
    lplexPage *lplexTab;
    static int RefreshFlag;
    QListWidget *contentsWidget;
     void clearOptionData();

signals:

    void defaultClick(bool);


private:

    QDialogButtonBox *closeButton;
    QStackedWidget *pagesWidget;

    void createIcons();
    void createIcon(const char* path, const char* text);

private slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

};
#endif // OPTIONS_H
#include "dvda.h"



void dvda::showEvent(QShowEvent *)
{
  myTimerId=startTimer(800);
}

void dvda::hideEvent(QHideEvent *)
{
  killTimer(myTimerId);
}


void dvda::timerEvent(QTimerEvent *event)
{
  qint64 new_value=0;
  qint64 new_isoSize;
  unsigned short int counter;
  static unsigned short int static_value;

  if (event->timerId() == myTimerId)
    {
      if (startProgressBar)
        {
          new_value=recursiveDirectorySize(Hash::wrapper["targetDir"]->toQString(), "*.AOB");
          progress->setValue(qFloor(discShare(new_value)));
          value=new_value;
        }
      else

        if (startProgressBar2)
          {
            new_isoSize=QFileInfo(Hash::wrapper["mkisofsPath"]->toQString()).size();
            outputTextEdit->append(tr(MSG_HTML_TAG "Size of iso output: %1").arg(QString::number(new_isoSize)));
            counter=qFloor(((float) new_isoSize*102)/ ((float) value));
            progress2->setValue(counter);
          }
        else

          if (startProgressBar3)
            {
              static_value += 3;
              progress3->setValue(static_value);

            }
          else static_value=0;
    }

  else
    QWidget::timerEvent(event);
}

float dvda::discShare(qint64 directorySize)
{
  qint64 tot=dvda::totalSize[AUDIO]+dvda::totalSize[VIDEO];
  if (tot > 1024*1024*1024*4.7) outputTextEdit->append(tr(ERROR_HTML_TAG "total size exceeds 4.7 GB\n"));
  float share=100* ((float) directorySize ) /((float) tot);
  return share;
}

QStringList dvda::createCommandLineString(int commandLineType)
{
 QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);
 QStringList commandLine;

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();
      int itemCommandLineType=item->commandLineType & flags::commandLineMask;
      if ((itemCommandLineType & commandLineType) == itemCommandLineType)
        {
           commandLine +=  item->commandLineStringList();
        }
    }

  return commandLine;
}


void dvda::run()
{
  QStringList args;
  QString command;

  progress->reset();
  if (progress3)
    {
      if ((*FString("burnDisc")).isTrue())
        {
          progressLayout->removeWidget(progress3);
          delete(progress3);
          progressLayout->removeWidget(killCdrecordButton);
          delete(killCdrecordButton);
          progress3=NULL;

        }
      else
        if (progress3->isEnabled()) progress3->reset();
    }

  if (progress2)
    {
      if ((*FString("runMkisofs")).isTrue())
        {
          progressLayout->removeWidget(progress2);
          delete(progress2);
          progressLayout->removeWidget(killMkisofsButton);
          delete(killMkisofsButton);
          progress2=NULL;

        }
      else
        progress2->reset();
    }


  if (dvda::totalSize[AUDIO] + dvda::totalSize[VIDEO] == 0)
    {
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  args << "-P0" << "-o" << common::tempdir+"/output"  << "-g" << "/home/fab/celice.wav" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          << "/home/fab/celice.wav" << "-j" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
          <<          "-g" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
                    << "/home/fab/celice.wav" << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"
                              << "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav"<< "/home/fab/celice.wav";


  args << createCommandLineString(dvdaCommandLine|createIso|createDisc);

  //args << createCommandLineString(lplexFiles).split("-ts");

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing input directory..."));
  outputTextEdit->append(tr(MSG_HTML_TAG "Size of Audio zone input %1").arg(QString::number(dvda::totalSize[AUDIO])));
  outputTextEdit->append(tr(MSG_HTML_TAG "Size of Video zone input %1").arg(QString::number(dvda::totalSize[VIDEO])));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : dvda-author %1").arg(command));

  startProgressBar=1;
  outputType="DVD-Audio authoring";
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.start(/*"konsole"*/ "dvda-author", args);

  // runLplex();
  outputTextEdit->moveCursor(QTextCursor::End);

}

void dvda::runLplex()
{
  QStringList args;
  QString command;


  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      if (item->commandLineType == lplexFiles)
        args << item->commandLineStringList();
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing input directory..."));
  command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  startProgressBar=1;
  outputType="audio DVD-Video disc authoring";
  process.start(/*"konsole"*/ "Lplex", args);

}

void dvda::processFinished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  QStringList  argsMkisofs;
  startProgressBar=0;
  startProgressBar3=0;

  if ((exitStatus == QProcess::CrashExit) ||  (exitCode == EXIT_FAILURE))
    {
       outputTextEdit->append(ERROR_HTML_TAG  +outputType + tr(": dvda-author crashed"));
       return;
     }
    else
     {
        outputTextEdit->append(MSG_HTML_TAG "\n" + outputType + tr(" completed, output directory is %1").arg(v(targetDir)));
        qint64 fsSize=recursiveDirectorySize(v(targetDir), "*.*");
        outputTextEdit->append(tr(MSG_HTML_TAG "File system size: ")+ QString::number(fsSize) + " Bytes ("+ QString::number(((float)fsSize)/(1024.0*1024.0*1024.0), 'f', 2)+ " GB)");
        progress->setValue(maxRange);

        if ((*FString("runMkisofs")).isTrue())
          {
            if ((*FString("targetDir")).isFilled() & (v(mkisofsPath)).isFilled())

              argsMkisofs << "-dvd-audio" << "-o" << v(mkisofsPath) << v(targetDir);

            if (progress2 == NULL)
              {
                killMkisofsButton = new QToolButton(this);
                killMkisofsButton->setToolTip(tr("Kill mkisofs"));
                const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
                killMkisofsButton->setIcon(iconKill);
                killMkisofsButton->setIconSize(QSize(22,22));

                connect(killMkisofsButton, SIGNAL(clicked()), this, SLOT(killMkisofs()));

                progress2 = new QProgressBar(this);
                progress2->setRange(0, maxRange=100);
                progress2->setToolTip(tr("ISO file creation progress bar"));

                QHBoxLayout *progress2Layout= new QHBoxLayout;
                progress2Layout->addWidget(killMkisofsButton);
                progress2Layout->addWidget(progress2);
                progressLayout->addLayout(progress2Layout);
              }
            progress2->reset();
            startProgressBar2=1;
            outputTextEdit->append(tr(MSG_HTML_TAG "mkisofs command line : %1").arg(argsMkisofs.join(" ")));
            process2.start("/usr/bin/mkisofs", argsMkisofs);
          }
   }
}


void dvda::process2Finished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  startProgressBar2=0;

  QFileInfo info=QFileInfo(v(mkisofsPath));

  if ((exitStatus == QProcess::CrashExit) || (exitCode == EXIT_FAILURE))
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG " mkisofs crashed"));
    }
   else
    {
      if (!info.isFile() ||  info.size() == 0)
      {
          outputTextEdit->append(tr(MSG_HTML_TAG "\nISO file could not be created by mksiofs"));
          progress2->reset();
      }
        else
      {
          outputTextEdit->append(tr(MSG_HTML_TAG "\nISO file %1 created").arg(v(mkisofsPath)));

        progress2->setValue(maxRange);
        outputTextEdit->append(tr(MSG_HTML_TAG " You can now burn your DVD-Audio disc"));
      }
     }
}

void dvda::process3Finished(int exitCode,  QProcess::ExitStatus exitStatus)
{
  startProgressBar3=0;

  if (exitStatus == QProcess::CrashExit)
    {
      outputTextEdit->append(tr(ERROR_HTML_TAG "cdrecord crashed"));
    } else

    if (exitCode == EXIT_FAILURE)
      {
        outputTextEdit->append(tr(ERROR_HTML_TAG "DVD-Audio disc was not burned"));
      } else
      {
        progress3->setValue(maxRange);
      }
}



void dvda::on_cdrecordButton_clicked()
{

  if (((*FString("burnDisc")).isFalse())||((*FString("dvdwriterPath")).isEmpty())) return;

  QStringList argsCdrecord;


  if ((*FString("runMkisofs")).isFalse())
    {
      QMessageBox::warning(this, tr("Record"), tr("You need to create an ISO file first to be able to burn a DVD-Audio disc."), QMessageBox::Ok );
      return;
    }


  if ((*FString("dvdwriterPath")).isEmpty())
    {
      QMessageBox::warning(this, tr("Record"), tr("You need to enter the path to a valid DVD writer device."), QMessageBox::Ok );
      return;
    }

  QFileInfo f(v(mkisofsPath));
  f.refresh();

  if (! f.isFile())
    {
      QMessageBox::warning(this, tr("Record"), tr("No valid ISO file path was entered:\n %1").arg(v(mkisofsPath)), QMessageBox::Ok );
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "\nBurning disc...please wait."));
  argsCdrecord << "dev="<< v(dvdwriterPath) << v(mkisofsPath);
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line: cdrecord %1").arg(argsCdrecord.join(" ")));

  if (progress3 == NULL)
    {
      progress3 = new QProgressBar(this);
      killCdrecordButton = new QToolButton(this);
      killCdrecordButton->setToolTip(tr("Kill cdrecord"));
      const QIcon iconKill = QIcon(QString::fromUtf8( ":/images/process-stop.png"));
      killCdrecordButton->setIcon(iconKill);
      killCdrecordButton->setIconSize(QSize(22,22));

      connect(killCdrecordButton, SIGNAL(clicked()), this, SLOT(killCdrecord()));

      QHBoxLayout *progress3Layout= new QHBoxLayout;
      progress3Layout->addWidget(killCdrecordButton);
      progress3Layout->addWidget(progress3);
      progressLayout->addLayout(progress3Layout);
    }

  progress3->setRange(0, maxRange=100);
  progress3->setToolTip(tr("Burning DVD-Audio disc with cdrecord"));
  progress3->reset();

  startProgressBar3=1;
  process3.start("cdrecord", argsCdrecord);

}


void dvda::killDvda()
{
  if (!process.atEnd()) return;
  process.kill();
  outputTextEdit->append(INFORMATION_HTML_TAG+ outputType + tr(" was killed (SIGKILL)"));
  progress->reset();
  processFinished(EXIT_FAILURE, QProcess::NormalExit );
}

void dvda::killMkisofs()
{
  if (!process2.atEnd()) return;
  process2.kill();
  progress2->reset();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG " mkisofs processing was killed (SIGKILL)"));
  process2Finished(EXIT_FAILURE, QProcess::NormalExit);
}

void dvda::killCdrecord()
{
  if (process3.atEnd()) return;
  process3.kill();
  progress3->reset();
  outputTextEdit->append(tr(INFORMATION_HTML_TAG " cdrecord processing was killed (SIGKILL)"));
  process3Finished(EXIT_FAILURE, QProcess::NormalExit);
}


void dvda::extract()
{
  QStringList args;

  progress->reset();
  outputType="Audio recovery";

  QItemSelectionModel *selectionModel = fileTreeView->selectionModel();
  QModelIndexList  indexList=selectionModel->selectedIndexes();

  if (indexList.isEmpty()) return;

  updateIndexInfo();

  uint size=indexList.size();

  if (size > 1) { QMessageBox::warning(this, "Error", tr("Enter just one directory")); return;}

  QModelIndex index;
  index=indexList.at(0);
  if (!index.isValid()) return;

  if (model->fileInfo(index).isFile())
    { QMessageBox::warning(this, "Error", tr("Enter a directory path")); return;}

  else if  (model->fileInfo(index).isDir())
    {
      sourceDir=model->fileInfo(index).absoluteFilePath();
      dvda::totalSize[AUDIO]=(sourceDir.isEmpty())? 0 : recursiveDirectorySize(sourceDir, "*.AOB");
      if (dvda::totalSize[AUDIO] < 100)
        {
          QMessageBox::warning(this, tr("Extract"), tr("Directory path is empty. Please select disc structure."), QMessageBox::Ok | QMessageBox::Cancel);
          return;
        }
    }
  else
    {
      QMessageBox::warning(this, tr("Browse"),
                           tr("%1 is not a file or a directory.").arg(model->fileInfo(index).fileName()));
      return;
    }

  QListIterator<FAbstractWidget*> w(Abstract::abstractWidgetList);

  while (w.hasNext())
    {
      FAbstractWidget* item=w.next();

      if (item->commandLineType == dvdaExtract)
        args << item->commandLineStringList();

    }

  if (dvda::totalSize[AUDIO] == 0)
    {
      processFinished(EXIT_FAILURE,QProcess::NormalExit);
      return;
    }

  outputTextEdit->append(tr(INFORMATION_HTML_TAG "Processing DVD-Audio structure %1").arg(sourceDir));

  outputTextEdit->append(tr(MSG_HTML_TAG "Size of audio content %1").arg(QString::number(dvda::totalSize[AUDIO])));

  QString command=args.join(" ");
  outputTextEdit->append(tr(MSG_HTML_TAG "Command line : %1").arg(command));

  startProgressBar3=1;
  //FAbstractWidget::setProtectedFields(runMkisofs="0";

  process.start(/*"konsole"*/ "dvda-author", args);
}
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "videoplayer.h"
#include <QtWidgets>
#include <qvideowidget.h>
#include <qvideosurfaceformat.h>

VideoPlayer::VideoPlayer(QString &fileName)
{
    errorLabel=new QLabel("") ;
    mediaPlayer =  new QMediaPlayer(0, QMediaPlayer::VideoSurface);

    playButton = new QPushButton;
    QPushButton *exitButton = new QPushButton;
    exitButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    exitButton->setToolTip("Exit");

    playButton->setEnabled(false);
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    if (!fileName.isEmpty())
    {
        mediaPlayer->setMedia(QUrl::fromLocalFile(fileName));
        playButton->setEnabled(true);
    }

    QVideoWidget *videoWidget = new QVideoWidget;

    connect(playButton, SIGNAL(clicked()),    this, SLOT(play()));

    positionSlider = new QSlider(Qt::Horizontal);
    positionSlider->setRange(0, 0);

    connect(positionSlider, SIGNAL(sliderMoved(int)),  this, SLOT(setPosition(int)));

    errorLabel = new QLabel;
    errorLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(exitButton);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(positionSlider);

    QBoxLayout *layout = new QVBoxLayout;

    layout->addWidget(videoWidget);
    layout->addLayout(controlLayout);
    layout->addWidget(errorLabel);
    setLayout(layout);
    setWindowTitle(tr("Player"));
    setWindowIcon(QIcon(":/images/dvda-author.png"));

    mediaPlayer->setVideoOutput(videoWidget);
    connect(mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),   this, SLOT(mediaStateChanged(QMediaPlayer::State)));
    connect(mediaPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
    connect(mediaPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(durationChanged(qint64)));
    connect(mediaPlayer, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleError()));
    connect(exitButton, SIGNAL(clicked()), this, SLOT(accept()));
}


void VideoPlayer::play()
{
    switch(mediaPlayer->state())
    {
    case QMediaPlayer::PlayingState:
        mediaPlayer->pause();
        break;
    default:
        mediaPlayer->play();
        break;
    }
}

void VideoPlayer::mediaStateChanged(QMediaPlayer::State state)
{
    switch(state)
    {
    case QMediaPlayer::PlayingState:
        playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        break;
    default:
        playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        break;
    }
}

void VideoPlayer::positionChanged(qint64 position)
{
    positionSlider->setValue(position);
}

void VideoPlayer::durationChanged(qint64 duration)
{
    positionSlider->setRange(0, duration);
}

void VideoPlayer::setPosition(int position)
{
    mediaPlayer->setPosition(position);
}

void VideoPlayer::handleError()
{
    playButton->setEnabled(false);
    errorLabel->setText("Error: " + mediaPlayer->errorString());
}
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <qmediaplayer.h>

#include <QtGui/QMovie>
#include <QtWidgets>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QSlider;
class QLabel;
QT_END_NAMESPACE

class VideoPlayer : public QDialog
{
    Q_OBJECT
public:
    VideoPlayer(QString &path);

public slots:
    void play();

private slots:
    void mediaStateChanged(QMediaPlayer::State state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void setPosition(int position);
    void handleError();

private:
    QMediaPlayer *mediaPlayer;
    QAbstractButton *playButton;
    QSlider *positionSlider;
    QLabel *errorLabel;
};

#endif

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QPrintDialog>

#include "viewer.h"


ImageViewer::ImageViewer(const QString &imagePath)
{
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    setCentralWidget(scrollArea);

    createActions();
    createMenus();
    if (imagePath.isEmpty()) return;
    const QImage image(imagePath);
    if (image.isNull())
    {
        QMessageBox::information(this, tr("Image Viewer"),
                                 tr("Cannot load %1.").arg(imagePath));
        return;
    }

    imageLabel->setPixmap(QPixmap::fromImage(image));
    scaleFactor = 1.0;
    printAct->setEnabled(true);
    fitToWindowAct->setEnabled(true);
    updateActions();

    if (!fitToWindowAct->isChecked())
        imageLabel->adjustSize();

    setWindowTitle(tr("Image Viewer"));
    setWindowIcon(QIcon(":/images/dvda-author.png"));

    resize(500, 400);

}



void ImageViewer::print()
{
    Q_ASSERT(imageLabel->pixmap());
#ifndef QT_NO_PRINTER

    QPrintDialog dialog(&printer, this);

    if (dialog.exec())
   {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = imageLabel->pixmap()->size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(imageLabel->pixmap()->rect());
        painter.drawPixmap(0, 0, *imageLabel->pixmap());
    }
#endif
}



void ImageViewer::zoomIn()

{
    scaleImage(1.25);
}

void ImageViewer::zoomOut()
{
    scaleImage(0.8);
}


void ImageViewer::normalSize()

{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}



void ImageViewer::fitToWindow()

{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow) {
        normalSize();
    }
    updateActions();
}



void ImageViewer::createActions()

{
    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcut(tr("Ctrl+P"));
    printAct->setEnabled(false);
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    zoomInAct = new QAction(tr("Zoom &In (25%)"), this);
    zoomInAct->setShortcut(tr("Ctrl++"));
    zoomInAct->setEnabled(false);
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));

    zoomOutAct = new QAction(tr("Zoom &Out (25%)"), this);
    zoomOutAct->setShortcut(tr("Ctrl+-"));
    zoomOutAct->setEnabled(false);
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));

    normalSizeAct = new QAction(tr("&Normal Size"), this);
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);
    connect(normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));

    fitToWindowAct = new QAction(tr("&Fit to Window"), this);
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));
    connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));


}



void ImageViewer::createMenus()

{
    fileMenu = new QMenu(tr("&File"), this);

    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    viewMenu = new QMenu(tr("&View"), this);
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);
    viewMenu->addAction(normalSizeAct);
    viewMenu->addSeparator();
    viewMenu->addAction(fitToWindowAct);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(viewMenu);

}



void ImageViewer::updateActions()

{
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}



void ImageViewer::scaleImage(double factor)

{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}



void ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)

{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}



/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef VIEWER_H
#define VIEWER_H


#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QMainWindow>
#include <QPrinter>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
QT_END_NAMESPACE


class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer(const QString &);

private slots:

    void print();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();


private:
    void createActions();
    void createMenus();
    void updateActions();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *openAct;
    QAction *printAct;
    QAction *exitAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;

    QMenu *fileMenu;
    QMenu *viewMenu;

};


#endif


#endif // VIEWER_H
#include "dvda.h"
#include "common.h"



inline const QString dvda::makeParserString(int start, int end)
{

    QStringList L=QStringList();

    for (int j=start; j <=end; j++)
      {

        FAbstractWidget* widget=Abstract::abstractWidgetList.at(j);
        QString hK=widget->getHashKey();

        if  (widget->getHashKey().isEmpty())
          {
            QMessageBox::warning(this, tr("Error"), tr(".dvp project parsing error"));
            continue;
          }

        QString xml=widget->setXmlFromWidget().toQString();
        QString widgetDepth=widget->getDepth();

        L <<  "  <" + hK + " widgetDepth=\"" + widgetDepth +  "\">\n   "
                                 + xml
              +"\n  </" + hK + ">\n";

      }

    return L.join("");

}


inline const QString  dvda::makeDataString()
{
    return  makeParserString(0,1);
}

inline const QString  dvda::makeSystemString()
{
    return makeParserString(2);
}


void dvda::writeProjectFile()
{
  QFile projectFile;
  checkEmptyProjectName();
  projectFile.setFileName(projectName);
  QErrorMessage *errorMessageDialog = new QErrorMessage(this);
  if (!projectFile.open(QIODevice::WriteOnly))
    {
          errorMessageDialog->showMessage(tr("Cannot open file for writing\n")+ qPrintable(projectFile.errorString()));
          QLabel *errorLabel = new QLabel;
          errorLabel->setText(tr("If the box is unchecked, the message "
                                 "won't appear again."));
          return;
     }

  QTextStream out(&projectFile);
  out.setCodec("UTF-8");

  out << "<?xml version=\"1.0\"?>\n" <<"<project>\n";
  out << " <data>\n";

  out << dvda::makeDataString();

  out << " </data>\n";
  out << " <system>\n";

  out << dvda::makeSystemString();

  out << " </system>\n <recent>\n";

  QStringListIterator w(parent->recentFiles);
  QString str;
  while (w.hasNext() && QFileInfo(str=w.next()).isFile())
     out    <<  "  <file>" << str << "</file>\n";

  out << " </recent>\n</project>\n";
  out.flush();
  options::RefreshFlag=hasSavedOptions;
}

namespace XmlMethod
{

   QTreeWidgetItem *itemParent=NULL;

  inline void stackData(const QDomNode & node, QStringList tags, int level, QVariant &textData)
    {
      QDomNode  childNode=node.firstChild();
      QList<QVariant> stackedInfo;
      QStringList strL;
      QString str;

      switch(level)
      {
      /* parses < tag> text </tag> */

           case 0:

                tags[0] = node.toElement().tagName();
                str.clear();
                while ((!childNode.isNull()) && (childNode.nodeType() == QDomNode::TextNode))
                  {
                     str += childNode.toText().data().simplified();
                     childNode=childNode.nextSibling();
                     //Q(str)
                  }

                textData=QVariant(str);
                break;

        /*
         * parses < tags[0]>
                             <tags[1]>  text </tags[1]>
                             ....
                             <tags[1]> text </tags[1]>
                        </tags[0]>
             note: does not check subordinate tag uniformity
        */

          case 1:

                tags[0]=node.toElement().tagName();

                while (!childNode.isNull())
                  {
                    QVariant str;
                    stackData(childNode, QStringList(tags[1]), 0, str);
                    strL << str.toString();
                    childNode=childNode.nextSibling();
                  }
                textData=QVariant(strL);
                break;

        /*
         *   parses
         *            <tags[0]>
         *               <tags[1]>
                             <tags[2]>  text </tags[2]>
                             ....
                             <tags[2]> text </tags[2]>
                         </tags[1]>
                         ...
                         <tags[1]>
                             <tags[2]>  text </tags[2]>
                             ....
                             <tags[2]> text </tags[2]>
                         </tags[1]>
                       </tags[0]>
        */

    case 2:
            tags[0]=node.toElement().tagName();
            childNode=node.firstChild();

           while (!childNode.isNull())
           {
             QStringList L={QString(), QString()};
             QVariant M;
             stackData(childNode, L,1, M);
             stackedInfo << M.toStringList();
             tags[1]=L.at(0);
             tags[2]=L.at(1);
             childNode=childNode.nextSibling();
           }

        textData= QVariant(stackedInfo);
        break;
    }
  }

/* computes sizes and sends filenames to main tab Widget */


/* displays on manager tree window */

void displayTextData(const QStringList &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                     const QColor &color=QColor("blue"));

void displayTextData(const QStringList &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                                 const QColor &color)
{
          static QString last;
          static QTreeWidgetItem* item;
          if ((firstColumn.at(0) != last) && !firstColumn.at(0).isEmpty())
          {
            item = new QTreeWidgetItem(XmlMethod::itemParent);
             item->setText(0, firstColumn.at(0));
             item->setExpanded(false);
          }

           if (firstColumn.count() > 1)
           {


               QTreeWidgetItem* item2 = new QTreeWidgetItem(item);
               item2->setText(0, firstColumn.at(1));
               item2->setText(1, secondColumn);
           }
           else
           {
               QTreeWidgetItem* item2 = new QTreeWidgetItem(item);
               item2->setText(1, secondColumn);
              if (!thirdColumn.isEmpty()) item2->setText(2, thirdColumn);
              if (color.isValid()) item2->setTextColor(2, color);
           }

           last= firstColumn.at(0);
}



/* tags[0] k
 *                       tags[1] 1 : xxx  ...  size MB
 *                       tags[1] 2 : xxx  ...  size MB  */

inline qint64 displaySecondLevelData(    const QStringList &tags,
                                              const QList<QStringList> &stackedInfo,
                                              const QList<QStringList> &stackedSizeInfo)
  {
      int k=0, count=0, l;
      qint64 filesizecount=0;
      QString  firstColumn, root=tags.at(0), secondColumn=tags.at(1), thirdColumn;

      QListIterator<QStringList> i(stackedInfo), j(stackedSizeInfo);

      while ((i.hasNext()) && (j.hasNext()))
       {
          if (!root.isEmpty())
           {
               firstColumn = root + " "+QString::number(++k);
           }

          displayTextData({firstColumn}, "", "");

           QStringListIterator w(i.next()), z(j.next());
           l=0;
           while ((w.hasNext()) && (z.hasNext()))
           {
              ++count;
               if (!tags.at(1).isEmpty())
                   secondColumn =  tags.at(1) +" " +QString::number(++l) + "/"+ QString::number(count) +": ";
               secondColumn += w.next()  ;

               if ((stackedSizeInfo.size() > 0) && (z.hasNext()))
               {
                   qint64 msize=z.next().toLongLong();
                   filesizecount += msize;
                   // force coertion into float or double using .0
                   thirdColumn    = QString::number(msize/1048576.0, 'f', 1) + "/"+  QString::number(filesizecount/1048576.0, 'f', 1)+ " MB" ;
               }

               displayTextData({""}, secondColumn, thirdColumn, (z.hasNext())? QColor("navy"): ((j.hasNext())? QColor("orange") :QColor("red")));

           }
       }
      return filesizecount;
  }


/* tags[0]
 *                       tags[1] 1 : xxx  ...  (size MB)
 *                       tags[1] 2 : xxx  ...  (size MB) ... */

inline void displayFirstLevelData( const QString &tag,  const QString &style, const QStringList &stackedInfo)
    {
       QStringListIterator i(stackedInfo);
       int count=0;
       while (i.hasNext())
          {
             ++count;
             displayTextData((count>1)?QStringList(""):QStringList(tag), style+" "+QString::number(count)+": "+i.next(), "");
           }
     }


}  // end of XmlMethod namespace


void dvda::DomParser(QIODevice* file)
{
  // Beware: to be able to interactively modify managerWidget in the DomParser child class constructor,
  // pass it as a parameter to the constructor otherwise the protected parent member will be accessible yet unaltered
  file->seek(0);

  QString errorStr;
  int errorLine;
  int errorColumn;

  QDomDocument doc;
  if (!doc.setContent(file, true, &errorStr, &errorLine, &errorColumn))
    {
      QMessageBox::warning(0, tr("DOM Parser"), tr("Parse error at line %1, " "column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
      return;
    }

  QDomElement root=doc.documentElement();

  if (root.tagName() != "project") return;

  parent->recentFiles.clear();

  QDomNode node= root.firstChild();

  /* this stacks data into relevant list structures, processes information
   * and displays it in the manager tree Widget  */

  dvda::totalSize[AUDIO]=dvda::totalSize[VIDEO]=0;

  for (QString maintag : {"data", "system", "recent"})
  {
       if (node.toElement().tagName() != maintag) return;

       QDomNode subnode=node.firstChild();

          while (!subnode.isNull())
            {
              const FStringList &str=parseEntry(subnode);
              if (!str.at(0).at(0).isEmpty())
                 *(Hash::wrapper[subnode.toElement().tagName()]=new FStringList) =   str;
                subnode=subnode.nextSibling();
            }

      node = node.nextSibling();
  }

  refreshProjectManagerValues();

  /* this assigns values to widgets (line edits, checkboxes, list widgets etc.)
   * in the Options dialog and ensures fills in main tab widget */

  //if ((dvda::RefreshFlag&UpdateTabMask) == (UpdateMainTabs|UpdateOptionTabs))
  //{

      assignVariables();

      // adds extra information to main window and sets alternating row colors

      for (int ZONE : {AUDIO, VIDEO})
      {
          for (int group_index=0; group_index<= project[ZONE]->getRank(); group_index++)
          {
              int r=0;
              for (QString text : Hash::wrapper[dvda::zoneTag(ZONE)]->at(group_index))
              {
                  if (!text.isEmpty())
                         assignGroupFiles(ZONE, group_index, QDir::toNativeSeparators(text));
                  r++;
              }

              refreshRowPresentation(ZONE, group_index);
          }
      }
  //}

  /* resets recent files using the ones listed in the dvp project file */

  parent->updateRecentFileActions();

  /* used to connect to slides, soundtracks and other option list widgets in Options dialog :
   * these will be activated depending on main tab widget information */

   emit(is_signalList_changed(project[AUDIO]->signalList->size()));
}



FStringList dvda::parseEntry(const QDomNode &node, QTreeWidgetItem *itemParent)
{

  QVariant textData;
  QStringList tags={QString(),QString(),QString()} ;
  int level=node.toElement().attribute("widgetDepth").toInt();

   XmlMethod::itemParent = itemParent;
   XmlMethod::stackData(node, tags, level, textData);

   if ((level == 0) &&(tags[0] == "file"))
                      parent->recentFiles.append(textData.toString());

   switch (level)
   {
   case 0:  return FStringList(textData.toString());
   case 1:  return FStringList(textData.toStringList());
   case 2:  return FStringList(textData.toList());

   }

return FStringList();
}


inline QList<QStringList> dvda::processSecondLevelData(QList<QStringList> &L, bool isFile)
  {
        QListIterator<QStringList> i(L);
        int group_index=0;

        QList<QStringList> stackedSizeInfo2 ;
        while (i.hasNext())
        {
               QStringListIterator w(i.next());
               QStringList stackedSizeInfo1;
               while (w.hasNext())
               {
                   QString text=w.next();
                   if (isFile & QFileInfo(text).isFile())  // double check on file status. First check is for processing speed, so that QFileInfo is only called when necessary
                   {
                       // computing filesizes
                        stackedSizeInfo1 <<  QString::number((long) QFileInfo(text).size());
                   }
               }

               stackedSizeInfo2 << stackedSizeInfo1;
               group_index++;
        }

        return stackedSizeInfo2;
 }



void dvda::refreshProjectManagerValues(int refreshProjectManagerFlag)
{
     if ((refreshProjectManagerFlag & refreshProjectInteractiveMask) == refreshProjectInteractiveMode)
    {
         updateIndexInfo();
         fileSizeDataBase[isVideo] = processSecondLevelData(*Hash::wrapper[dvda::zoneTag(isVideo)]);
    }

    QTreeWidgetItem *item=new QTreeWidgetItem(managerWidget);
    item->setText(0, "data");
    item->setExpanded(true);
    XmlMethod::itemParent=item;

    bool test[2]={(refreshProjectManagerFlag & refreshProjectAudioZoneMask) == refreshAudioZone,
                             (refreshProjectManagerFlag & refreshProjectVideoZoneMask) == refreshVideoZone};

    for (int ZONE: {AUDIO, VIDEO})
             if (test[ZONE])
                      dvda::totalSize[ZONE]=XmlMethod::displaySecondLevelData(
                                                                {dvda::zoneGroupLabel(ZONE), "file"},
                                                                   *Hash::wrapper[dvda::zoneTag(ZONE)],
                              fileSizeDataBase[ZONE]=processSecondLevelData(*Hash::wrapper[dvda::zoneTag(ZONE)]));

       item=new QTreeWidgetItem(managerWidget);
       item->setText(0, "system");
       item->setExpanded(true);
       XmlMethod::itemParent=item;

       if ((refreshProjectManagerFlag & refreshProjectSystemZoneMask) == refreshSystemZone)
      {

           for (int k=2; k <Abstract::abstractWidgetList.count(); k++)
           {

               QString key=Abstract::abstractWidgetList[k]->getHashKey();

               if (Abstract::abstractWidgetList[k]->getDepth() == "0")
               {
                   XmlMethod::displayTextData(Hash::description[key], Hash::wrapper[key]->toQString(), "");
               }
               else if (Abstract::abstractWidgetList[k]->getDepth() == "1")
                   XmlMethod::displayFirstLevelData(Hash::description[key].at(0),   "button", Hash::wrapper[key]->at(0));
           }
       }

       options::RefreshFlag|=hasSavedOptions;

}
