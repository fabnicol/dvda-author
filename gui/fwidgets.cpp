#include "fwidgets.h"
#include "fcolor.h"
#include "common.h"



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
this->setToolTip(desc);\
this->commandLineType=stat | flags::multimodal;\
if (!hK.isEmpty())\
{\
    if (!desc.isEmpty())\
    {\
        this->description=desc;\
        hash::description[hK]=desc;\
    }\
    if (((stat) & flags::commandLinewidgetDepthMask) == flags::hasListCommandLine)\
        hash::qstring[hK]=this->commandLineList[0];\
}\
\
this->optionLabel=opt;\
\
this->setEnabled((((stat) | flags::multimodal)& flags::enabledMask) ==  flags::enabled);\
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

void FAbstractConnection::meta_connect(FAbstractWidget* w,  const Q2ListWidget *enabledObjects,  const Q2ListWidget *disabledObjects)
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


QStringList FAbstractWidget::commandLineStringList()
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
        j.next()->refreshWidgetDisplay();
}

FListWidget::FListWidget(const QString& hashKey,
                         int status,
                         const QString& description,
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

    hash::initializeFStringListHash(hashKey);

    rank=0;
    setObjectName(hashKey+" "+description);
    currentListWidget=new QListWidget;
    currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    currentListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    Q2ListWidget *controlledListWidget=new Q2ListWidget;
    *controlledListWidget << (QList<QWidget*>() << controlledWidget);

    FCore3("", hashKey, description, commandLine, status, controlledListWidget)
    separator=sep;

    tags=taglist;
    signalList=new QStringList;

    /* if a hash has been activated, build the terms-translation hash table so that translated terms
   * can be translated back to original terms later on, so as to get the correct command line string chunks */

    if ((terms == NULL) || (translation == NULL)) listWidgetTranslationHash=NULL;
    else
    {
        listWidgetTranslationHash=new QHash<QString, QString>;
        createHash(listWidgetTranslationHash, translation, terms);

    }

    connect(currentListWidget, SIGNAL(currentRowChanged(int)), SIGNAL(is_signalList_changed(int)));

}



FString FListWidget::translate(const FStringList &s)
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
        if (hash::FStringListHash.contains(hashKey))
            *hash::FStringListHash[hashKey]=s;
        else
            return;
        /* add as many groups as there are QStringLists in excess of 1 */
        if (size) emit(open_tabs_signal(size)) ;

    }
    else
    {
        commandLineList={""};
        return;
    }


    /* for command-line */

    /* if a hash has been activated, strings are saved in Xml projects
    * as "translated" items to be displayed straightaway in list widgets
    * command lines, in this case, need to be translated back to original terms */

    if (listWidgetTranslationHash)
        commandLineList= QList<FString>() << translate(s);
    else
    {
        if ((commandLineType & flags::commandLinewidgetDepthMask) == flags::hasListCommandLine)
        {
            if (separator.size() < 2) commandLineList=QList<FString>();
            FStringListIterator i(hash::FStringListHash[hashKey]);
            while (i.hasNext())
            {
                commandLineList << separator[1] ;
                QStringListIterator j(i.next());
                while (j.hasNext())
                    commandLineList << j.next();
            }
        }
        else
            commandLineList[0]= hash::FStringListHash[hashKey]->join(separator);
    }
}

FString FListWidget::setXmlFromWidget()
{
    if (!hash::FStringListHash.contains(hashKey)) return FStringList().setEmptyTags(tags);

    if (listWidgetTranslationHash)
        commandLineList=QList<FString>() << translate(*hash::FStringListHash[hashKey]);
    else
    {
        if ((commandLineType & flags::commandLinewidgetDepthMask)  == hasListCommandLine)
        {
            if (separator.size() < 2) commandLineList=QList<FString>();
            FStringListIterator i(hash::FStringListHash[hashKey]);
            while (i.hasNext())
            {
                commandLineList << separator[1] ;
                QStringListIterator j(i.next());
                while (j.hasNext())
                    commandLineList << j.next();
            }
        }

        else
            commandLineList[0]=hash::FStringListHash[hashKey]->join(separator);
    }

    return hash::FStringListHash[hashKey]->setTags(tags);
}


void FListWidget::refreshWidgetDisplay()
{
    currentListWidget->clear();
    if ((hash::FStringListHash.contains(hashKey)) && (hash::FStringListHash[hashKey]->count() > rank ))
    currentListWidget->addItems(hash::FStringListHash[hashKey]->at(rank));
}


FCheckBox::FCheckBox(const QString &boxLabel, int status,const QString &hashKey, const QString &description,
                     const QString &commandLineString, const Q2ListWidget* controlledObjects) : QCheckBox(boxLabel)
{
    componentList={this};
    widgetDepth="0";
    bool mode= ((status & flags::widgetMask) == flags::checked) ;
    FCore3(FString(mode), hashKey, description, commandLineString, status, controlledObjects)
}


