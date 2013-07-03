#include "dvda.h"


namespace XmlMethod
{
    /* parses < tag> text </tag> */

   QTreeWidgetItem *itemParent=NULL;

  inline QString stackTextData(const QDomNode & node, QString &tag)
    {
        QDomNode  childNode=node.firstChild();
        QString stackedInfo;

        tag = node.toElement().tagName();

        while ((!childNode.isNull()) && (childNode.nodeType() == QDomNode::TextNode))
          {
            stackedInfo += childNode.toText().data().simplified();

            childNode=childNode.nextSibling();
          }
        return stackedInfo;
    }

/*
 * parses < tags[0]>
                     <tags[1]>  text </tags[1]>
                     ....
                     <tags[1]> text </tags[1]>
                </tags[0]>
     note: does not check subordinate tag uniformity
*/

inline QStringList stackFirstLevelData(const QDomNode & node, QStringList &tags)
    {
        QDomNode  childNode=node.firstChild();
        QStringList stackedInfo;

        tags[0]=node.toElement().tagName();

        while (!childNode.isNull())
          {
            stackedInfo << stackTextData(childNode, tags[1]);
            childNode=childNode.nextSibling();
          }
        return stackedInfo;
    }

/*
 *   parses
 *            <tags[0]>
 *               <tags[1]>
                     <tags[2]>  text </tags[2]>
                     ....
                     <tags[2]> text </tags[2]>
                 </tags[1]>
                 ...
                 <tags[1]>
                     <tags[2]>  text </tags[2]>
                     ....
                     <tags[2]> text </tags[2]>
                 </tags[1]>
               </tags[0]>
*/


inline QList<QStringList> stackSecondLevelData(const QDomNode & node, QStringList &tags)
    {
        tags[0]=node.toElement().tagName();
        QDomNode  childNode=node.firstChild();
        QList<QStringList> stackedInfo;

        while (!childNode.isNull())
          {
            QStringList L={QString(), QString()};
            stackedInfo << stackFirstLevelData(childNode, L);
            tags[1]=L.at(0);
            tags[2]=L.at(1);
            childNode=childNode.nextSibling();
          }
        return stackedInfo;
    }

/* computes sizes and sends filenames to main tab Widget */


/* displays on manager tree window */

void displayTextData(const QString &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                     const QColor &color=QColor("blue"));

void displayTextData(const QString &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                                 const QColor &color)
{
          QTreeWidgetItem* item = new QTreeWidgetItem(XmlMethod::itemParent);
           item->setText(0, firstColumn);
           item->setText(1, secondColumn);
           if (!thirdColumn.isEmpty()) item->setText(2, thirdColumn);
           if (color.isValid()) item->setTextColor(2, color);
}



/* tags[0] k
 *                       tags[1] 1 : xxx  ...  size MB
 *                       tags[1] 2 : xxx  ...  size MB  */

inline qint64 displaySecondLevelData(    const QStringList &tags,
                                              const QList<QStringList> &stackedInfo,
                                              const QList<QStringList> &stackedSizeInfo)
    {
      int k=0, count=0, l;
      qint64 filesizecount=0;
      QString  firstColumn, root=tags.at(1), secondColumn=tags.at(2), thirdColumn;

      QListIterator<QStringList> i(stackedInfo), j(stackedSizeInfo);

      while ((i.hasNext()) && (j.hasNext()))
       {
          if (!root.isEmpty())
           {
               firstColumn = root + " "+QString::number(++k);
           }

          displayTextData(firstColumn, "", "");

           QStringListIterator w(i.next()), z(j.next());
           l=0;
           while ((w.hasNext()) && (z.hasNext()))
           {
              ++count;
               if (!tags.at(2).isEmpty())
                   secondColumn =  tags.at(2) +" " +QString::number(++l) + "/"+ QString::number(count) +": ";
               secondColumn += w.next()  ;

               if ((stackedSizeInfo.size() > 0) && (z.hasNext()))
               {
                   qint64 msize=z.next().toLongLong();
                   filesizecount += msize;
                   // force coertion into float or double using .0
                   thirdColumn    = QString::number(msize/1048576.0, 'f', 1) + "/"+  QString::number(filesizecount/1048576.0, 'f', 1)+ " MB" ;
               }

               displayTextData("", secondColumn, thirdColumn, (z.hasNext())? QColor("navy"): ((j.hasNext())? QColor("orange") :QColor("red")));

           }
         }
      return filesizecount;
     }


/* tags[0]
 *                       tags[1] 1 : xxx  ...  (size MB)
 *                       tags[1] 2 : xxx  ...  (size MB) ... */

inline qint64 displayFirstLevelData( const QStringList &tags,  const QStringList &stackedInfo)
    {
         return displaySecondLevelData( tags, QList<QStringList>() << stackedInfo, QList<QStringList>());
    }


}  // end of XmlMethod namespace


