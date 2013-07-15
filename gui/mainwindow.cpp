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

  QGridLayout* consoleLayout=new QGridLayout;
  console=new QDialog(this);
  console->hide();
  console->setSizeGripEnabled(true);
  console->setWindowTitle("Console");
  console->setMinimumSize(800,600);
  console->show();
  console->raise();
  QToolButton *closeConsoleButton=new QToolButton;
  closeConsoleButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
  closeConsoleButton->setToolTip(tr("Close (Ctrl + Q)"));
  closeConsoleButton->setShortcut(QKeySequence("Ctrl+Q"));
  consoleLayout->addWidget(consoleDialog=new QTextEdit,0,0);
  consoleLayout->addWidget(closeConsoleButton, 1,0,Qt::AlignRight);
  console->setLayout(consoleLayout);

  connect(timer, SIGNAL(timeout()),this, SLOT(feedConsole()));
  connect(closeConsoleButton, &QToolButton::clicked, [=](){on_displayConsoleButton_clicked();});
  connect(&(dvda_author->process), &QProcess::readyReadStandardOutput, [=]()
                                                                                                                                              {
                                                                                                                                                  feedConsole();
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
  bottomDockWidget->setWidget(bottomTabWidget);
  addDockWidget(Qt::BottomDockWidgetArea, bottomDockWidget);

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
  setWindowIcon(QIcon(":/images/dvda-author.png"));
  setWindowTitle("dvda-author interface  "+ QString(VERSION) +" version");
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
  connect(saveAction, &QAction::triggered, [=] () {dvda_author->saveProject(true);});

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
  connect(displayConsoleAction, SIGNAL(triggered()), this, SLOT(on_displayConsoleButton_clicked()));

  editProjectAction=new QAction(tr("Edit current project"), this);
  editProjectAction->setShortcut(QKeySequence("Ctrl+E"));
  editProjectAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
  connect(editProjectAction, SIGNAL(triggered()), this, SLOT(on_editProjectButton_clicked()));

  displayOutputAction  = new QAction(tr("Show/Close messages"), this);
  const QIcon displayOutput = QIcon(QString::fromUtf8( ":/images/display-output.png"));
  displayOutputAction->setIcon(displayOutput);
  connect(displayOutputAction, &QAction::triggered,  [=] () {bottomDockWidget->setVisible(!bottomDockWidget->isVisible());});

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
  connect(exitAction, &QAction::triggered,  [=] () { exit(1);});

  aboutAction=new QAction(tr("&About"), this);
  aboutAction->setIcon(QIcon(":/images/about.png"));

  connect(aboutAction, &QAction::triggered,  [=] () {
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
           << new QAction(tr("Save as..."),this)
           << new QAction(tr("Refresh project"),this)
           << new QAction(tr("S&ave and exit"),this)
           << new QAction(tr("&Exit"),this);

    const char* seq[]={"Ctrl+N","Ctrl+O","Ctrl+S","Ctrl+T","Ctrl+R","Ctrl+E","Ctrl+Q"};
    int j=0;

    for (QAction *a: actionList)
    {
            fileMenu->addAction(a);
            a->setShortcut(QKeySequence(seq[j++]));
    }

    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    editor = new QTextEdit;
    editor->setFont(font);

    highlighter = new Highlighter(editor->document());

   const QString str=dvda_author->projectName;
   if (str.isEmpty()) return;
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
                                                                                        file->close();
                                                                                        dvda::RefreshFlag = dvda::RefreshFlag |UpdateTree | ParseXml;
                                                                                        dvda_author->initializeProject(true);
                                                                                      });

   connect(actionList[3], &QAction::triggered,  [=] ()
                                                                                     {
                                                                                       QString newstr=QFileDialog::getSaveFileName(this, tr("Save project as..."), QDir::currentPath(), tr("dvp projects (*.dvp)"));
                                                                                       if (newstr.isEmpty()) return;
                                                                                       if (newstr == str)
                                                                                       {
                                                                                           actionList[2]->trigger();
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

                                                                                      });

   connect(actionList[4], &QAction::triggered,  [=] ()
                                                                                     {
                                                                                        dvda_author->saveProject(true);
                                                                                        if (file->open(QFile::ReadWrite |  QFile::Text))
                                                                                           {
                                                                                               editor->clear();
                                                                                               editor->setPlainText(file->readAll());
                                                                                               file->close();
                                                                                           }

                                                                                      });

   connect(actionList[5], &QAction::triggered,  [=] ()
                                                                                      {
                                                                                           actionList[2]->trigger();
                                                                                           actionList[6]->trigger();
                                                                                       });

   connect(actionList[6], &QAction::triggered,  [=] ()
                                                                                      {
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

    QGroupBox *displayGroupBox =new QGroupBox(tr("Display"));

    closeButton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout *layout=new QVBoxLayout;
    QVBoxLayout *dlayout=new QVBoxLayout;
    QVBoxLayout *blayout=new QVBoxLayout;

    defaultFileManagerWidgetLayoutBox=new FCheckBox("Display file manager",
                                                                                                                           flags::noCommandLine|flags::checked,
                                                                                                                           "fileManagerDisplay",
                                                                                                                           "Display file manager on left panel");

    defaultProjectManagerWidgetLayoutBox=new FCheckBox("Display project manager",
                                                                                                                                 flags::noCommandLine|flags::checked,
                                                                                                                                 "projectManagerDisplay",
                                                                                                                                 "Display project manager on right panel");

    defaultConsoleLayoutBox=new FCheckBox("Display console as bottom panel tab",
                                                                                                       flags::noCommandLine|flags::checked,
                                                                                                       "launchConsoleAsTab",
                                                                                                       "Add tab to bottom output panel\non launching console");

    defaultFullScreenLayout=new FCheckBox("Full screen",
                                                                                                      flags::noCommandLine|flags::unchecked,
                                                                                                      "fullScreenDisplay",
                                                                                                      "Display interface full screen on launch");


    defaultLplexActivation=new FCheckBox("Activate video zone editing using Lplex",
                                                                                                  flags::noCommandLine|flags::checked,
                                                                                                  "activateLplex",
                                                                                                  "Create DVD-Video zone\nusing Lplex");

    defaultOutputTextEditBox=new FCheckBox("Display message panel",
                                                                                                  flags::noCommandLine|flags::checked,
                                                                                                  "outputTextEdit",
                                                                                                  "Display message panel");

    QGroupBox *behaviorGroupBox =new QGroupBox(tr("Save/Launch"));

    defaultSaveProjectBehavior=new FCheckBox("Save .dvp project automatically",
                                                                                                      flags::noCommandLine|flags::checked,
                                                                                                      "saveProjectBehavior",
                                                                                                      "Saves project if a tab content is changed\nand on exiting the interface");

    defaultLoadProjectBehavior=new FCheckBox("Load .dvp project on launch",
                                                                                                      flags::noCommandLine|flags::checked,
                                                                                                      "loadProjectBehavior",
                                                                                                      "Load latest .dvp project on launch");

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
                        [=] () {
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
    connect(closeButton, &QDialogButtonBox::accepted, [=] ()
                                                                                                         {
                                                                                                               if (    (defaultSaveProjectBehavior->isChecked())
                                                                                                                    || (QMessageBox::Yes == QMessageBox::warning(this, tr("Save project"), tr("Project has not been saved.\nPress Yes to save current .dvp project file now\nor No to close dialog without saving project."), QMessageBox::Yes|QMessageBox::No))
                                                                                                                   )
                                                                                                                         dvda_author->saveProject();
                                                                                                          });

    connect(defaultFileManagerWidgetLayoutBox, SIGNAL(toggled(bool)), this, SLOT(on_displayFileTreeViewButton_clicked(bool)));
    connect(defaultProjectManagerWidgetLayoutBox, SIGNAL(toggled(bool)), dvda_author, SLOT(on_openManagerWidgetButton_clicked(bool)));
    connect(defaultLplexActivation, &FCheckBox::toggled, this, &MainWindow::on_activate_lplex);
    connect(defaultConsoleLayoutBox, SIGNAL(toggled(bool)),  this, SLOT(detachConsole(bool)));
    connect(defaultFullScreenLayout, SIGNAL(toggled(bool)), this, SLOT(showMainWidget(bool)));
    connect(defaultOutputTextEditBox, &FCheckBox::toggled, [=] () {bottomDockWidget->setVisible(defaultOutputTextEditBox->isChecked());});
    connect(defaultLoadProjectBehavior, &FCheckBox::toggled, [=]() {if (defaultLoadProjectBehavior->isChecked()) dvda_author->RefreshFlag |=  ParseXml;});

    setWindowTitle(tr("Configure dvda-author GUI"));
    setWindowIcon(QIcon(":/images/dvda-author.png"));
}

void MainWindow::detachConsole(bool isDetached)
{
    if (isDetached)
    {
        console->hide();
        bottomTabWidget->addTab(consoleDialog, tr("Console"));
        bottomTabWidget->setCurrentIndex(1);
    }
    else
    {
        console->show();
        bottomTabWidget->removeTab(1);
    }
}


void MainWindow::on_displayConsoleButton_clicked()
{
    static bool isDetached;
    detachConsole(isDetached);
    isDetached=!isDetached;
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




void MainWindow::feedConsole()
{
    static bool initialized;

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
           QRegExp reg6("(.*licenses/.)");

            QString text=QString(data).replace(reg6, (QString) HTML_TAG(navy) "\\1</span><br>");
            text= text.replace(reg, (QString) INFORMATION_HTML_TAG "\\1<br>");
            text=text.replace(reg2, (QString) PARAMETER_HTML_TAG "\\1<br>");
            text=text.replace(reg3, (QString) MSG_HTML_TAG "\\1<br>");
            text=text.replace(reg4, (QString) ERROR_HTML_TAG "\\1<br>");
            text=text.replace(reg5, (QString) WARNING_HTML_TAG "\\1<br>");
            text=text.replace("Group", (QString) HTML_TAG(red) "Group</span>");
            text=text.replace("Title", (QString) HTML_TAG(green) "Title</span>");
            text=text.replace("Track", (QString) HTML_TAG(blue) "Track</span>");
            text=text.replace(QRegExp("(.*) ([0-9]*) / ([0-9]*)([ ]*)([0-9]*)"),
                              HTML_TAG(red) "\\1</span>"+QString("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;")
                              + HTML_TAG(green) "\\2</span>" +HTML_TAG(green) "/ <b>\\3</b></span>"
                              +QString("&nbsp;&nbsp;")+HTML_TAG(blue) "\\5</span>&nbsp;&nbsp;&nbsp;&nbsp;");
            consoleDialog->insertHtml(text.replace("\n", "<br>"));
            consoleDialog->update();

    }
    else timer->stop();
    initialized=true;
 }

