

greaterThan(QT_MAJOR_VERSION, 5)
#use at least Qt5.1 with g++-4.8 for windows

TEMPLATE = app

QT       += core gui xml widgets webkitwidgets multimedia multimediawidgets

TARGET = dvda-author-gui

VPATH = .

DEFINES += CDRECORD_LOCAL_PATH

QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS = -Wl,--no-keep-memory

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
    videoplayer.cpp \
    xmlparser.cpp \
    highlighter.cpp \
    run.cpp \
    console.cpp


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
    videoplayer.h \
    dvda.h \
    highlighter.h \
    console.h


RESOURCES += \
    ../share/dvda-author-gui-12.12/dvda-author-gui.qrc







