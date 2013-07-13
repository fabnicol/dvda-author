#ifndef BROWSER_H
#define BROWSER_H

#include <QtWidgets>
#include <QtWebKitWidgets>
#include <QWebView>
#include "common.h"

class QToolButton;
class QWebView;


class browser : public QWidget
{
  Q_OBJECT
public:
   browser(const QUrl & url,  QWidget *parent = 0);
   static void showPage(const QUrl &url);

 private:
   QWebView *textBrowser  = new QWebView;
   QToolButton *homeButton = new QToolButton ;
   QToolButton *backButton = new QToolButton ;
   QToolButton *forwardButton = new QToolButton ;
   QToolButton *closeButton =new QToolButton;
   QUrl url;

public slots:
   void updateWindowTitle();
   void home();

};

#endif // BROWSER_H
