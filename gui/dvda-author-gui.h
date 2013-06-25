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

#define SETTINGS(X)      { if (settings->value(#X).isValid())        X->setChecked(settings->value(#X).toBool());}

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
   void on_displayFileTreeViewButton_clicked(bool);
   void on_displayFileTreeViewButton_clicked();
   void on_optionsButton_clicked();
   void showMainWidget();
   void showMainWidget(bool);
   void configure();
   void configureOptions();
   void about();
   void on_activate_lplex(bool);

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
                        *defaultFileManagerWidgetLayoutBox;
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


#endif

