greaterThan(QT_MAJOR_VERSION, 5)

include(spectrum.pri)

# Ensure that library is built before application
CONFIG  += ordered

SUBDIRS += 3rdparty/fftreal \
                       spectrumAnalyzer \
                       Flac

#use at least Qt5.1 with g++-4.8 for windows

TEMPLATE = app

QT       += core gui xml widgets webkitwidgets multimedia multimediawidgets

TARGET = dvda-author-gui

VPATH = .

INCLUDEPATH += spectrumAnalyzer 3rdparty 3rdparty/fftreal

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
    spectrumAnalyzer/engine.cpp \
    spectrumAnalyzer/frequencyspectrum.cpp \
    spectrumAnalyzer/levelmeter.cpp \
    spectrumAnalyzer/mainwidget.cpp \
    spectrumAnalyzer/progressbar.cpp \
    spectrumAnalyzer/settingsdialog.cpp \
    spectrumAnalyzer/spectrograph.cpp \
    spectrumAnalyzer/spectrumanalyser.cpp \
    spectrumAnalyzer/tonegeneratordialog.cpp \
    spectrumAnalyzer/utils.cpp \
    spectrumAnalyzer/waveform.cpp \
    spectrumAnalyzer/wavfile.cpp \
    3rdparty/fftreal/fftreal_wrapper.cpp \
    Flac/flac_metadata_processing.cpp


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
    spectrumAnalyzer/engine.h \
    spectrumAnalyzer/frequencyspectrum.h \
    spectrumAnalyzer/levelmeter.h \
    spectrumAnalyzer/mainwidget.h \
    spectrumAnalyzer/progressbar.h \
    spectrumAnalyzer/settingsdialog.h \
    spectrumAnalyzer/spectrograph.h \
    spectrumAnalyzer/spectrum.h \
    spectrumAnalyzer/spectrumanalyser.h \
    spectrumAnalyzer/tonegeneratordialog.h \
    spectrumAnalyzer/utils.h \
    spectrumAnalyzer/waveform.h \
    spectrumAnalyzer/wavfile.h \
    3rdparty/fftreal/Array.h \
    3rdparty/fftreal/Array.hpp \
    3rdparty/fftreal/def.h \
    3rdparty/fftreal/DynArray.h \
    3rdparty/fftreal/DynArray.hpp \
    3rdparty/fftreal/fftreal_wrapper.h \
    3rdparty/fftreal/FFTReal.h \
    3rdparty/fftreal/FFTReal.hpp \
    3rdparty/fftreal/FFTRealFixLen.h \
    3rdparty/fftreal/FFTRealFixLen.hpp \
    3rdparty/fftreal/FFTRealFixLenParam.h \
    3rdparty/fftreal/FFTRealPassDirect.h \
    3rdparty/fftreal/FFTRealPassDirect.hpp \
    3rdparty/fftreal/FFTRealPassInverse.h \
    3rdparty/fftreal/FFTRealPassInverse.hpp \
    3rdparty/fftreal/FFTRealSelect.h \
    3rdparty/fftreal/FFTRealSelect.hpp \
    3rdparty/fftreal/FFTRealUseTrigo.h \
    3rdparty/fftreal/FFTRealUseTrigo.hpp \
    3rdparty/fftreal/OscSinCos.h \
    3rdparty/fftreal/OscSinCos.hpp \
    Flac/flac_metadata_processing.h \
    Flac/format.h \
    Flac/stream_decoder.h \
    Flac/export.h





RESOURCES += \
    ../share/dvda-author-gui-12.12/dvda-author-gui.qrc \
    spectrumAnalyzer/spectrum.qrc

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
    spectrumAnalyzer/images/record.png \
    spectrumAnalyzer/images/settings.png \
    3rdparty/fftreal/bwins/fftrealu.def \
    3rdparty/fftreal/FFTReal.dsp \
    3rdparty/fftreal/FFTReal.dsw \
    3rdparty/fftreal/license.txt \
    3rdparty/fftreal/readme.txt \
    3rdparty/fftreal/testapp.dpr \
    3rdparty/fftreal/fftreal.pas







