#ifndef FSTRING_H
#define FSTRING_H

#include <QtCore>
#include "enums.h"

class FString;
class FStringList;


class Hash : public QHash<QString, QString>
{
public:

  /* Hash::description converts a string like "targetDir" into its (sentence-like) description for display in project manager (first column)*/
  static QHash<QString,QStringList> description;

  /* Hash::wrapper  is used for storing information for xml project parsing/writing.
   *It converts a string label like "audioMenu" into a pointer to an FStringList object that contains a set of file paths
   * (or more generally, text phrases) grouped into a QStringList for each associated file in a list of files */
  static QHash<QString, FStringList *> wrapper;

};

class FString : public QString
{
private:
 int x;
 QString p;
 void testBool(QString &s, flags::status flag=flags::defaultStatus)
 {
 if (s.isEmpty())
   x=2;
 else
   {
     if (s == QString("yes"))
       x=1;
     else
     if (s == QString("no"))
       x=0;
     else
       // Preserving flagged status
         x= (flag != flags::defaultStatus)? flag : 2;
    }
 }

public:

  FString  operator | (FString   );
  FString  operator ! ();
  FString  operator & (FString  );
  FString  operator & (bool );
  void  operator &= (FString  );
  void  operator &= (bool );
  FString  operator * ();

  FString()
  {
    x=2;
    p="";
  }

  FString(QString s, flags::status flag=flags::defaultStatus):QString(s)
  {
    p=s;
    testBool(s, flag);
  }

  FString(const char* s):FString(QString(s))  {  }

  FString(bool value)
  {
    x=value;
    if (value) p="yes"; else p="no";
    this->append(p); // caution! does not work otherwise
  }

  /* copy constructor */
  FString(const FString  & v):QString(v.p)
  {
    x=v.x;
    p=v.p;
  }


  const FStringList& split(const QString &) const;
  const FStringList& split(const QStringList &separator) const;

  short toBool();
  bool isBoolean();
  bool isFalse();
  bool isTrue();
  bool isMultimodal();
  void setMultimodal();
  bool isFilled();
  const FString fromBool(bool);
  QString toQString() const;
  QString& toQStringRef();
};


class FStringList : public QList<QStringList>
{
private:
  QStringList q;

public:

  FStringList( ) : QList<QStringList>() { }
  FStringList(const QString& s) : QList<QStringList>()  { this->append(QStringList(s)); }
  FStringList(const FString &s):FStringList(s.toQString()) {}
  FStringList(const FString *s):FStringList(s->toQString()) {}
  FStringList(const QStringList &L): QList<QStringList>() {this->append(L);}
  FStringList(const  QList<QStringList> &s) : QList<QStringList>(s) {}
  FStringList(const  QList<QVariant> &s) : QList<QStringList>()
  {
      QListIterator<QVariant> i(s);
      while (i.hasNext())
      {
          QVariant v=i.next();
          if (!v.isValid()) continue;
          if (v.canConvert(QMetaType::QStringList))
                        this->append(v.toStringList());
      }
  }

  FStringList(const QString& a, const QString& b, const QString& c):QList<QStringList>()  { this->append(QStringList() << a << b << c);}
  const FString join(const QStringList &) const ;
  const FString& join(const char* s) const {return join(QStringList((QString(s)))); }
  const QStringList& join() ;
  QString setEmptyTags(const QStringList &)const;
  const QString setTags(const QStringList &tags, const FStringList *properties=NULL) const;
  FString toFString() const {if (this == nullptr) return ""; else return ((this->isEmpty()) || this->at(0).isEmpty())?  "" : FString(this->at(0).at(0)); }
  QString toQString() const { return ((this->isEmpty()) || this->at(0).isEmpty())?  "" : QString(this->at(0).at(0)); }
  int toInt() const {return ((this->isEmpty() || this->at(0).isEmpty())? 0: this->at(0).at(0).toInt());}
  bool hasNoString() const { return (isEmpty() || (this->at(0).isEmpty()) || (this->at(0).at(0).isEmpty())); }
  bool  isFilled() const { return (!isEmpty() && (!this->at(0).isEmpty()) && (!this->at(0).at(0).isEmpty())); }

  /* copy constructor */
  FStringList(const FStringList  & L):QList<QStringList>(L)  { }
};

class FStringListIterator : public QListIterator<QStringList>
{
public:
  FStringListIterator(const FStringList& list) : QListIterator(list) {}
  FStringListIterator(FStringList *list) : QListIterator(*list) {}
  FStringListIterator(const FStringList *list) : QListIterator(*list) {}
 };


#endif // FSTRING_H
