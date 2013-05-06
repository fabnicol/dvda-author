

greaterThan(QT_MAJOR_VERSION, 4)

TEMPLATE = app

QT       += core gui xml widgets webkitwidgets multimedia multimediawidgets

QMAKE_CXXFLAGS +=  -std=c++11

TARGET = dvda-author-gui

VPATH = .

SOURCES += \
    options.cpp \
    mainwindow.cpp \
    lplex.cpp \
    fwidgets.cpp \
    fstring.cpp \
    flistframe.cpp \
    dvda.cpp \
    common.cpp \
    forms.cpp \
    main.cpp \
    viewer.cpp \
    browser.cpp \ 
    fcolor.cpp \
    videoplayer.cpp


HEADERS  += \
    options.h \
    fwidgets.h \
    fstring.h \
    flistframe.h \
    common.h \
    dvda-author-gui.h \
    lplex.h \
    enums.h \
    forms.h \
    viewer.h \
    browser.h \
    fcolor.h \
    videoplayer.h


RESOURCES += \
    ../share/dvda-author-gui-12.12/dvda-author-gui.qrc






