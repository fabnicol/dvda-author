#include <QtCore>
#include <QListWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include "fcolor.h"
#include "fstring.h"


class FStringList;
class common;


class flags
{
protected:
  enum {importFiles, importNames, typeIn, isEmbedded};

  enum commandLineType {dvdaFiles, createDisc, createIso, dvdaExtract, lplexFiles, noCommandLine};
  enum status {
    defaultStatus,
    commandLineTypeMask=0xF,
    untoggledCommandLine=0x00,
    toggledCommandLine=0x10,
    commandLineToggleMask=0xF0,
    enabled=0x000,
    disabled=0x100,
    enabledMask=0xF00,
    widgetMask=0xF000,
    checked=0x0000,
    unchecked=0x1000,
    multimodal=0x2000
    };
};


class FAbstractConnection : QObject
{
  Q_OBJECT

public:

  static void meta_connect(QWidget* w, const QList<QWidget*> *enabledObjects, const QList<QWidget*> *disabledObjects=NULL);
};

class FAbstractWidget : public flags
{
protected:
  QString hashKey;
  QString description;
  QString optionLabel;
  FString commandLineFString;

public:

  static QList<FAbstractWidget*> abstractWidgetList;

  /* is used for .dvp Xml project writing: refresh Widget information and injects current Widget state into hash::qstring as left-valued of <...hashKey=...> */
  virtual FString setXmlFromWidget()=0;

  /* does the reverse of setXmlFromWidget : reads left value of <...hashKey=...> and injects it into commandLineFString. Refreshes Widget state accordingly */
  virtual void setWidgetFromXml(FStringList& )=0;

  /* Refreshes widget state from current value of commandLineFString member to ensure coherence betwenn internal object state and on-screen display */
  virtual void refreshWidgetDisplay()=0;

  /* accessor to privale hashKey value */
  virtual QString getHashKey()  {return hashKey; }

  virtual QString getDescription()  { return description; }


  /* command-line interface maker */
  virtual QString commandLineString();

  /* command-line interface type */
  int commandLineType;

  // isEnabled() cannot be used as it would trigger lexical ambiguity with QWidget-inherited isEnabled() in subclasses
  // yet using virtual derivation makes it possible to invoke the QWidget-inherited isEnabled().
  virtual bool isAbstractEnabled()=0;
  bool isAbstractDisabled() {return !isAbstractEnabled();}

protected:

  template <typename W> void setPrivateFields(W* , QString defaultValue, QString &, QString &, QString, int, QList<QWidget*>* =NULL,  QList<QWidget*>* =NULL);

};



class FListWidget : public QListWidget, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:
  FListWidget(QString& hashKey,int status, QString& description, QString& commandLine, QString& sep,
              QStringList &taglist, QStringList *terms=NULL, QStringList *translation=NULL);

  virtual void setWidgetFromXml(FStringList & );
  virtual FString setXmlFromWidget();
  virtual void refreshWidgetDisplay();
  virtual bool isAbstractEnabled() {return this->isEnabled();}

  int rank;
  int nframes;
  QStringList *fileStringList;

private:

  QString separator;
  QStringList tags;

  template <typename T, typename U> friend void applyListFunction(QHash<T, U > *H, QList<T> *L, QList<U> *M);
  friend  void applyListFunction(QStringList *L, QHash<QString, QString> *H,  const QStringList *M);

  QHash<QString, QString> *listWidgetTranslationHash;

  const QList<QWidget*>* enabledObjects;
  const QList<QWidget*>* disabledObjects;

  FString translate(FStringList &s);

signals:
  void  open_tabs_signal(int);
  void is_fileStringList_changed(int);

};


class FCheckBox : public QCheckBox, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:


  FCheckBox(QString boxLabel, int status, QString hashKey, QString description,
            QString commandLineString,  QList<QWidget*>* enabledObjects=NULL, QList<QWidget*>* disabledObjects=NULL) ;

  FCheckBox(QString boxLabel, int status, QString hashKey, QString description,
             QList<QWidget*>* enabledObjects=NULL, QList<QWidget*>* disabledObjects=NULL):
    FCheckBox(boxLabel,  status | flags::noCommandLine, hashKey, description, "",  enabledObjects, disabledObjects){}

  FCheckBox(QString boxLabel, int status, QString hashKey, QString description,
             QWidget* enabledObject, QWidget* disabledObject=NULL):
    FCheckBox(boxLabel,  status | flags::noCommandLine, hashKey, description, "",
              &(QList<QWidget*>() << enabledObject),  &(QList<QWidget*>() << disabledObject)) {}

  FCheckBox(QString boxLabel, QString hashKey, QString description,
             QList<QWidget*>* enabledObjects=NULL, QList<QWidget*>* disabledObjects=NULL):
    FCheckBox(boxLabel,   flags::noCommandLine, hashKey, description, "",  enabledObjects, disabledObjects){}

  FCheckBox(QString boxLabel, QString hashKey, QString description,
             QWidget* enabledObject, QWidget* disabledObject=NULL):
    FCheckBox(boxLabel,   flags::noCommandLine, hashKey, description, "",
              &(QList<QWidget*>() << enabledObject),  &(QList<QWidget*>() << disabledObject)) {}

  FCheckBox(QString boxLabel, QString hashKey, QString description,
            QString commandLineString,  QList<QWidget*>* enabledObjects=NULL, QList<QWidget*>* disabledObjects=NULL):
                FCheckBox(boxLabel, flags::defaultStatus, hashKey, description,  commandLineString, enabledObjects, disabledObjects){}

  FCheckBox(QString boxLabel, QString hashKey,QString description,
            QString commandLineString,   QWidget* enabledObject, QWidget* disabledObject=NULL):
                FCheckBox(boxLabel, flags::defaultStatus, hashKey, description,  commandLineString,
                          &(QList<QWidget*>() << enabledObject),  &(QList<QWidget*>() << disabledObject)) {}


  virtual void setWidgetFromXml(FStringList& );
  virtual FString setXmlFromWidget();
  virtual void refreshWidgetDisplay();
  virtual bool isAbstractEnabled() {return this->isEnabled();}

  QList<QWidget*>* enabledObjects;
  QList<QWidget*>* disabledObjects;

