#include "fstring.h"

QHash<QString,QStringList>    Hash::description;
QHash<QString, FStringList* >   Hash::wrapper;


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

QString FString::toQString() const
{
  return p;
}


QString& FString::toQStringRef()
{
  return p;
}

FString  FString::operator * ()
{
  return Hash::wrapper[p]->toFString();
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

const FString  FString::fromBool(bool value)
{
  x=value;
  if (value) p="yes"; else p="no";
  return FString(p);
}

bool FString::isTrue()
{
  return (x == 1);
}

bool FString::isMultimodal()
{
  return (x == static_cast<int>(flags::status::multimodal));
}

void FString::setMultimodal()
{
  x = static_cast<int>(flags::status::multimodal);
}

bool FString::isFalse()
{
  return (x == 0);
}

bool FString::isBoolean()
{
  return ((x == 0) | (x == 1));
}

const FStringList& FString::split(const QString &sep) const
{
   QStringList Q=QStringList();
  for (int i=0; i < sep.size(); i++)
       Q << QString(sep[i]) ;
  return split(Q);
}


const FStringList& FString::split(const QStringList &separator) const
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



const QStringList& FStringList::join()
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

const FString  FStringList::join(const QStringList &separator) const
{
  if (this == NULL) return "";
  if (this->size() == 0) return "";

  QStringList S;
  QString sep, str;
  FStringListIterator i(this);

   switch(separator.length())
    {
     case 0:
      for (int i=0; i < size() ; i++)
        str += this->at(i).join("");
      return FString(str);

    case 1:
       if (size() == 1)
          return this->at(0).join(separator[0]);
      return "";

   case 2:
      sep=QString(separator[0]);
      if (size() == 1)
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
      S << QString("<") + tag+ (properties.isEmpty() ? "": " ")+  properties.join(" ") + QString(">")+ item +QString("</")+tag+QString(">");
    }
  return S;
}


QString FStringList::setEmptyTags(const QStringList & tags) const
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


const QString FStringList::setTags(const QStringList  &tags, const FStringList *properties ) const
{
  if ((this == NULL) ||  this->hasNoString())
  {
      // should trigger a crash
      return setEmptyTags(tags);
  }


    // deactivated

  if ((properties) && (properties->size() != 2) )
      return setEmptyTags(tags);
   // deactivated

  QStringList S;

  if (tags.length() >= 3 ) return setEmptyTags(tags);

  for (int i=0; i < size(); i++)
    {
      QStringList tagged=this->at(i);
      if  (tagged.isEmpty()) continue;
      QString str;
      if  (properties)
        str="     "+ setDistributedTags(tags[0], properties->at(0), tagged).join("\n     ");
      else
        str="     "+ setDistributedTags(tags[0], QStringList(), tagged).join("\n     ");
      S << "\n"   + str   + "\n   ";
    }

  QString str;

  if (tags.length() == 1)
    {
      return S.join("");
    }
  else
    if (tags.length() == 2)
      {

      if (properties)
          str=setDistributedTags(tags[1],  properties->at(1),  S).join("\n   ");
     else
          str=setDistributedTags(tags[1],  QStringList(),  S).join("\n   ");
      }

   return (str);
}
