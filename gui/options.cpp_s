#include <QtGui>
#include <QFile>

#include "dvda.h"
#include "forms.h"
#include "options.h"
#include "videoplayer.h"

#define v(X) *FString(#X)

standardPage::standardPage()
{
    normTypeBox=new QGroupBox(tr("TV Standard"));
    aspectRatioBox=new QGroupBox(tr("Screen Size"));

    normTypeLineEdit = new FLineEdit("PAL",
                                     "normType",
                                     "TV Standard",
                                     "norm");

    aspectRatioLineEdit = new FLineEdit("16:9",
                                        "aspectRatio",
                                        "Screen size",
                                        "aspect");

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
    mkisofsButton = new QToolButton;
    mkisofsButton->setToolTip(tr("Select or type in .iso filename for disc image.\nThis file will be used for disc burning."));

    QLabel *mkisofsLabel = new QLabel(tr("Path to ISO file:"));
    mkisofsLineEdit = new FLineEdit(tempdir+QDir::separator()+"dvd.iso",
                                    createIso,
                                    "mkisofsPath",
                                    "Path to ISO image",
                                    "mkisofs");


    dvdwriterComboBox = new FComboBox("",
                                      createDisc,
                                      "dvdwriterPath",
                                      "Path to DVD writer device",
                                      "cdrecord");

    dvdwriterComboBox->setMinimumContentsLength(35);


    cdrecordBox= new FCheckBox("Burn to DVD-Audio/Video disc",
                               "burnDisc",
                               "Burn disc image to DVD",
                                {dvdwriterComboBox});

    mkisofsBox =new FCheckBox("Create ISO file",
                              flags::checked|flags::enabled|flags::dvdaCommandLine,
                              "runMkisofs",
                              "Create disc image using mkisofs",
                              {
                                  mkisofsButton,
                                  cdrecordBox,
                                  mkisofsLabel,
                                  mkisofsLineEdit
                              });

    playbackBox= new FCheckBox("Launch playback on loading disc",
                               "playback",
                               "Launch playback on loading",
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
                               "Pad wav files",
                               "padding");

    pruneBox = new FCheckBox("Cut silence at end of wav files ",
                             flags::disabled|flags::dvdaCommandLine,
                             "prune",
                             "Cut silence at end of wav files",
                             "prune");

    Q2ListWidget controlledObjects={{paddingBox, pruneBox}} ;

    fixWavOnlyBox=new FCheckBox("Only fix wav headers,\ndo not process audio",
                                flags::disabled|flags::dvdaCommandLine,
                                "fixWavOnly",
                                "Only fix wav headers",
                                "fixwav",
                                &controlledObjects);

    setWhatsThisText(fixWavOnlyBox, 78,79);

    fixwavBox = new FCheckBox("Fix corrupt wav headers\nand process audio",
                              "fixwav",
                              "Fix corrupt wav headers",
                              "fixwav",
                              &controlledObjects);

    setWhatsThisText(fixwavBox, 82,90);

    startsectorLabel = new QLabel(tr("&Start sector"));
    startsectorLineEdit = new FLineEdit("281", "startsector", "Start sector number","startsector");
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
                          "Use SoX to convert audio files",
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
                                 "Number of menus",
                                 "nmenus");

    nmenuFComboBox->setMaximumWidth(50);

    QIcon *iconSlides=new QIcon(":/images/64x64/still.png");
    QIcon *iconSoundtracks=new QIcon(":/images/audio_file_icon.png");
    QIcon *iconScreentext=new QIcon(":/images/64x64/text-rtf.png");

    slides= new FListFrame(parent,
                           parent->fileTreeView,
                           importFiles,
                           "audioMenuSlides",
                           "DVD-Audio menu slides",
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
                                "DVD-Audio menu tracks",
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
                               "DVD-Audio menu text",
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
                                "Loop menu video",
                                "loop");

    menuStyleFComboBox=new FComboBox({"standard", "hierarchical", "active"},
                                     "menuStyle",
                                     "Menu style",
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
                                           "Highlight format",
                                           "highlightformat",
                                           iconList);

    QString fontPath=generateDatadirPath("fonts");

    fontList=QStringList();

    if (common::readFile(fontPath, fontList) == 0)
        QMessageBox::warning(this, tr("Error"), tr("Failed to open font list file in ") +fontPath);

    fontFComboBox=new FComboBox(fontList,
                                "font",
                                "Font",
                                "fontname");

    fontSizeFComboBox=new FComboBox(QStringList(),
                "fontSize",
                "Font size",
                "fontsize");

    readFontSizes(0);

    fontFComboBox->setMaximumWidth(250);

    fontSizeFComboBox->setMaximumWidth(50);
    fontSizeFComboBox->setCurrentIndex(4);

    setWhatsThisText(menuStyleBox, 4, 9);

    audioMenuLineEdit = new FLineEdit(common::tempdir+QDir::separator()+QString::fromUtf8("audiobackground.png"),
                                      "audioBackgroundPath",
                                      "Path to DVD-Audio menu background",
                                      "blankscreen");

    QLabel *audioMenuLabel = new QLabel(tr("Menu background"));
    audioMenuButton = new QPushButton(tr("Browse"));
    audioMenuButton->setToolTip(tr("Select customized menu background image (*.png)"));
    openAudioMenuButton = new QPushButton(tr("Open"));
    openAudioMenuButton ->setToolTip(tr("Open menu background file in image viewer"));

    setWhatsThisText(slidesBox, 12, 20);

    FPalette *palette=new FPalette("Track",
                                   "Highlight",
                                   "Album/Group",
                                   "topmenuPalette",
                                   "Top menu colors",
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
                                      "Create DVD-Audio menu",
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
                                        "Path to DVD-Video directory",
                                        "videodir");

    QLabel *videoImportLabel = new QLabel(tr("Video directory"));
    QToolButton *videoImportButton;
    videoImportButton = new QToolButton;
    videoImportButton->setToolTip(tr("Import DVD-Video directory to project"));
    videoImportButton->setFixedWidth(buttonwidth);

    QPushButton *openVideoImportButton;
    openVideoImportButton = new QPushButton(tr("Open"));
    openVideoImportButton ->setToolTip(tr("Open DVD-Video directory "));
    openVideoImportButton->setFixedWidth(buttonwidth);

    videoImportCheckBox= new FCheckBox("Import DVD-Video",
                                       "videoMenu",
                                       "Import DVD-Video ",
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
    QToolButton *videoMenuImportButton = new QToolButton;
    videoMenuImportButton->setToolTip(tr("Import DVD-Video menu"));
    videoMenuImportButton->setFixedWidth(buttonwidth);

    videoMenuImportLineEdit = new FLineEdit(tempdir+QDir::separator()+QString::fromUtf8("VIDEO_TS/VIDEO_TS.VOB"),
                                            flags::disabled,
                                            "videoMenuImport",
                                            "Import DVD-Video menu",
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
                "Create DVD-Audio/Video hybrid",
    {"hybridate", "hybridate", "hybridate-export-menu"},
                audioExportRadioBoxEnabledObjects);

    QGroupBox *audioExportBox = new QGroupBox(tr("DVD-Audio/Video hybrid"));
    QGridLayout *audioExportLayout=new QGridLayout;

    audioExportCheckBox = new FCheckBox("Create DVD-Audio/Video hybrid disc",
                                        "createHybrid",
                                        "Create hybrid DVD-Audio/Video disc",
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
                                     "Rank of linked video titleset",
                                     "T");

    videolinkSpinBox->setEnabled(true);

    videolinkSpinBox->setMaximumWidth(50);

    setWhatsThisText(videolinkSpinBox, 44 ,46);

    QLabel *videolinkLabel = new QLabel(tr("Rank of video titleset "));
    videolinkLabel->setBuddy(videolinkSpinBox);

    videoZoneLineEdit = new FLineEdit(tempdir + QDir::separator()+QString::fromUtf8("VIDEO_TS"),
                                      "videoZonePath",
                                      "Path to VIDEO_TS linked to",
                                      "V");

    videoZoneButton = new QPushButton(tr("Browse"));
    videoZoneButton->setToolTip(tr("Select customized menu background image (*.png)"));

    videolinkCheckBox = new FCheckBox("Link Audio zone\nto Video zone",
                                      "videolink",
                                      "Link Audio zone to Video zone",
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
    logButton = new QPushButton(tr("&Browse"));
    logButton->setFixedWidth(110);
    logButton->setToolTip(tr("Browse or type in filename for dvda-author log file."));

    openLogButton = new QPushButton(tr("&Open log"));
    openLogButton->setFixedWidth(110);
    openLogButton->setToolTip(tr("Open log text file in system-selected text editor."));

    openHtmlLogButton = new QPushButton(tr("Open &Html log"));
    openHtmlLogButton->setFixedWidth(110);
    openHtmlLogButton->setToolTip(tr("Open log.html file in system-selected brower."));

    debugBox = new FCheckBox(tr("Debugging-level verbosity"),
                             flags::disabled|flags::dvdaCommandLine,
                             "debug",
                             "Use debug-level verbosity",
                             "debug" );

    veryverboseBox = new FCheckBox(tr("Increased verbosity"),
                                   flags::disabled|flags::dvdaCommandLine,
                                   "veryverbose",
                                   "Use enhanced verbosity",
                                   "veryverbose");

    htmlFormatBox = new FCheckBox(tr("Html format"),
                                  flags::disabled|flags::dvdaCommandLine,
                                  "htmlFormat",
                                  "Output html log",
                                  "loghtml");

    logrefreshBox=new FCheckBox(tr("Refresh log"),
                                flags::disabled|flags::dvdaCommandLine,
                                "logrefresh",
                                "Erase prior logs on running",
                                "logrefresh");


    logBox = new FCheckBox(tr("&Log file"),
                           "log",
                           "Create log file",
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
    logLayout->addWidget(logButton,1,3,1,1);
    logLayout->addWidget(openLogButton,2,3,1,1);
    logLayout->addWidget(debugBox,3,1);
    logLayout->addWidget(veryverboseBox,4,1);
    logLayout->addWidget(htmlFormatBox,5,1);
    logLayout->addWidget(logrefreshBox,7,1);
    logLayout->addWidget(openHtmlLogButton,6,3,1,1);

    logLayout->setRowMinimumHeight(0,20);
    logLayout->setRowMinimumHeight(5,20);
    logLayout->setRowMinimumHeight(7,30);
    logLayout->setColumnMinimumWidth(0,30);
    logLayout->setColumnMinimumWidth(2,20);
    logLayout->setColumnMinimumWidth(4,180);

    logGroupBox->setLayout(logLayout);

    targetDirLabel = new QLabel(tr("Output directory"));
    targetDirButton = new QToolButton;
    openTargetDirButton = new QPushButton(tr("Open"));

    targetDirLineEdit = new FLineEdit(tempdir+QDir::separator()+"output", "targetDir", "DVD-A file directory", "o");

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
                                           "Create DVD Files",
                                           { "" , "no-output"},
                                           createDVDFilesEnabledObjects,
                                           createDVDFilesDisabledObjects);

    parent->advancedTab->fixWavOnlyBox->disabledObjects=new Q2ListWidget;
    //parent->advancedTab->fixWavOnlyBox->disabledObjects->append(createDVDFilesRadioBox);

    setWhatsThisText(createDVDFilesRadioBox, 104, 105);

    targetDirButton->setToolTip(tr("Browse output directory for DVD-Audio disc files."));

    openTargetDirButton->setToolTip(tr("Open output directory for DVD-Audio disc files."));

    QHBoxLayout *targetLayout = new QHBoxLayout;

    targetLayout->addWidget(createDVDFilesRadioBox);
    targetLayout->addStretch(4);

    QGridLayout *targetDirLineEditLayout=new QGridLayout;
    targetDirLineEditLayout->addWidget(targetDirLabel,1,0);
    targetDirLineEditLayout->addWidget(targetDirLineEdit,1,1);
    targetDirLineEditLayout->addWidget(targetDirButton, 1,2);
    targetDirLineEditLayout->addWidget(openTargetDirButton, 1,3, Qt::AlignLeft);
    targetDirLineEditLayout->setColumnMinimumWidth(0,150);
    targetDirLineEditLayout->setColumnMinimumWidth(4,60);

    QVBoxLayout *mainTargetLayout=new QVBoxLayout;
    mainTargetLayout->addLayout(targetLayout);
    mainTargetLayout->addStretch(2);
    mainTargetLayout->addLayout(targetDirLineEditLayout);
    mainTargetLayout->addStretch();

    QGroupBox *outputGroupBox = new QGroupBox(tr("DVD-Audio disc files"));
    outputGroupBox->setLayout(mainTargetLayout);

    QLabel* workDirLabel = new QLabel(tr("Working directory"));
    QToolButton *openWorkDirButton = new QToolButton;
    workDirLineEdit = new FLineEdit(QDir::currentPath (), "workDir", "Working directory", "workdir");
    workDirLabel->setBuddy(workDirLineEdit);

    QLabel* tempDirLabel = new QLabel(tr("Temporary directory"));
    tempDirLineEdit = new FLineEdit(common::tempdir, "tempDir", "Temporary directory", "tempdir");
    tempDirLabel->setBuddy(tempDirLineEdit);
    QToolButton *openTempDirButton = new QToolButton;

    QLabel* binDirLabel = new QLabel(tr("Binary directory"));
    QToolButton *openBinDirButton = new QToolButton;
    binDirLineEdit = new FLineEdit(QDir::currentPath ()+QDir::separator()+"bindir", "binDir", "Binary directory", "bindir");
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

    auxdirLayout->setColumnMinimumWidth(4,60);
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
    FString logPath = QFileDialog::getSaveFileName(this,  tr("Set log file"), "dvda-author.log",tr("Log files (*.log)"));
    if (logPath.isFilled()) htmlLogPath = logPath+".html";
}


void outputPage::on_openLogButton_clicked()
{
    if ((v(log)).isFalse()) return;
    if ((v(logPath)).isEmpty()) return;
    QUrl url("file:///"+hash::qstring["logPath"]);
    QDesktopServices::openUrl(url);
}

void outputPage::on_openHtmlLogButton_clicked()
{
    if ((v(htmlFormat)).isFalse()) return;
    if ((v(logPath)).isEmpty()) return;
    if (!QFileInfo(htmlLogPath).exists()) return;

    QUrl url("file:///"+htmlLogPath);
    QDesktopServices::openUrl(url);
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
            else
                outputTextEdit->append("Removed output directory "+path);

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
                           "Track slides",
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
                                            "Slide options",
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

    FPalette *palette=new FPalette("Track", "Highlight", "Album/Group", "activemenuPalette", "Active menu colors", "activemenu-palette");

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
                                                  "User active menu palette",
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
                                     "Path to .mpg slideshow",
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
    connect(selectoptionListWidget->clearList, SIGNAL(clicked()), this, SLOT(on_clearList_clicked()));
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

    //  for (int r=0;  r < hash::FStringListHash[slides->frameHashKey]->at(slides->fileNumber).count();  r++)
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
        selectoptionListWidget->clearList->setToolTip(tr("Erase selected effect list"));
    }
    else
    {
        nextStep->setText("Effects");
        nextStep->setArrowType(Qt::RightArrow);
        nextStep->setToolTip(tr("Step 2: Select transition effects for slides"));
        slides->clearList->setToolTip(tr("Erase file list"));
    }

    selectFilesView=filesView;
}

