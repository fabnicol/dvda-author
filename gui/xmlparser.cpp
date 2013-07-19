#include "dvda.h"
#include "common.h"



inline const QString dvda::makeParserString(int start, int end)
{

    QStringList L=QStringList();

    for (int j=start; j <=end; j++)
      {

        FAbstractWidget* widget=Abstract::abstractWidgetList.at(j);
        QString hK=widget->getHashKey();

        if  (widget->getHashKey().isEmpty())
          {
            QMessageBox::warning(this, tr("Error"), tr(".dvp project parsing error"));
            continue;
          }

        QString xml=widget->setXmlFromWidget().toQString();
        QString widgetDepth=widget->getDepth();

        L <<  "  <" + hK + " widgetDepth=\"" + widgetDepth +  "\">\n   "
                                 + xml
              +"\n  </" + hK + ">\n";

      }

    return L.join("");

}


inline const QString  dvda::makeDataString()
{
    return  makeParserString(0,1);
}

inline const QString  dvda::makeSystemString()
{
    return makeParserString(2);
}


void dvda::writeProjectFile()
{
  QFile projectFile;
  checkEmptyProjectName();
  projectFile.setFileName(projectName);
  QErrorMessage *errorMessageDialog = new QErrorMessage(this);
  if (!projectFile.open(QIODevice::WriteOnly))
    {
          errorMessageDialog->showMessage(tr("Cannot open file for writing\n")+ qPrintable(projectFile.errorString()));
          QLabel *errorLabel = new QLabel;
          errorLabel->setText(tr("If the box is unchecked, the message "
                                 "won't appear again."));
          return;
     }

  QTextStream out(&projectFile);
  out.setCodec("UTF-8");

  out << "<?xml version=\"1.0\"?>\n" <<"<project>\n";
  out << " <data>\n";

  out << dvda::makeDataString();

  out << " </data>\n";
  out << " <system>\n";

  out << dvda::makeSystemString();

  out << " </system>\n <recent>\n";

  QStringListIterator w(parent->recentFiles);
  QString str;
  while (w.hasNext() && QFileInfo(str=w.next()).isFile())
     out    <<  "  <file>" << str << "</file>\n";

  out << " </recent>\n</project>\n";
  out.flush();
  options::RefreshFlag=hasSavedOptions;
}

namespace XmlMethod
{

   QTreeWidgetItem *itemParent=NULL;

