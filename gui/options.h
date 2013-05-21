#ifndef OPTIONS_H
#define OPTIONS_H
#include <QtGui>
#include "common.h"
#include "lplex.h"
#include "dvda-author-gui.h"
#include "viewer.h"
//#include "forms.h"


class standardPage : public common
{
    Q_OBJECT

public :
    standardPage();
    FLineEdit* aspectRatioLineEdit;
    FLineEdit* normTypeLineEdit;

private:
    QListWidget* normWidget, *aspectRatioWidget;
    QGroupBox *normTypeBox, *aspectRatioBox;
    FString aspectRatioMsg, standardMsg;

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

    QPushButton   *videoZoneButton;

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
    QPushButton   *mkisofsButton;

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
    FCheckBox  *logBox, *debugBox,  *htmlFormatBox, *veryverboseBox, *logrefreshBox;
    FRadioBox *createDVDFilesRadioBox;

private:
    QPushButton *logButton,*openLogButton ,*openHtmlLogButton ,*targetDirButton, *openTargetDirButton;
    QFileDialog logDialog;
    QLabel *targetDirLabel;
    FLineEdit* targetDirLineEdit, *binDirLineEdit, *tempDirLineEdit, *workDirLineEdit;
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
    FLineEdit * audioMenuLineEdit;
    FComboBox* menuStyleFComboBox, *highlightFormatFComboBox, *fontFComboBox, *fontSizeFComboBox, *nmenuFComboBox;
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

    QGroupBox *slidesBox;
    QPushButton *openAudioMenuButton;
    QPushButton *audioMenuButton;
    FCheckBox *audioMenuCheckBox;
    QToolButton  *clearList ;
    QPushButton* slidesButton;
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

    FCheckBox *videoImportCheckBox, *audioExportCheckBox;
    FRadioBox *audioExportRadioBox;
    FLineEdit  *videoImportLineEdit, *videoMenuImportLineEdit;
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
QIcon applyEffectsToOneFileUntoggledIcon;
QIcon applyEffectsToOneFileToggledIcon;
QListWidget  *stilloptionListWidget;
QToolButton *nextStep, *clearList,  *applyAllEffects, *applyEffectsToOneFile;

QLabel* stilloptionListLabel;
FLineEdit* videoFileLineEdit;
QPushButton *videoPlayerButton;

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
    static RefreshManagerFilter RefreshFlag;

public slots:

    void refreshOptionFields();

signals:

    void defaultClick(bool);
    void registered();

private:

    QDialogButtonBox *closeButton;
    QListWidget *contentsWidget;
    QStackedWidget *pagesWidget;

    void createIcons();
    void createIcon(const char* path, const char* text);

private slots:

    void closeOptions();
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void clearOptionData();

};
#endif // OPTIONS_H