void stillPage::refreshApplyEffectsIcon()
{

    if (hash::FStringListHash[slides->hashKey()][slides->mainTabWidget->currentIndex()].isEmpty())
        applyEffectsToOneFile->setIcon(applyEffectsToOneFileUntoggledIcon);
    else
        applyEffectsToOneFile->setIcon(applyEffectsToOneFileToggledIcon);
}


RefreshManagerFilter options::RefreshFlag;

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
    connect(closeButton, SIGNAL(accepted()), this, SLOT(closeOptions()));
    connect( this, SIGNAL(registered()), parent, SLOT(saveProject()));
    connect( parent, SIGNAL(clearOptionData()), this, SLOT(clearOptionData()));
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
    refreshOptionFields();
}

/* implement a global clear() function for the FStringList of data in an FListFrame ; it will be used as dvda::clearData() too. Usage below is faulty. */

void options::clearOptionData()
{
    hash::FStringListHash.clear();
    stillTab->slides->fileListWidget->currentListWidget->clear();
    stillTab->selectoptionListWidget->fileListWidget->currentListWidget->clear();
    audioMenuTab->slides->fileListWidget->currentListWidget->clear();
    audioMenuTab->soundtracks->fileListWidget->currentListWidget->clear();
    audioMenuTab->screentext->fileListWidget->currentListWidget->clear();
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


void options::refreshOptionFields()
{

    QListIterator<FAbstractWidget*>  j(Abstract::abstractWidgetList);

    while (j.hasNext())
        j.next()->refreshWidgetDisplay();

}


void options::closeOptions()
{
    options::RefreshFlag =  NoCreate;

    emit(registered());
    accept();
}

