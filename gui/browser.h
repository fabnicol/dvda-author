#ifndef BROWSER_H
#define BROWSER_H

#include <QWidget>
#include <QtGui>
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
   QWebView *textBrowser;
   QToolButton *homeButton;
   QToolButton *backButton;
   QToolButton *forwardButton;
   QToolButton *closeButton;
   QUrl url;

public slots:
   void updateWindowTitle();
   void home();

};

#endif // BROWSER_H
