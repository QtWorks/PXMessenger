TEMPLATE = app
TARGET = PXMessenger
VERSION = 1.5.0
QMAKE_TARGET_COMPANY = Bolar Code Solutions
QMAKE_TARGET_PRODUCT = PXMessenger
QMAKE_TARGET_DESCRIPTION = Instant Messenger
QMAKE_TARGET_COPYRIGHT = GPLv3

target.path 	= /usr/local/bin
desktop.path 	= /usr/share/applications
desktop.files 	+= $$PWD/resources/pxmessenger.desktop
icon.path 	= /usr/share/pixmaps
icon.files 	+= $$PWD/resources/PXMessenger.png
INSTALLS += target desktop icon

QT = core gui widgets multimedia
CONFIG += DEBUG \
          RELEASE
win32: CONFIG += windows

unix: LIBS += -levent -levent_pthreads

win32 {
LIBS += -L$$PWD/../libevent/build/lib -levent -levent_core
INCLUDEPATH += $$PWD/../libevent/include \
               $$PWD/../libevent/build/include
}

INCLUDEPATH += $$PWD/include

win32 {
LIBS += -lws2_32
RC_ICONS = $$PWD/resources/PXM_Icon.ico
}

QMAKE_CXXFLAGS += -Wall \
                -std=c++14

SOURCES += \
    $$PWD/src/pxmclient.cpp \
    $$PWD/src/pxmpeerworker.cpp \
    $$PWD/src/pxmsync.cpp \
    $$PWD/src/pxminireader.cpp \
    $$PWD/src/pxmserver.cpp \
    $$PWD/src/pxmmainwindow.cpp \
    $$PWD/src/pxmessenger.cpp \
    $$PWD/src/netcompression.cpp \
    $$PWD/src/pxmstackwidget.cpp \
    $$PWD/src/pxmconsole.cpp \
    $$PWD/src/pxmpeers.cpp \
    $$PWD/src/pxmagent.cpp

HEADERS += \
    $$PWD/include/pxmpeerworker.h \
    $$PWD/include/pxmmainwindow.h \
    $$PWD/include/pxmsync.h \
    $$PWD/include/pxminireader.h \
    $$PWD/include/pxmserver.h \
    $$PWD/include/pxmclient.h \
    $$PWD/include/netcompression.h \
    $$PWD/include/timedvector.h \
    $$PWD/include/pxmstackwidget.h \
    $$PWD/include/pxmconsole.h \
    $$PWD/include/pxmconsts.h \
    $$PWD/include/pxmpeers.h \
    $$PWD/include/pxmagent.h

FORMS += \
    $$PWD/ui/pxmmainwindow.ui \
    $$PWD/ui/pxmaboutdialog.ui \
    $$PWD/ui/pxmsettingsdialog.ui

RESOURCES += 	$$PWD/resources/resources.qrc

win32 {
Release:DESTDIR = $$PWD/
Debug:DESTDIR = $$PWD/
OBJECTS_DIR = $$PWD/build-win32/obj
MOC_DIR = $$PWD/build-win32/moc
RCC_DIR = $$PWD/build-win32/rcc
UI_DIR = $$PWD/build-win32/ui
}
unix {
release:DESTDIR = $$PWD/
debug:DESTDIR = $$PWD/
OBJECTS_DIR = $$PWD/build-unix/obj
MOC_DIR = $$PWD/build-unix/moc
RCC_DIR = $$PWD/build-unix/rcc
UI_DIR = $$PWD/build-unix/ui
}

win32 {
include($$PWD/QSimpleUpdater/QSimpleUpdater.pri)
}
