TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../redist ..
QMAKE_CXXFLAGS += -std=c++1z
QMAKE_LFLAGS += -L../redist/md5
LIBS += -lFLAC++ -lFLAC -ldvdread -lm -lstdc++fs -lmd5
SOURCES += \
    ../src/dvd.cpp \
    ../src/exec.cpp \
    ../src/flac.cpp \
    ../src/layout.cpp \
    ../src/lgzip.cpp \
    ../src/lpcm.cpp \
    ../src/lplex.cpp \
    ../src/main.cpp \
    ../src/reader.cpp \
    ../src/util.cpp \
    ../src/video.cpp \
    ../src/writer.cpp \
    ../src/wx.cpp

HEADERS += \
    ../src/color.h \
    ../src/dvd.hpp \
    ../src/flac.hpp \
    ../src/lpcm.hpp \
    ../src/lplex.hpp \
    ../src/platform.h \
    ../src/processor.hpp \
    ../src/util.h \
    ../src/wx.hpp \
    ../redist/lplex_precompile.h \
    ../src/jobs.hpp \
    ../redist/lplex_precompile.h \
    ../redist/vlc_bits.h

DISTFILES += \
    ../Makefile \
    ../lplex.def