FCheckBox::FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QString &description,
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

FString FCheckBox::setXmlFromWidget()
{
    commandLineList[0].fromBool(this->isChecked()) ;
    hash::qstring[hashKey]=commandLineList[0];

    return commandLineList[0];
}

void FCheckBox::setWidgetFromXml(const FStringList &s)
{
    QString st=s.toFString();
    commandLineList[0]=FString(st);
    hash::qstring[hashKey]=st;
    this->setChecked( commandLineList[0].isTrue());
}



FRadioBox::FRadioBox(const QStringList &boxLabelList, int status,const QString &hashKey, const QString &description,
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

FString FRadioBox::setXmlFromWidget()
{
    commandLineList[0]=FString(QString::number(rank), flags::multimodal) ;
    hash::qstring[hashKey]=commandLineList[0];

    return commandLineList[0];
}

void FRadioBox::setWidgetFromXml(const FStringList &s)
{
    QString st=s.toFString();
    commandLineList[0]=FString(st, flags::multimodal);
    hash::qstring[hashKey]=st;
    rank=st.toUInt();
    for (int i=0; i < size ; i++)
        radioButtonList[i]->setChecked(i == rank);
}


FComboBox::FComboBox(const QStringList &labelList,
                     const QStringList &translation,
                     int status,
                     const QString &hashKey,
                     const QString &description,
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

    /* if a hash has been activated, build the terms-translation hash table so that translated terms
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

FString FComboBox::setXmlFromWidget()
{
    commandLineList[0]= (comboBoxTranslationHash)? comboBoxTranslationHash->value(currentText()) : currentText();
    if (commandLineList[0].isEmpty()) commandLineList[0]="none";
    hash::qstring[hashKey]=commandLineList[0];
    return commandLineList[0];
}


void FComboBox::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();

    hash::qstring[hashKey]=commandLineList[0];

    refreshWidgetDisplay();

}


FLineEdit::FLineEdit(const QString &defaultString, int status, const QString &hashKey, const QString &description, const QString &commandLine):QLineEdit()
{
    widgetDepth="0";
    FCore(defaultString, hashKey, description, commandLine, status, NULL, NULL);
}


void FLineEdit::refreshWidgetDisplay()
{
    this->setText(commandLineList[0].toQString());
}

FString FLineEdit::setXmlFromWidget()
{
    commandLineList[0]=FString(this->text());
    if (commandLineList[0].isEmpty()) commandLineList[0]="none";
    hash::qstring[hashKey]=this->text();
    return commandLineList[0];
}

void FLineEdit::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();
    hash::qstring[hashKey]=commandLineList[0].toQString();
    this->setText(commandLineList[0].toQString());
}


FColorButton::FColorButton(const char* text, const char* color)
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
    commandLineList=QList<FString>() << RGBStr2YCrCbStr(color+1);
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


FString FColorButton::setXmlFromWidget()
{

    hash::qstring[hashKey]=commandLineList[0];
    return commandLineList[0];
}

void FColorButton::setWidgetFromXml(const FStringList &s)
{
    commandLineList[0] = s.toFString();
    hash::qstring[hashKey]=commandLineList[0];
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
                   const QString &description,
                   const QString &commandLine,
                   int buttonWidth)
{
    widgetDepth="1";
    button[0]=new FColorButton(textR, DEFAULT_COLOR_0); // red RGB
    button[1]=new FColorButton(textG, DEFAULT_COLOR_1); // green RGB
    button[2]=new FColorButton( textB, DEFAULT_COLOR_2); //blue RGB

    hash::initializeFStringListHash(hashKey);

    FCore( "000000:000000:000000",  hashKey, description, commandLine, status, NULL, NULL);
    setMinimumButtonWidth(buttonWidth);
}


FString FPalette::setXmlFromWidget()
{

    FStringList L=FStringList(button[0]->setXmlFromWidget(), button[1]->setXmlFromWidget(), button[2]->setXmlFromWidget());
    commandLineList[0]=L[0][0] + ":"+ L[1][0] + ":"+ L[2][0];

    if (hash::FStringListHash.contains(hashKey))  *hash::FStringListHash[hashKey]=L;
    return L.setTags({ "YCrCb"});
}

void FPalette::setWidgetFromXml(const FStringList &s)
{
    if (hash::FStringListHash.contains(hashKey)) *hash::FStringListHash[hashKey]=s;
    commandLineList[0]=s.join(":");
    refreshWidgetDisplay();
}

void FPalette::refreshComponent(short i)
{
    if (!hash::FStringListHash.contains(hashKey)) return;
    if (hash::FStringListHash[hashKey]->size() != 1) return;
    if  (hash::FStringListHash[hashKey]->at(0).size() != 3) return;

    FString str=hash::FStringListHash[hashKey]->at(0).at(i);

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