void dvda::DomParser(QIODevice* file)
{
  // Beware: to be able to interactively modify managerWidget in the DomParser child class constructor,
  // pass it as a parameter to the constructor otherwise the protected parent member will be accessible yet unaltered
  file->seek(0);

  QString errorStr;
  int errorLine;
  int errorColumn;

  QDomDocument doc;
  if (!doc.setContent(file, true, &errorStr, &errorLine, &errorColumn))
    {
      QMessageBox::warning(0, tr("DOM Parser"), tr("Parse error at line %1, " "column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr));
      return;
    }

  QDomElement root=doc.documentElement();

  if (root.tagName() != "project") return;

  parent->recentFiles.clear();

  QDomNode node= root.firstChild();

  /* this stacks data into relevant list structures, processes information
   * and displays it in the manager tree Widget  */

  if (!xmlDataWrapper.isEmpty()) xmlDataWrapper.clear();
  dvda::totalSize=0;

  for (QString maintag : {"data", "system", "recent"})
  {
      parseXmlNodes(node, maintag);
      node = node.nextSibling();
  }

  /* this assigns values to widgets (line edits, checkboxes, list widgets etc.)
   * in the Options dialog and ensures fills in main tab widget */

  if ((dvda::RefreshFlag&UpdateTabMask) == UpdateTabs)
  {
      assignVariables(xmlDataWrapper);

      // adds extra information to main window and sets alternating row colors

      for (int ZONE : {AUDIO, VIDEO})
          for (int group_index=0; group_index<= project[ZONE]->getRank(); group_index++)
          {
              int r=0;
              for (QString text : xmlDataWrapper[ZONE][group_index])
              {
                  if (!text.isEmpty())
                         assignGroupFiles(ZONE, group_index, fileSizeDataBase[ZONE].at(group_index).at(r),QDir::toNativeSeparators(text));
                  r++;
              }
              refreshRowPresentation(ZONE, group_index);
          }

  }

  /* resets recent files using the ones listed in the dvp project file */

  parent->updateRecentFileActions();

  /* used to connect to slides, soundtracks and other option list widgets in Options dialog :
   * these will be activated depending on main tab widget information */

   emit(is_signalList_changed(project[AUDIO]->signalList->size()));
}



void dvda::parseXmlNodes(const QDomNode &node, const QString &maintag)
{
    QTreeWidgetItem *item=new QTreeWidgetItem(managerWidget);
    if (node.toElement().tagName() != maintag) return;
    item->setText(0, maintag);
    item->setExpanded(true);

    QDomNode subnode=node.firstChild();

    while (!subnode.isNull())
      {
          FStringList str=parseEntry(subnode, item);
          xmlDataWrapper <<   str;
          subnode=subnode.nextSibling();
      }
}


FStringList dvda::parseEntry(const QDomNode &node, QTreeWidgetItem *itemParent)
{
  /* first examine the <hashKey widgetDepth=... > node */

  XmlMethod::itemParent = itemParent;

  QString tag, textData;
  QStringList firstLevelData, tags={QString(),QString(),QString()} ;
  QList<QStringList> secondLevelData, fileSizeData;

  /* then process the  node according to its predicted widgetDepth */


  switch (node.toElement().attribute("widgetDepth").toInt())
      {
      case 0 :
                textData=XmlMethod::stackTextData(node, tag);
                XmlMethod::displayTextData(hash::description[tag], textData, "");
                /* recent file resetting following project */
                if (tag == "file")
                      parent->recentFiles.append(textData);

                return FStringList(textData);

      case 1 :
                firstLevelData=XmlMethod::stackFirstLevelData(node, tags);
                XmlMethod::displayFirstLevelData({"", hash::description[tags.at(0)], hash::description[tags.at(1)]}, firstLevelData);
                return FStringList(firstLevelData);

       case 2 :
              secondLevelData=XmlMethod::stackSecondLevelData(node, tags);
              fileSizeData=processSecondLevelData(secondLevelData, tags.at(2) == "file");
              dvda::totalSize+=XmlMethod::displaySecondLevelData(
                          {tags.at(0), tags.at(1), tags.at(2)},
                          secondLevelData,
                          fileSizeData);

              if (tags.at(0) == "DVD-A")
              {
                  fileSizeDataBase[AUDIO] =  fileSizeData;
              }
              else
              if (tags.at(0) == "DVD-V")
               {
                   fileSizeDataBase[VIDEO] =  fileSizeData;
               }

              return FStringList(secondLevelData);
      }

  return FStringList();
}


inline QList<QStringList> dvda::processSecondLevelData(QList<QStringList> &L, bool isFile)
  {
        QListIterator<QStringList> i(L);
        int group_index=0;

        QList<QStringList> stackedSizeInfo2 ;
        while (i.hasNext())
        {
               QStringListIterator w(i.next());
               QStringList stackedSizeInfo1;
               while (w.hasNext())
               {
                   QString text=w.next();
                   if (isFile & QFileInfo(text).isFile())  // double check on file status. First check is for processing speed, so that QFileInfo is only called when necessary
                   {
                       // computing filesizes
                        stackedSizeInfo1 <<  QString::number((long) QFileInfo(text).size());
                   }
               }

               stackedSizeInfo2 << stackedSizeInfo1;
               group_index++;
        }

        return stackedSizeInfo2;
 }

void dvda::refreshProjectManagerValue(){}
