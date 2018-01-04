#-------------------------------------------------
#
# Project created by QtCreator 2015-01-28T14:03:53
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StreamPlay
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    bonjourservicebrowser.cpp \
    bonjourserviceresolver.cpp \
    torrentstream.cpp \
    airplay.cpp \
    hlsserver.cpp \
    hlstranscode.cpp \
    hlsvideo.cpp \
    moovfaststart.cpp \
    preferences.cpp \
    autoupdater.cpp \
    preferencesdialog.cpp \
    application.cpp \
    aboutdialog.cpp

HEADERS  += mainwindow.h \
    bonjourrecord.h \
    bonjourservicebrowser.h \
    bonjourserviceresolver.h \
    torrentstream.h \
    airplay.h \
    hlsserver.h \
    hlstranscode.h \
    hlsvideo.h \
    moovfaststart.h \
    preferences.h \    
    autoupdater.h \
    preferencesdialog.h \
    config.h \
    application.h \
    aboutdialog.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui \
    aboutdialog.ui

DEFINES += WITH_SHIPPED_GEOIP_H
DEFINES += BOOST_ASIO_DYN_LINK
DEFINES += WITH_GEOIP_EMBEDDED

RESOURCES += \
    StreamPlay.qrc

DISTFILES += \
    License.icns

win32 {
    LIBS+=-ldnssd

    HEADERS +=

    SOURCES +=
}

mac {
    LIBS += -stdlib=libc++
    QMAKE_CXXFLAGS += -stdlib=libc++

    LIBS += -L/usr/local/lib -ltorrent-rasterbar
    LIBS += -L/usr/local/lib -lboost_system-mt

    INCLUDEPATH += /usr/local/include

    HEADERS += sparkleautoupdater.h cocoaInitializer.h

    OBJECTIVE_SOURCES += sparkleautoupdater.mm cocoaInitializer.mm

    QMAKE_LFLAGS += -F /Library/Frameworks

    LIBS += -framework Sparkle -framework AppKit

    APP_ICONS.files = StreamPlay.icns License.icns
    APP_ICONS.path = Contents/Resources
    QMAKE_BUNDLE_DATA += APP_ICONS

    QMAKE_INFO_PLIST = Info.plist
    QMAKE_POST_LINK = mkdir -p StreamPlay.app/Contents/Frameworks && cp -a /Library/Frameworks/Sparkle.framework StreamPlay.app/Contents/Frameworks
}
