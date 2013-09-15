#include "fwidgets.h"
#include "common.h"
#include "fcolor.h"


/* using above function with controlled object encapsulation */



void applyHashToStringList(QStringList *L, QHash<QString, QString> *H,  const QStringList *M)
{
    if ((H == nullptr) || (M == nullptr) || (L == nullptr)) return;
    QStringListIterator j(*M);

    while (j.hasNext())
        *L << (*H) [j.next()];
}

template <typename T, typename U> void createHash(QHash<T, U > *H, const QList<T> *L, const QList<U> *M)
{
    if ((H == nullptr) || (L == nullptr) || (M == nullptr)) return;
    QListIterator<T> i(*L);
    QListIterator<U> j(*M);

    while ((j.hasNext()) && (i.hasNext()))
        (*H)[i.next()]=j.next();
}

void FAbstractConnection::meta_connect(const FAbstractWidget* w,  const Q2ListWidget *enabledObjects,  const Q2ListWidget *disabledObjects)
{
    if ((enabledObjects != nullptr) &&  (!enabledObjects->isEmpty()) )
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
                if ((item == nullptr) || (component==nullptr)) continue;
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

    if ((disabledObjects != nullptr) &&  (!disabledObjects->isEmpty()))
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

                if ((item == nullptr) || (component==nullptr)) continue;

                connect(component, SIGNAL(toggled(bool)), item , SLOT(setDisabled(bool)));

                // this connection must follow the general enabling adjustments above
                if (item->metaObject()->className() == QString("FCheckBox"))
                    connect(component, SIGNAL(toggled(bool)), item, SLOT(uncheckDisabledBox()));
            }
        }
    }
}


inline void FAbstractWidget::FCore(const QList<QWidget*>& w, FString defaultCommandLine, int commandLineType, const QString &hashKey, const QStringList & description,
                  const QString &option, const QList<QWidget*>&enabledObjects, const QList<QWidget*>&disabledObjects)
{
    Q2ListWidget *dObjects=new Q2ListWidget, *eObjects=new Q2ListWidget;
    if (enabledObjects.isEmpty()) eObjects=nullptr;
    else
        *eObjects << enabledObjects;
    if (disabledObjects.isEmpty()) dObjects=nullptr;
    else
        *dObjects << disabledObjects;

    FCore(w, defaultCommandLine, commandLineType, hashKey, description, option, eObjects, dObjects);
}

inline void FAbstractWidget::FCore(const QList<QWidget*>& w,FString defaultCommandLine,  int status, const QString &hashKey, const QStringList & description,
                  const QString &option, const Q2ListWidget *enabledObjects, const Q2ListWidget *disabledObjects)
{
    this->enabledObjects=enabledObjects;
    this->disabledObjects=disabledObjects;

    w.at(0)->setToolTip(description.at(1));
    w.at(0)->setEnabled((status & flags::status::enabledMask) ==  flags::status::enabled);

    this->commandLineList= QList<FString>() << defaultCommandLine;
    this->componentList= w;
    if ((status & flags::status::widgetMask) == flags::status::multimodal) { this->commandLineList[0].setMultimodal(); }
    this->hashKey=hashKey;
    this->commandLineType=static_cast<flags::commandLineType>(status & static_cast<int>(flags::commandLineType::commandLineMask));
    this->status=static_cast<flags::status>(status & static_cast<int>(flags::status::statusMask));;
    if (!hashKey.isEmpty())
            {
                if (!description.isEmpty())\
                {
                     this->description=description;
                    Hash::description[hashKey]=description;
                }
            }
    this->optionLabel=option;
    Hash::wrapper[hashKey] = new FStringList;
    *Hash::wrapper[hashKey]  << (QStringList() << QString());
    Abstract::abstractWidgetList.append(this);
    FAbstractConnection::meta_connect(this, this->enabledObjects, this->disabledObjects);

}


