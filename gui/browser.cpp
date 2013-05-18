
#include "browser.h"

browser::browser(const QUrl &urlPath,  QWidget *parent) :  QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);

    url=urlPath;

    textBrowser = new QWebView;

    homeButton = new QToolButton;
    backButton = new QToolButton;
    forwardButton = new QToolButton;
    closeButton  =new QToolButton;

    homeButton->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    homeButton->setToolTip(tr("Home"));
    backButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    backButton->setToolTip(tr("Back (Ctrl + <)"));
    backButton->setShortcut(QKeySequence("Ctrl+<"));

    forwardButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    forwardButton->setToolTip(tr("Forward (Ctrl + >)"));
    forwardButton->setShortcut(QKeySequence("Ctrl+>"));

    closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    closeButton->setToolTip(tr("Close (Ctrl + Q)"));
    closeButton->setShortcut(QKeySequence("Ctrl+Q"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(homeButton);
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(forwardButton);
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();


    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(textBrowser);
    setLayout(mainLayout);

    connect(homeButton, SIGNAL(clicked()), this, SLOT(home()));
    connect(backButton, SIGNAL(clicked()), textBrowser, SLOT(back()));
    connect(forwardButton, SIGNAL(clicked()), textBrowser, SLOT(forward()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(textBrowser, SIGNAL(urlChanged(const QUrl &)), this, SLOT(updateWindowTitle()));

    textBrowser->load(url);
    const QIcon dvdaIcon=QIcon(QString::fromUtf8( ":/images/dvda-author.png"));
    setWindowIcon(dvdaIcon);

}

void browser::home()
{
    textBrowser->load(url);
}

void browser::updateWindowTitle()
{
    setWindowTitle(textBrowser->url().toString());
}

void browser::showPage(const QUrl &url)
{
    browser *app = new browser(url);
    app->resize(1000, 500);
    app->show();
}



