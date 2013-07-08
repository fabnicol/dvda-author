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
   FCheckBox *defaultSaveProjectBehavior;

  protected :

//   void dragEnterEvent(QDragEnterEvent *event);
//   void dropEvent(QDropEvent *event);

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

   bool readFile(const QString &fileName);
   dvda *dvda_author;
   QMainWindow *editWidget;
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
                        *defaultOutputTextEditBox;

   QList<FCheckBox*> widgetList;
   QTextEdit *editor;
   Highlighter *highlighter;
};


#endif