private slots:
  void fromBool(bool);
  void uncheckDisabledBox();
};


class FRadioBox : public QRadioButton, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:
   FRadioBox(QStringList boxLabelList, int status,QString hashKey, QString description,
                     QStringList commandLineStringList, QList<QWidget*>* enabledObjects=NULL,  QList<QWidget*>* disabledObjects=NULL) ;

   FRadioBox(QStringList boxLabelList, QString hashKey, QString description,
                     QStringList commandLineStringList, QList<QWidget*>* enabledObjects=NULL,  QList<QWidget*>* disabledObjects=NULL) :
     FRadioBox(boxLabelList, 0,hashKey, description,  commandLineStringList, enabledObjects,  disabledObjects) {}

  virtual void setWidgetFromXml(FStringList& );
  virtual FString setXmlFromWidget();
  virtual void refreshWidgetDisplay();
  virtual bool isAbstractEnabled() { return true;}//return this->radioGroupBox->isEnabled();}
  void setToolTip(QString & description) {}//this->radioGroupBox->setToolTip(description);}
  void setEnabled(bool enabled) {}//this->radioGroupBox->setEnabled(enabled);}

   QList<QWidget*>* enabledObjects;
   QList<QWidget*>* disabledObjects;

private:
  uint size;
  QList<QRadioButton*> radioButtonList;
  QGroupBox *radioGroupBox;

};


class FComboBox : public QComboBox, virtual public FAbstractWidget
{
  Q_OBJECT

  friend class FAbstractWidget;

public:

  FComboBox(QStringList &labelList,int status, QString hashKey, QString description, QString commandLine,  QList<QIcon> *iconList=NULL);
  FComboBox(QStringList &labelList, QString hashKey, QString description, QString commandLine,  QList<QIcon> *iconList=NULL):
      FComboBox(labelList, flags::defaultStatus, hashKey, description, commandLine,  iconList){}

  virtual void setWidgetFromXml(FStringList&);
  virtual FString setXmlFromWidget();
  virtual void refreshWidgetDisplay();
  virtual bool isAbstractEnabled() {return this->isEnabled();}
  QStringList *fileStringList;

private:
  const QList<QWidget*>* enabledObjects;
  const QList<QWidget*>* disabledObjects;

private slots:
  void fromCurrentIndex(const QString&);

signals:
  void is_fileStringList_changed(int );

};

class FLineEdit : public QLineEdit, virtual public FAbstractWidget
{
  Q_OBJECT
  friend class FAbstractWidget;

public:
  FLineEdit(QString defaultstring, int status, QString hashKey, QString description, QString commandLine);
  FLineEdit(QString defaultstring, QString hashKey, QString description, QString commandLine):
    FLineEdit(defaultstring, flags::defaultStatus, hashKey, description, commandLine){}

  virtual void setWidgetFromXml(FStringList&);
  virtual FString setXmlFromWidget();
  virtual void refreshWidgetDisplay();
  virtual bool isAbstractEnabled() {return this->isEnabled();}

private:
  const  QList<QWidget*>* enabledObjects;
  const  QList<QWidget*>* disabledObjects;
};



class FColorButton :  public QWidget, virtual public FAbstractWidget
{
 Q_OBJECT

private:
  colorRect* rectIcon;
  QPushButton *button;
  FAbstractWidget *palette;

public:
  FColorButton(FAbstractWidget* parent, const char* text, const char* color);
  int buttonWidth()   const ;
  void setMinimumButtonWidth(const int w);
  void setBrush(const QBrush &brush)
  {
    rectIcon->setBrush(brush);
    rectIcon->update();
  }

  virtual void setWidgetFromXml(FStringList&);
  virtual void refreshWidgetDisplay();
  virtual FString setXmlFromWidget();
  virtual bool isAbstractEnabled() {return this->isEnabled();}

public slots:
  void changeColors();

};

class FPalette :  public QWidget, virtual public FAbstractWidget
{
  Q_OBJECT
  friend class FAbstractWidget;

  private:
    const  QList<QWidget*>* enabledObjects;
    const  QList<QWidget*>* disabledObjects;

  public:
    FPalette(const char* textR, const char* textG, const char* textB, int status , QString& hashKey,QString& description, QString& commandLine);
    FPalette(const char* textR, const char* textG, const char* textB,  QString& hashKey,QString& description, QString& commandLine):
      FPalette(textR, textG, textB, flags::defaultStatus,hashKey,description,commandLine) {}
    virtual void setWidgetFromXml(FStringList&);
    virtual void refreshWidgetDisplay();
    void refreshComponent(short i);
    void setToolTip(QString &);
    FColorButton *button[3];
    virtual FString setXmlFromWidget();
    void setMinimumButtonWidth(const int w);
    virtual bool isAbstractEnabled() {return this->isEnabled();}
};


#endif