  inline void stackData(const QDomNode & node, QStringList tags, int level, QVariant &textData)
    {
      QDomNode  childNode=node.firstChild();
      QList<QVariant> stackedInfo;
      QStringList strL;
      QString str;

      switch(level)
      {
      /* parses < tag> text </tag> */

           case 0:

                tags[0] = node.toElement().tagName();
                str.clear();
                while ((!childNode.isNull()) && (childNode.nodeType() == QDomNode::TextNode))
                  {
                     str += childNode.toText().data().simplified();
                     childNode=childNode.nextSibling();
                     //Q(str)
                  }

                textData=QVariant(str);
                break;

        /*
         * parses < tags[0]>
                             <tags[1]>  text </tags[1]>
                             ....
                             <tags[1]> text </tags[1]>
                        </tags[0]>
             note: does not check subordinate tag uniformity
        */

          case 1:

                tags[0]=node.toElement().tagName();

                while (!childNode.isNull())
                  {
                    QVariant str;
                    stackData(childNode, QStringList(tags[1]), 0, str);
                    strL << str.toString();
                    childNode=childNode.nextSibling();
                  }
                textData=QVariant(strL);
                break;

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

    case 2:
            tags[0]=node.toElement().tagName();
            childNode=node.firstChild();

           while (!childNode.isNull())
           {
             QStringList L={QString(), QString()};
             QVariant M;
             stackData(childNode, L,1, M);
             stackedInfo << M.toStringList();
             tags[1]=L.at(0);
             tags[2]=L.at(1);
             childNode=childNode.nextSibling();
           }

        textData= QVariant(stackedInfo);
        break;
    }
  }

/* computes sizes and sends filenames to main tab Widget */


/* displays on manager tree window */

void displayTextData(const QStringList &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                     const QColor &color=QColor("blue"));

void displayTextData(const QStringList &firstColumn,
                                 const QString &secondColumn,
                                 const QString &thirdColumn,
                                 const QColor &color)
{
          static QString last;
          static QTreeWidgetItem* item;
          int shift=0;
          if (firstColumn.at(0) != last)
          {
            item = new QTreeWidgetItem(XmlMethod::itemParent);
             item->setText(0, firstColumn.at(0));
             item->setExpanded(false);
             shift=1;
          }


           if (firstColumn.count() > 1)
           {
               QTreeWidgetItem* item2 = new QTreeWidgetItem(item);
               item2->setText(0, firstColumn.at(1));
               item2->setText(1, secondColumn);
           }
           else
           {
              item->setText(1, secondColumn);
              if (!thirdColumn.isEmpty()) item->setText(2, thirdColumn);
              if (color.isValid()) item->setTextColor(2, color);
           }
           last= firstColumn.at(0);
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
      QString  firstColumn, root=tags.at(0), secondColumn=tags.at(1), thirdColumn;

      QListIterator<QStringList> i(stackedInfo), j(stackedSizeInfo);

      while ((i.hasNext()) && (j.hasNext()))
       {
          if (!root.isEmpty())
           {
               firstColumn = root + " "+QString::number(++k);
           }

          displayTextData({firstColumn}, "", "");

           QStringListIterator w(i.next()), z(j.next());
           l=0;
           while ((w.hasNext()) && (z.hasNext()))
           {
              ++count;
               if (!tags.at(1).isEmpty())
                   secondColumn =  tags.at(1) +" " +QString::number(++l) + "/"+ QString::number(count) +": ";
               secondColumn += w.next()  ;

               if ((stackedSizeInfo.size() > 0) && (z.hasNext()))
               {
                   qint64 msize=z.next().toLongLong();
                   filesizecount += msize;
                   // force coertion into float or double using .0
                   thirdColumn    = QString::number(msize/1048576.0, 'f', 1) + "/"+  QString::number(filesizecount/1048576.0, 'f', 1)+ " MB" ;
               }

               displayTextData({""}, secondColumn, thirdColumn, (z.hasNext())? QColor("navy"): ((j.hasNext())? QColor("orange") :QColor("red")));

           }
       }
      return filesizecount;
  }


/* tags[0]
 *                       tags[1] 1 : xxx  ...  (size MB)
 *                       tags[1] 2 : xxx  ...  (size MB) ... */

inline void displayFirstLevelData( const QString &tag,  const QString &style, const QStringList &stackedInfo)
    {
       QStringListIterator i(stackedInfo);
       int count=0;
       while (i.hasNext())
          {
             ++count;
             displayTextData((count>1)?QStringList(""):QStringList(tag), style+" "+QString::number(count)+": "+i.next(), "");
           }
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

  dvda::totalSize[AUDIO]=dvda::totalSize[VIDEO]=0;

  for (QString maintag : {"data", "system", "recent"})
  {
       if (node.toElement().tagName() != maintag) return;

       QDomNode subnode=node.firstChild();

          while (!subnode.isNull())
            {
              const FStringList &str=parseEntry(subnode);
              if (!str.at(0).at(0).isEmpty())
                 *(Hash::wrapper[subnode.toElement().tagName()]=new FStringList) =   str;
                subnode=subnode.nextSibling();
            }

      node = node.nextSibling();
  }

  refreshProjectManagerValues();

  /* this assigns values to widgets (line edits, checkboxes, list widgets etc.)
   * in the Options dialog and ensures fills in main tab widget */

  //if ((dvda::RefreshFlag&UpdateTabMask) == (UpdateMainTabs|UpdateOptionTabs))
  //{

      assignVariables();

      // adds extra information to main window and sets alternating row colors

      for (int ZONE : {AUDIO, VIDEO})
      {
          for (int group_index=0; group_index<= project[ZONE]->getRank(); group_index++)
          {
              int r=0;
              for (QString text : Hash::wrapper[dvda::zoneTag(ZONE)]->at(group_index))
              {
                  if (!text.isEmpty())
                         assignGroupFiles(ZONE, group_index, QDir::toNativeSeparators(text));
                  r++;
              }

              refreshRowPresentation(ZONE, group_index);
          }
      }
  //}

  /* resets recent files using the ones listed in the dvp project file */

  parent->updateRecentFileActions();

  /* used to connect to slides, soundtracks and other option list widgets in Options dialog :
   * these will be activated depending on main tab widget information */

   emit(is_signalList_changed(project[AUDIO]->signalList->size()));
}



FStringList dvda::parseEntry(const QDomNode &node, QTreeWidgetItem *itemParent)
{

  QVariant textData;
  QStringList tags={QString(),QString(),QString()} ;
  int level=node.toElement().attribute("widgetDepth").toInt();

   XmlMethod::itemParent = itemParent;
   XmlMethod::stackData(node, tags, level, textData);

   if ((level == 0) &&(tags[0] == "file"))
                      parent->recentFiles.append(textData.toString());

   switch (level)
   {
   case 0:  return FStringList(textData.toString());
   case 1:  return FStringList(textData.toStringList());
   case 2:  return FStringList(textData.toList());

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



void dvda::refreshProjectManagerValues(int refreshProjectManagerFlag)
{
     if ((refreshProjectManagerFlag & refreshProjectInteractiveMask) == refreshProjectInteractiveMode)
    {
         updateIndexInfo();
         fileSizeDataBase[isVideo] = processSecondLevelData(*Hash::wrapper[dvda::zoneTag(isVideo)]);
    }

    QTreeWidgetItem *item=new QTreeWidgetItem(managerWidget);
    item->setText(0, "data");
    item->setExpanded(true);
    XmlMethod::itemParent=item;

    bool test[2]={(refreshProjectManagerFlag & refreshProjectAudioZoneMask) == refreshAudioZone,
                             (refreshProjectManagerFlag & refreshProjectVideoZoneMask) == refreshVideoZone};

    for (int ZONE: {AUDIO, VIDEO})
             if (test[ZONE])
                      dvda::totalSize[ZONE]=XmlMethod::displaySecondLevelData(
                                                                {dvda::zoneGroupLabel(ZONE), "file"},
                                                                   *Hash::wrapper[dvda::zoneTag(ZONE)],
                              fileSizeDataBase[ZONE]=processSecondLevelData(*Hash::wrapper[dvda::zoneTag(ZONE)]));

       item=new QTreeWidgetItem(managerWidget);
       item->setText(0, "system");
       item->setExpanded(true);
       XmlMethod::itemParent=item;

       if ((refreshProjectManagerFlag & refreshProjectSystemZoneMask) == refreshSystemZone)
      {

           for (int k=2; k <Abstract::abstractWidgetList.count(); k++)
           {

               QString key=Abstract::abstractWidgetList[k]->getHashKey();

               if (Abstract::abstractWidgetList[k]->getDepth() == "0")
               {
                   XmlMethod::displayTextData(Hash::description[key], Hash::wrapper[key]->toQString(), "");
               }
               else if (Abstract::abstractWidgetList[k]->getDepth() == "1")
                   XmlMethod::displayFirstLevelData(Hash::description[key].at(0),   "button", Hash::wrapper[key]->at(0));
           }
       }

       options::RefreshFlag|=hasSavedOptions;

}
