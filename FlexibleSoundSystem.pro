#-------------------------------------------------
#
# Project created by QtCreator 2017-01-17T01:16:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FlexibleSoundSystem
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp \
    gui/src/flexiblesoundsystem.cpp \
    gui/src/cmusictree.cpp \
    src/decoder.cpp \
    src/encoder.cpp \
    src/gui.cpp \
    src/midicontroller.cpp \
    src/mixer.cpp \
    src/speaker.cpp \
    third_party/src/RtMidi.cpp \
    gui/src/cimageviewer.cpp \
    src/streamer.cpp \
    src/knobs.cpp

HEADERS  += gui/inc/flexiblesoundsystem.h \
    gui/inc/cmusictree.h \
    inc/decoder.h \
    inc/encoder.h \
    inc/gui.h \
    inc/midicontroller.h \
    inc/mixer.h \
    inc/speaker.h \
    third_party/inc/RtMidi.h \
    inc/midi_cmd.h \
    gui/inc/cimageviewer.h \
    inc/streamer.h \
    inc/knobs.h

DEFINES += __LINUX_ALSA__

FORMS    += ui/flexiblesoundsystem.ui

INCLUDEPATH += inc/
INCLUDEPATH += third_party/inc/
INCLUDEPATH += gui/inc/

LIBS += -lrt -lasound -lavcodec -lavformat -lavutil -ljrtp -ljthread

INSTALLS = target
target.files = FlexibleSoundSystem
target.path = /home/hugoa
