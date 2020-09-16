QT += gui KWindowSystem

TARGET = libinput-touch-translator

CONFIG += c++11 console link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += libinput libudev

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(touch-screen/touch-screen.pri)
include(touchpad/touchpad.pri)

SOURCES += \
        event-monitor.cpp \
        main.cpp \
        settings-manager.cpp \
        uinput-helper.cpp

target.path = /usr/libexec
!isEmpty(target.path): INSTALLS += target

service.files = systemd/libinput-touch-translator.service
service.path = /lib/systemd/system
INSTALLS += service

HEADERS += \
    event-monitor.h \
    settings-manager.h \
    uinput-helper.h
