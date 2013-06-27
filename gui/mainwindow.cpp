#include "dvda.h"


/* AUTHOR NOTE




mainWindow.cpp  - Main Window for dvda-author-gui

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


// createFontDataBase looks to be fast enough to be run on each launch.
// Should it slow down application launch on some platform, one option could be to launch it just once then on user demand



void createFontDataBase()
{
    QFontDatabase database;

    QDir dir;
    if (! dir.mkpath(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation)))) return;

    QString fontPath=common::generateDatadirPath("fonts");

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


MainWindow::MainWindow()
{

  createFontDataBase();

  setGeometry(QRect(200, 200,1000,400));
  recentFiles=QStringList()<<QString("default") ;
  FString str=FString();

  dvda_author=new dvda;

  dialog=new options(dvda_author);
//  dialog->setAcceptDrops(true);
  dialog->setParent(dvda_author, Qt::Window);
  dvda_author->parent=this;

  createActions();
  createMenus();
  createToolBars();

  settings = new QSettings("dvda-author", "Free Software Inc");
  QString defaultPath= QDir::currentPath()+"/"+ QString("default.dvp");

  if ((settings->value("default").isValid())
                &&
     (!settings->value("default").toString().isEmpty()))
        dvda_author->setCurrentFile(settings->value("default").toString());
  else
    {
        dvda_author->setCurrentFile(defaultPath);
        settings->setValue("default",defaultPath);
    }


  setCentralWidget(dvda_author);
  dvda_author->setAcceptDrops(false);
  dvda_author->mainTabWidget->addActions(actionList);
  dvda_author->setContextMenuPolicy(Qt::ActionsContextMenu);
  dvda_author->mainTabWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
  outputTextEditDockWidget= new QDockWidget("Messages");
  outputTextEditDockWidget->setWidget(dvda_author->outputTextEdit);
  addDockWidget(Qt::BottomDockWidgetArea, outputTextEditDockWidget);

  fileTreeViewDockWidget= new QDockWidget;
  fileTreeViewDockWidget->setWidget(dvda_author->fileTreeView);
  fileTreeViewDockWidget->setMinimumHeight((unsigned) (height()*0.3));
  fileTreeViewDockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
  fileTreeViewDockWidget->hide();
  addDockWidget(Qt::LeftDockWidgetArea, fileTreeViewDockWidget);

  configureOptions();

  Abstract::refreshOptionFields();

  SETTINGS(defaultLplexActivation)
  SETTINGS(defaultFullScreenLayout)
  SETTINGS(defaultConsoleLayoutBox)
  SETTINGS(defaultProjectManagerWidgetLayoutBox)
  SETTINGS(defaultFileManagerWidgetLayoutBox)


  setWindowIcon(QIcon(":/images/dvda-author.png"));
  setWindowTitle("dvda-author GUI "+ QString(VERSION));
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

void MainWindow::about()
{
  QUrl url=QUrl::fromLocalFile( dvda_author->generateDatadirPath("about.html") );
  browser::showPage(url);
}

void MainWindow::createActions()
{
  openAction = new QAction(tr("&Open .dvp project file"), this);
  openAction->setIcon(QIcon(":/images/open-project.png"));
  connect(openAction, SIGNAL(triggered()), dvda_author, SLOT(on_openProjectButton_clicked()));

  saveAction = new QAction(tr("&Save project file as..."), this);
  saveAction->setIcon(QIcon(":/images/document-save-as.png"));
  connect(saveAction, SIGNAL(triggered()), dvda_author, SLOT(requestSaveProject()));

  closeAction = new QAction(tr("&Close .dvp project file"), this);
  closeAction->setIcon(QIcon(":/images/document-close.png"));
  connect(closeAction, SIGNAL(triggered()), dvda_author, SLOT(closeProject()));

  burnAction = new QAction(tr("&Burn files to disc"), this);
  burnAction->setIcon(QIcon(":/images/burn.png"));
  connect(burnAction, SIGNAL(triggered()), dvda_author, SLOT(on_cdrecordButton_clicked()));


  encodeAction = new QAction(tr("Start c&reating disc files"), this);
  encodeAction->setIcon(QIcon(":/images/encode.png"));
  connect(encodeAction, SIGNAL(triggered()), dvda_author, SLOT(run()));

  decodeAction = new QAction(tr("&Decode disc to generate wav files"), this);
  decodeAction->setIcon(QIcon(":/images/decode.png"));
  connect(decodeAction, SIGNAL(triggered()), dvda_author, SLOT(extract()));

  optionsAction = new QAction(tr("&Processing options"), this);
  optionsAction->setIcon(QIcon(":/images/configure.png"));
  connect(optionsAction, SIGNAL(triggered()), this, SLOT(on_optionsButton_clicked()));

  configureAction= new QAction(tr("&Configure interface"), this);
  configureAction->setIcon(QIcon(":/images/configure-toolbars.png"));
  connect(configureAction, SIGNAL(triggered()), this, SLOT(configure()));

  helpAction = new QAction(tr("&Help"), this);
  helpAction->setIcon(QIcon(":/images/help-contents.png"));
  connect(helpAction, SIGNAL(triggered()), dvda_author, SLOT(on_helpButton_clicked()));

  displayAction = new QAction(tr("&Show maximized/normal"), this);
  displayAction->setIcon(QIcon(":/images/show-maximized.png"));
  connect(displayAction, SIGNAL(triggered()), this, SLOT(showMainWidget()));

  displayManagerAction = new QAction(tr("Show/Close project &manager"), this);
  const QIcon iconViewList = QIcon(QString::fromUtf8( ":/images/manager.png"));
  displayManagerAction->setIcon(iconViewList);
  connect(displayManagerAction, SIGNAL(triggered()), dvda_author, SLOT(on_openManagerWidgetButton_clicked()));

  displayConsoleAction = new QAction(tr("Show/Close &console"), this);
  const QIcon consoleIcon = QIcon(QString::fromUtf8( ":/images/console.png"));
  displayConsoleAction->setIcon(consoleIcon);
  connect(displayConsoleAction, SIGNAL(triggered()), dvda_author, SLOT(on_displayConsoleButton_clicked()));

  editProjectAction=new QAction(tr("Edit current project"), this);
  editProjectAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
  connect(editProjectAction, SIGNAL(triggered()), this, SLOT(on_editProjectButton_clicked()));

  displayOutputAction  = new QAction(tr("Show/Close messages"), this);
  const QIcon displayOutput = QIcon(QString::fromUtf8( ":/images/display-output.png"));
  displayOutputAction->setIcon(displayOutput);
  connect(displayOutputAction, SIGNAL(triggered()), this, SLOT(on_displayOutputButton_clicked()));

  displayFileTreeViewAction  = new QAction(tr("Show/Close file manager"), this);
  const QIcon displayFileTreeView = QIcon(QString::fromUtf8( ":/images/view-list-tree.png"));
  displayFileTreeViewAction->setIcon(displayFileTreeView);
  connect(displayFileTreeViewAction, SIGNAL(triggered()), this, SLOT(on_displayFileTreeViewButton_clicked()));

  clearOutputTextAction = new QAction(tr("Clear message &window"), this);
  const QIcon clearOutputText = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
  clearOutputTextAction->setIcon(clearOutputText);
  connect(clearOutputTextAction, SIGNAL(triggered()), dvda_author, SLOT(on_clearOutputTextButton_clicked()));

  exitAction = new QAction(tr("&Exit"), this);
  exitAction->setIcon(QIcon(":/images/application-exit.png"));
  exitAction->setShortcut(QKeySequence("Ctrl+Q"));
  connect(exitAction, SIGNAL(triggered()), this, SLOT(on_exitButton_clicked()));

  aboutAction=new QAction(tr("&About"), this);
  aboutAction->setIcon(QIcon(":/images/about.png"));
  connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

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

  actionList << openAction << saveAction << closeAction << exitAction << separator[0] <<
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


void MainWindow::on_displayOutputButton_clicked()
{
  outputTextEditDockWidget->setVisible(!outputTextEditDockWidget->isVisible());
 }


void MainWindow::on_displayFileTreeViewButton_clicked(bool isHidden)
{
   fileTreeViewDockWidget->setVisible(isHidden);
   dvda_author->on_frameTab_changed(dvda_author->mainTabWidget->currentIndex());
 }

void MainWindow::on_displayFileTreeViewButton_clicked()
{
  bool isHidden=fileTreeViewDockWidget->isHidden();
  on_displayFileTreeViewButton_clicked(isHidden);
}

void MainWindow::on_exitButton_clicked()
{
    exit(1);
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
 fileToolBar->addAction(closeAction);
 fileToolBar->addAction(exitAction);
 fileToolBar->addSeparator();

 editToolBar->addAction(displayAction);
 editToolBar->addAction(displayOutputAction);
 editToolBar->addAction(displayFileTreeViewAction);
 editToolBar->addAction(displayManagerAction);
 editToolBar->addAction(displayConsoleAction);
 editToolBar->addAction(clearOutputTextAction);
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

    QList<QAction*> actionList=QList<QAction*>()
           << new QAction(tr("&New"),this)
           << new QAction(tr("&Open"),this)
           << new QAction(tr("&Save"),this)
           << new QAction(tr("&Exit"),this);

    for (QAction *a: actionList)
            fileMenu->addAction(a);

    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    editor = new QTextEdit;
    editor->setFont(font);

    highlighter = new Highlighter(editor->document());

   const QString str=dvda_author->projectName;
   QFile  *file=new QFile(str);
   if (file->open(QFile::ReadWrite| QFile::Text))
   {
       editor->setPlainText(file->readAll());
       file->close();
   }

   connect(actionList[0], &QAction::triggered, [=] () { editor->clear();});
   connect(actionList[1], &QAction::triggered, [=] ()
                                                                                     {
                                                                                        file->~QFile();
                                                                                        dvda_author->on_openProjectButton_clicked() ;
                                                                                        editWidget->~QMainWindow();
                                                                                        on_editProjectButton_clicked();
                                                                                      });

   connect(actionList[2], &QAction::triggered,  [=] ()
                                                                                     {
                                                                                        file->open(QFile::Truncate |QFile::WriteOnly| QFile::Text);
                                                                                        file->write(editor->document()->toPlainText().toUtf8()) ;
                                                                                      });

   connect(actionList[3], &QAction::triggered,  [=] ()
                                                                                      {
                                                                                         file->close();
                                                                                         file->~QFile();
                                                                                         actionList.~QList();
                                                                                         editWidget->~QMainWindow() ;
                                                                                       });
   editWidget->setCentralWidget(editor);
   editWidget->setGeometry(200,200,600,800);
   editWidget->show();

}

void MainWindow::configureOptions()
{
    /* plain old data types must be 0-initialised even though the class instance was new-initialised. */

    contentsWidget = new QDialog(this);
    contentsWidget->setVisible(false);

    closeButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    //QHBoxLayout *layout=new QHBoxLayout;
    QVBoxLayout *vlayout=new QVBoxLayout;

    defaultFileManagerWidgetLayoutBox=new FCheckBox("Display file manager",
                                                                                                                           flags::noCommandLine|flags::checked,
                                                                                                                           "fileManagerDisplay",
                                                                                                                           "Display file manager on left panel");

    defaultProjectManagerWidgetLayoutBox=new FCheckBox("Display project manager",
                                                                                                                                 flags::noCommandLine|flags::checked,
                                                                                                                                 "projectManagerDisplay",
                                                                                                                                 "Display project manager on right panel");

    defaultConsoleLayoutBox=new FCheckBox("Launch console as tab",
                                                                                                       flags::noCommandLine|flags::checked,
                                                                                                       "launchConsoleAsTab",
                                                                                                       "Add tab to bottom output panel on launching console");

    defaultFullScreenLayout=new FCheckBox("Launch as full screen",
                                                                                                      flags::noCommandLine|flags::unchecked,
                                                                                                      "fullScreenDisplay",
                                                                                                      "Display interface full screen on launch");

    defaultLplexActivation=new FCheckBox("Activate video zone editing using Lplex",
                                                                                                  flags::noCommandLine|flags::checked,
                                                                                                  "activateLplex",
                                                                                                  "A DVD-Video zone can be created using Lplex");

    vlayout->addWidget(defaultFileManagerWidgetLayoutBox);
    vlayout->addWidget(defaultProjectManagerWidgetLayoutBox);
    vlayout->addWidget(defaultConsoleLayoutBox);
    vlayout->addWidget(defaultFullScreenLayout);
    vlayout->addWidget(defaultLplexActivation);

    vlayout->addWidget(closeButton);
    contentsWidget->setLayout(vlayout);
    connect(closeButton, &QDialogButtonBox::accepted,
                        [=] () {
                           settings->setValue("defaultLplexActivation", defaultLplexActivation->isChecked());
                           settings->setValue("defaultFullScreenLayout", defaultFullScreenLayout->isChecked());
                           settings->setValue("defaultConsoleLayoutBox", defaultConsoleLayoutBox->isChecked());
                           settings->setValue("defaultProjectManagerWidgetLayoutBox", defaultProjectManagerWidgetLayoutBox->isChecked());
                           settings->setValue("defaultFileManagerWidgetLayoutBox", defaultFileManagerWidgetLayoutBox->isChecked());
                           contentsWidget->accept();
                        }
                  );

    /* note on connection syntax
     * Here the new Qt5 connection syntax should be used with care and disregarded when both an action button and an FCheckBox activate a slot as the slots
     * are overloaded (which could possibly be rewritten) and a) the action button uses the argumentless slot whilst
     * b) the boolean version of slots must be used by the FcheckBox. The new Qt5 syntax cannot work this out as it does not manage overloading. */

    connect(closeButton, &QDialogButtonBox::rejected, contentsWidget, &QDialog::reject);
    connect(closeButton, &QDialogButtonBox::accepted, dvda_author, &dvda::saveProject);
    connect(defaultFileManagerWidgetLayoutBox, SIGNAL(toggled(bool)), this, SLOT(on_displayFileTreeViewButton_clicked(bool)));
    connect(defaultProjectManagerWidgetLayoutBox, SIGNAL(toggled(bool)), dvda_author, SLOT(on_openManagerWidgetButton_clicked(bool)));
    connect(defaultLplexActivation, &FCheckBox::toggled, this, &MainWindow::on_activate_lplex);
    //connect(defaultConsoleLayoutBox, &FCheckBox::toggled, this, &MainWindow:on_displayFileTreeViewButton_clicked);
    connect(defaultFullScreenLayout, SIGNAL(toggled(bool)), this, SLOT(showMainWidget(bool)));

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



