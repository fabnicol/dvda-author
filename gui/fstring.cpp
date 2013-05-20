#include "fstring.h"

QHash<QString,QString>    hash::description;
QHash<QString, QString >   hash::qstring;
QHash<QString, FStringList* >   hash::fstringlist;

void hash::initialize(const QString &hashKey)
{
    hash::fstringlist[hashKey]=new FStringList;
    *hash::fstringlist[hashKey] << QStringList();
}

FString   FString::operator & (FString  s)
{
  if (x * s.x == 1) return "yes";
  else return "no";
}


FString   FString::operator & (bool  s)
{
  if (x * s ==1) return "yes";
  else return "no";
}

void   FString::operator &= (bool  s)
{
  x = x & s;
  if (x == 1) p="yes";
  else p="no";
}

void   FString::operator &= (FString  s)
{
  x = x & s.x;
  if (x ==1) p="yes";
  else p="no";
}


FString   FString::operator | (FString  s)
{
  if ((x == 1) || (s.x == 1) )return "yes";
  else return "no";
}

FString   FString::operator ! ()
{
  switch (x)
    {
      case  1:  return "no"; break;
      case  0:  return "yes"; break;
      default:  return "no";
    }
}

QString FString::toQString()
{
  return p;
}


QString& FString::toQStringRef()
{
  return p;
}

FString  FString::operator * ()
{
  return hash::qstring[p];
}

short FString::toBool()
{
  if ( x > 1) return 0;
  else return x;
}

bool FString::isFilled()
{
  return (!p.isEmpty());
}

void FString::fromBool(bool value)
{
  x=value;
  if (value) p="yes"; else p="no";
}

bool FString::isTrue()
{
  return (x == 1);
}

bool FString::isMultimodal()
{
  return (x == flags::multimodal);
}

void FString::setMultimodal()
{
  x = flags::multimodal;
}

bool FString::isFalse()
{
  return (x == 0);
}

bool FString::isBoolean()
{
  return ((x == 0) | (x == 1));
}

FStringList FString::split(QString &sep)
{
   QStringList Q=QStringList();
  for (int i=0; i < sep.size(); i++)
       Q << QString(sep[i]) ;
  return split(Q);
}


FStringList FString::split(QStringList &separator)
{
  if (this->isEmpty()) return FStringList();

  short length=separator.size();

  // switch cannot be used here because of QListIterator declaration

  switch (length)
    {

    case 1:
      return this->toQString().split(separator[0]); break; //Beware of calling toQString() to avoid infinite loop.

    case 2:
      if (this->length() >= 3)
        {
            QListIterator<QString> i=QListIterator<QString>(this->toQString().split(separator[1]));
            QList<QStringList> L=QList<QStringList>();
            while (i.hasNext())
            L << i.next().split(separator[0]);
            return L;
        }

    default:
      return FStringList(this);
    }
}



QStringList FStringList::join()
{
  // Flattens FStringList into QStringList

  FStringListIterator i(this);
  QStringList S=QStringList();
  while (i.hasNext())
    {
      S << i.next();
    }
  return S;
}

FString FStringList::join(QStringList &separator)
{
  if (this == NULL) return "";
  if (this->count() == 0) return "";

  QStringList S;
  QString sep;
  FStringListIterator i(this);
  short length=separator.length();

  if (length == 0)
    {
      QString str=QString();
      for (int i=0; i < this->count() ; i++)
        str += this->at(i).join("");
      return FString(str);
    }

  if (length == 1)
    {
      if (count() == 1)
        {
          sep=QString(separator[0]);
          return this->at(0).join(sep);
        }
      return "";
    }

  if (length == 2)
    {
      sep=QString(separator[0]);
      if (count() == 1)
        {
          return this->at(0).join(sep);
        }

      S=QStringList();

      while (i.hasNext())
        S  << i.next().join(sep);
      sep=QString(separator[1]);
      return FString(S.join(sep));
    }

  return ("");
}


inline QStringList setDistributedTags(const QString & tag,const QStringList &properties, const QStringList &tagged)
{
  QStringList taggedList;
  taggedList =  (tagged.isEmpty()) ? QStringList("") : tagged;
  QStringListIterator i(taggedList);
  QStringList S=QStringList();
  while (i.hasNext())
    {
      QString item=i.next();
      S << QString("<") + tag+" "+  properties.join(" ") + QString(">")+ item +QString("</")+tag+QString(">");
    }
  return S;
}


 QString FStringList::setEmptyTags(const QStringList & tags)
{
  QStringListIterator i(tags);
  QString S="";
  while (i.hasNext())
    {
      QString tag=i.next();
      S =  "<" + tag + ">" + S + "</" + tag + ">" ;
    }
  return S;
}


QString FStringList::setTags(const QStringList  &tags, const FStringList *properties )
{
  if ((this == NULL) ||  this->hasNoString())
    return setEmptyTags(tags);

  if ((properties) && (properties->size() != 2) )
      return setEmptyTags(tags);

  QStringList S;

  if (tags.length() < 3 )
    {

      for (int i=0; i < size(); i++)
        {
          QStringList tagged=this->at(i);
          if  (tagged.isEmpty()) continue;
          QString str;
          if  (properties)
            str=setDistributedTags(tags[0], properties->at(0), tagged).join("\n      ");
          else
            str=setDistributedTags(tags[0], QStringList(), tagged).join("\n      ");
          S << "\n      "   + str   + "\n     ";
        }

      if (tags.length() == 1)
        {
          return S.join("");
        }
      else
        if (tags.length() == 2)
          {
          QString str;
          if (properties)
             str=setDistributedTags(tags[1],  properties->at(1),  S).join("\n     ");
         else
             str=setDistributedTags(tags[1],  QStringList(),  S).join("\n     ");

          return ( "\n     " + str  + "\n    ");
          }
    }
  return setEmptyTags(tags);
}
