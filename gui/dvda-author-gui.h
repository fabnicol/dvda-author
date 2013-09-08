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
   QAction *playInSpectrumAnalyzerAction ;
   QAction *playAction ;
   long long cdRecordProcessedOutput;


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
   QAction *reSampleTo16_44100;
   QAction *reSampleTo16_48000;
   QAction *reSampleTo16_96000;
   QAction *reSampleTo24_44100;
   QAction *reSampleTo24_48000;
   QAction *reSampleTo24_96000;
   QAction *exportAudioToVideo;
   QAction *exportVideoToAudio;
   QAction *createMirror;

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

