#ifndef CONSOLE_H
#define CONSOLE_H
#include <QtWidgets>
#include <QObject>

#include "dvda-author-gui.h"

class MainWindow;

class Console : public QDialog
{

  public:
    Console(MainWindow* ) ;
    void detachConsole(bool, MainWindow*);
    void on_displayConsoleButton_clicked(MainWindow*);
    void appendHtml(const QString &s) const {textWidget->insertHtml(s); textWidget->moveCursor(QTextCursor::End);}

  private:
    QTextEdit* textWidget=new QTextEdit;

};

#endif // CONSOLE_H
