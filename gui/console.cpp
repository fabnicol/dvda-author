#include "console.h"

Console::Console(MainWindow* p): QDialog(0)
      {
            QGridLayout* consoleLayout=new QGridLayout;
            hide();
            setSizeGripEnabled(true);
            setWindowTitle("Console");
            setMinimumSize(800,600);
            QToolButton *closeConsoleButton=new QToolButton;
            closeConsoleButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
            closeConsoleButton->setToolTip(tr("Close (Ctrl + Q)"));
            closeConsoleButton->setShortcut(QKeySequence("Ctrl+Q"));
            const QIcon clearOutputText = QIcon(QString::fromUtf8( ":/images/edit-clear.png"));
            QToolButton *clearConsoleButton=new QToolButton;
            clearConsoleButton->setIcon(clearOutputText);
            clearConsoleButton->setShortcut(QKeySequence("Ctrl+N"));
            clearConsoleButton->setToolTip(tr("Clear console (Ctrl + N)"));
            consoleLayout->addWidget(textWidget,0,0);
            consoleLayout->addWidget(closeConsoleButton, 1,0,Qt::AlignRight);
            consoleLayout->addWidget(clearConsoleButton, 2,0,Qt::AlignRight);
            setLayout(consoleLayout);
            // [=] not [&]
            connect(closeConsoleButton, &QToolButton::clicked, [=]{on_displayConsoleButton_clicked(p);});
            connect(clearConsoleButton, &QToolButton::clicked, [this]{textWidget->clear();});
     }


void Console::detachConsole(bool isDetached, MainWindow* parent)
{
    if (isDetached)
    {
        hide();
        parent->bottomTabWidget->addTab(parent->consoleDialog, tr("Console"));
        parent->bottomTabWidget->setCurrentIndex(1);
    }
    else
    {
        show();
        parent->bottomTabWidget->removeTab(1);
    }
}

void Console::on_displayConsoleButton_clicked(MainWindow* parent)
    {
        static bool isDetached;
        detachConsole(isDetached, parent);
        isDetached=!isDetached;
    }

