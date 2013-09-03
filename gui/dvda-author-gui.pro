greaterThan(QT_MAJOR_VERSION, 5)

# Ensure that library is built before application
CONFIG  += ordered

SUBDIRS += 3rdparty 3rdparty/FLAC 3rdparty/sox 3rdparty/sonic-visualiser

#use at least Qt5.1 with g++-4.8 for windows

TEMPLATE = app

QT       += core gui xml widgets webkitwidgets multimedia multimediawidgets

TARGET = dvda-author-gui

VPATH = .

INCLUDEPATH += 3rdparty 3rdparty/sox 3rdparty/FLAC 3rdparty/sonic-visaliser 3rdparty/Qt
LIBS += -L../libs -lFLAC -lsox

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
    console.cpp \
    3rdparty/FLAC/flac_metadata_processing.cpp \
    3rdparty/Qt/wavfile.cpp \
    probe.cpp




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
    console.h \
    3rdparty/FLAC/format.h \
    3rdparty/FLAC/stream_decoder.h \
    3rdparty/FLAC/export.h \
    3rdparty/FLAC/ordinals.h \
    3rdparty/sox/sox.h \
    3rdparty/FLAC/flac_metadata_processing.h \
    3rdparty/Qt/wavfile.h \
    probe.h







RESOURCES += \
    ../share/dvda-author-gui-12.12/dvda-author-gui.qrc 

OTHER_FILES += \
    android/src/org/qtproject/qt5/android/bindings/QtApplication.java \
    android/src/org/qtproject/qt5/android/bindings/QtActivity.java \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/AndroidManifest.xml \
    android/version.xml \
    android/res/values-ru/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/res/values/strings.xml \
    android/res/values/libs.xml \
    android/res/values-fr/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/layout/splash.xml \
    android/res/values-de/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-id/strings.xml \
     3rdparty/sonic-visualiser/svapp/COPYING \
    3rdparty/sonic-visualiser/svapp/INSTALL.txt \
    3rdparty/sonic-visualiser/COPYING \
    3rdparty/sonic-visualiser/INSTALL.txt \
    3rdparty/sonic-visualiser/README \
    3rdparty/sonic-visualiser/README.OSC \
    3rdparty/sonic-visualiser/fork.patch \
    3rdparty/sonic-visualiser/svcore/COPYING \
    3rdparty/sonic-visualiser/svcore/INSTALL.txt







