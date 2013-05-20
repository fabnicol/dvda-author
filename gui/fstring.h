#ifndef FSTRING_H
#define FSTRING_H

#include <QtCore>
#include "enums.h"

class FString;
class FStringList;

class hash : public QHash<QString, QString>
{
public:
  /* hash::qstring converts a string label like "targetDir" into its associated path, yes/no value, or QString number. To be inserted within <value> tags. */
  static QHash<QString,QString> qstring;

  /* hash::description converts a string like "targetDir" into its (sentence-like) description for display in project manager (first column)*/
  static QHash<QString,QString> description;

  /* hash::fstringlist is only used for FListWidget objects. It is used for storing information for xml project parsing/writing.
   *It converts a string label like "audioMenu" into a pointer to an FStringList object that contains a set of file paths
   * (or more generally, text phrases) grouped into a QStringList for each associated file in a list of files */
  static QHash<QString, FStringList *> fstringlist;

  static void initialize(const QString& hashKey);

};

class FString : public QString
{
private:
 int x;
 QString p;
 inline void testBool(QString &s, flags::status flag=flags::defaultStatus)
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


  FStringList split(QString &);
  FStringList split(QStringList &separator);

  short toBool();
  bool isBoolean();
  bool isFalse();
  bool isTrue();
  bool isMultimodal();
  void setMultimodal();
  bool isFilled();
  void fromBool(bool);
  QString toQString();
  QString& toQStringRef();
};


class FStringList : public QList<QStringList>
{
private:
  QStringList q;

public:

  FStringList( ) : QList<QStringList>() { }
  FStringList(QString &s) : QList<QStringList>()  { this->append(QStringList(s)); }
  FStringList(QString s) : QList<QStringList>()  { this->append(QStringList(s)); }
  FStringList(FString &s):FStringList(s.toQString()) {}
  FStringList(FString *s):FStringList(s->toQString()) {}
  FStringList(const QStringList &L): QList<QStringList>() {this->append(L);}
  FStringList(const  QList<QStringList> &s) : QList<QStringList>(s) {}
  FStringList(QString a, QString b, QString c):QList<QStringList>()  { this->append(QStringList(a));this->append(QStringList(b));this->append(QStringList(c)); }
  FString join(QStringList &);
  FString join(const char* s) {QStringList strL=QStringList((QString(s))); return join(strL);}
  QStringList join();
  QString setEmptyTags(const QStringList &);
  QString setTags(const QStringList &tags, const FStringList *properties=NULL);
  FString toFString() { return ((this->isEmpty()) || this->at(0).isEmpty())?  "" : FString(this->at(0).at(0)); }
  bool hasNoString() { return (isEmpty() || (this->at(0).isEmpty()) || (this->at(0).at(0).isEmpty())); }
  bool isFilled() { return (!isEmpty() && (!this->at(0).isEmpty()) && (!this->at(0).at(0).isEmpty())); }

  /* copy constructor */
  FStringList(const FStringList  & L):QList<QStringList>(L)  { }
};

class FStringListIterator : public QListIterator<QStringList>
{
public:
  FStringListIterator(FStringList list) : QListIterator(list) {}
  FStringListIterator(FStringList *list) : QListIterator(*list) {}
};


#endif // FSTRING_H
