#ifndef FWIDGETS_H
#define FWIDGETS_H

#include <QtWidgets>
#include "fcolor.h"
#include "fstring.h"
#include "FProgressBar.h"
#include "tags.h"

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