const QStringList FAbstractWidget::commandLineStringList()
{
    /* If command line option is ill-formed, or if a corresponding checkbox is unchecked (or negatively checked)
  * or if an argument-taking option has no-argument, return empty */
//QMessageBox::warning(nullptr, "", optionLabel+":"+QString::number(commandLineType & flags::::widgetMask));

    if (
            (optionLabel.isEmpty()) ||
              (commandLineList[0].isFalse())
           // ||  (commandLineList[0].toQString().isEmpty())
            ||  (this->isAbstractDisabled())) return {};

    QStringList strL=QStringList();

      if ((this->status & flags::status::widgetMask) == flags::status::hasListCommandLine)
        {
            QListIterator<FString> i(commandLineList);
            while (i.hasNext())
                if (!i.peekNext().isEmpty()) strL <<  i.next().trimmed();
               else i.next();

         }
      else
    {
        if (commandLineList[0].isTrue() | commandLineList[0].isMultimodal())
        {
            if  (optionLabel.size() == 1)   strL= QStringList("-"+optionLabel);
            else
            if (optionLabel.at(0) == '^')  strL=QStringList(optionLabel.mid(1));
            else strL=QStringList ("--" +optionLabel);
        }
        else
        {
            if (!commandLineList[0].toQString().isEmpty())
                 strL= (optionLabel.size() == 1)? QStringList("-"+optionLabel+" "+commandLineList[0].toQString())
                                                                       :QStringList("--"+optionLabel+"="+commandLineList[0].toQString());
        }
    }
              return QStringList(strL);
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

    widgetDepth="2";

    Abstract::initializeFStringListHash(hashKey);

    setObjectName(hashKey+" "+description.join(" "));

    currentListWidget=new QListWidget;
    currentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    currentListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    componentList=QList<QWidget*>() << currentListWidget;

    componentList[0]->setToolTip(description.at(1));
    setEnabled((status& flags::status::enabledMask) ==  flags::status::enabled);

    FCore({this}, "", status, hashKey, description, commandLine, QList<QWidget*>() << controlledWidget);

    separator=sep;

    tags=taglist;
    signalList=new QStringList;

    /* if a Hash has been activated, build the terms-translation Hash table so that translated terms
   * can be translated back to original terms later on, so as to get the correct command line string chunks */

    if ((terms == nullptr) || (translation == nullptr))
        listWidgetTranslationHash=nullptr;
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
        if ((this->status & flags::status::widgetMask) == flags::status::hasListCommandLine)
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
        if ((this->status & flags::status::widgetMask)  == flags::status::hasListCommandLine)
        {
            if (separator.size() < 2) commandLineList=QList<FString>();
            FStringListIterator i(Hash::wrapper[hashKey]);

            while (i.hasNext())
            {
                QStringList str=i.next();
                if (str.isEmpty()) continue;
                commandLineList << separator[1] ;
                QStringListIterator j(str);
                while (j.hasNext())
                {
                    QString s=j.next();
                    commandLineList << s;

                }
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



FCheckBox::FCheckBox(const QString &boxLabel, int status, const QString &hashKey, const QStringList &description, const char* commandLineString,
                     const QList<QWidget*> &enabledObjects, const QList<QWidget*> &disabledObjects) : QCheckBox(boxLabel)
{
    bool mode= ((status & flags::status::widgetMask) == flags::status::checked) ;

    FCore({this}, FString(mode), status, hashKey, description, commandLineString, enabledObjects, disabledObjects);
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
            if (item == nullptr) continue;
            item->setEnabled(checked);
        }
    }

    if ((disabledObjects) && (disabledObjects->size()))
    {
        QListIterator<QWidget*> i(disabledObjects->at(0));
        while (i.hasNext())
        {
            QWidget* item=i.next();
            if (item == nullptr) continue;
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
    rank=0;
    if (boxLabelList.size() != (size+1)) return;
    QStringListIterator i(boxLabelList);
    radioButtonList=QList<QRadioButton*> ();
    QList<QWidget*> componentList;
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

    FCore(componentList, "0", status| flags::status::multimodal, hashKey, description, optionLabelStringList[0],enabledObjects, disabledObjects);

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
    mainLayout->setMargin(0);
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

    if ((enabledObjects != nullptr) && (!enabledObjects->isEmpty()))
    {
        QListIterator<QWidget*> i(enabledObjects->at(0));

        while (i.hasNext())
        {
            QWidget *item=i.next();
            if (item == nullptr) continue;
            item->setEnabled(rank ==0);
        }
    }

    if ((disabledObjects != nullptr) && (!disabledObjects->isEmpty()))
    {
        QListIterator<QWidget*> i(disabledObjects->at(0));
        while (i.hasNext())
        {
            QWidget* item=i.next();
            if (item == nullptr) continue;
            item->setDisabled(rank ==0);
        }
    }
}

const FString FRadioBox::setXmlFromWidget()
{
    commandLineList[0]=FString(QString::number(rank), flags::status::multimodal) ;
    *Hash::wrapper[getHashKey()]=FStringList(commandLineList[0]);
    return commandLineList[0].toQStringRef();
}

void FRadioBox::setWidgetFromXml(const FStringList &s)
{
    QString st=s.toFString();
    commandLineList[0]=FString(st, flags::status::multimodal);
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

    FCore({this}, labelList.at(0), status, hashKey, description, commandLine);

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

    if ((labelList.isEmpty()) || (translation.isEmpty())) comboBoxTranslationHash=nullptr;
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

    FCore({this}, defaultString, status, hashKey, description, commandLine);
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
    button->setToolTip(strtext);

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

    FCore({this}, "000000:000000:000000", status, hashKey, description, commandLine);
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



void FPalette::setMinimumButtonWidth(const int w)
{
    button[0]->setMinimumButtonWidth( w);
    button[1]->setMinimumButtonWidth( w);
    button[2]->setMinimumButtonWidth( w);
}

