#-------------------------------------------------
#
# Project created by QtCreator 2014-07-06T12:08:44
#
#-------------------------------------------------

VERSION   = 2.0.4
QT       += core gui network svg gui-private
CODECFORSRC = UTF-8
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = upv
TEMPLATE = app

CONFIG += c++11 link_pkgconfig
CONFIG += install_translations install_scripts

DESTDIR = build
OBJECTS_DIR = $${DESTDIR}/obj
MOC_DIR = $${DESTDIR}/moc
RCC_DIR = $${DESTDIR}/rcc
UI_DIR = $${DESTDIR}/ui

PKGCONFIG += mpv python3


macx {
    PKG_CONFIG = PKG_CONFIG_PATH=/Users/liberize/Code/GitHub/mpv/inst/lib/pkgconfig:/usr/local/lib/pkgconfig /usr/local/bin/pkg-config

    APP_DATA_DIR = ../Resources
    ICON = img/logo.icns

    CONFIG += objective_c
    OBJECTIVE_SOURCES += platform/osx.mm widgets/mpvcocoawidget.mm
    QMAKE_OBJECTIVE_CFLAGS += -fobjc-arc
    LIBS += -framework AppKit -framework Foundation -framework OpenGL -framework QuartzCore
    DEFINES += "ENABLE_MPV_COCOA_WIDGET"
}

unix:!macx {
    QT += x11extras
    PKGCONFIG += x11

    APP_DATA_DIR = /usr/share/upv

    SOURCES += platform/linux.cpp

    # INSTROOT is the installation root directory, leave empty if not using a package management system
    isEmpty(BINDIR):BINDIR=/usr/bin
    isEmpty(MEDIADIR):MEDIADIR=/usr/share/pixmaps
    isEmpty(ICONDIR):ICONDIR=/usr/share/icons/hicolor/scalable/apps
    isEmpty(APPDIR):APPDIR=/usr/share/applications
    isEmpty(DOCDIR):DOCDIR=/usr/share/doc
    isEmpty(MANDIR):MANDIR=/usr/share/man
    isEmpty(LICENSEDIR):LICENSEDIR=/usr/share/licenses

    target.path = $$INSTROOT$$BINDIR
    logo.path = $$INSTROOT$$MEDIADIR
    icon.path = $$INSTROOT$$ICONDIR
    desktop.path = $$INSTROOT$$APPDIR
    manual.path = $$INSTROOT$$DOCDIR/upv
    man.path = $$INSTROOT$$MANDIR/man1
    license.path = $$INSTROOT$$LICENSEDIR/upv

    logo.files = ../etc/logo/upv.svg
    icon.files = ../etc/logo/upv.svg
    desktop.files = ../etc/upv.desktop
    manual.files = ../DOCS/upv.md
    man.files = ../DOCS/upv.1.gz
    license.files = ../LICENSE

    INSTALLS += target icon logo desktop manual man license
}

win32 {
    QT += winextras
    PKGCONFIG += libzip

    APP_DATA_DIR = .

    # mxe fix:
    CONFIG -= windows
    QMAKE_LFLAGS += $$QMAKE_LFLAGS_WINDOWS -pthread

    # xp (minimum version)
    DEFINES += "WINVER=0x0501" \
               "_WIN32_WINNT=0x0501"

    # application information
    RC_ICONS += img/logo.ico
    QMAKE_TARGET_PRODUCT += Baka MPlayer
    QMAKE_TARGET_DESCRIPTION += Baka MPlayer
    #RC_LANG +=

    SOURCES += platform/win.cpp
    RESOURCES += win_rsclist.qrc

    # 32 bit
    contains(QMAKE_HOST.arch, x86): SOURCES += platform/win32.cpp
    # 64 bit
    contains(QMAKE_HOST.arch, x86_64): SOURCES += platform/win64.cpp
}


RESOURCES += rsclist.qrc

isEmpty(TRANSLATIONS) {
    include(translations.pri)
}

TRANSLATIONS_COMPILED = $$TRANSLATIONS
TRANSLATIONS_COMPILED ~= s/\.ts/.qm/g

CONFIG(embed_translations) {
    # create translations resource file
    system("echo \'<RCC><qresource prefix=\"/\">\' > translations.qrc")
    for (translation, TRANSLATIONS_COMPILED):system("echo \'<file>$$translation</file>\' >> translations.qrc")
    system("echo \'</qresource></RCC>\'" >> translations.qrc)

    # add file to build
    RESOURCES += translations.qrc

    # make sure translations are updated and released
    CONFIG *= update_translations release_translations
}

CONFIG(install_translations) {
    # install translation files
    translations.files = $$TRANSLATIONS_COMPILED

    macx {
        translations.path = Contents/Resources/translations
        QMAKE_BUNDLE_DATA += translations
    }
    unix:!macx {
        translations.path = $$INSTROOT$$APP_DATA_DIR/translations
        INSTALLS += translations
    }
    win32 {
        translations.path = $$DESTDIR/translations
        INSTALLS += translations
    }

    # make sure translations are updated and released
    CONFIG *= update_translations release_translations
}

CONFIG(begin_translations) {
    isEmpty(lupdate):lupdate=lupdate
    system($$lupdate -locations absolute $$_PRO_FILE_)
}

CONFIG(update_translations) {
    isEmpty(lupdate):lupdate=lupdate
    system($$lupdate -no-obsolete -locations none $$_PRO_FILE_)
}

CONFIG(release_translations) {
    isEmpty(lrelease):lrelease=lrelease
    system($$lrelease $$_PRO_FILE_)
}


CONFIG(install_scripts) {
    scripts.files += scripts

    macx {
        scripts.path = Contents/Resources
        QMAKE_BUNDLE_DATA += scripts
    }
    unix:!macx {
        scripts.path = $$INSTROOT$$APP_DATA_DIR
        INSTALLS += scripts
    }
    win32 {
        scripts.path = $$DESTDIR
        INSTALLS += scripts
    }
}

APP_NAME = upv

DEFINES += "APP_VERSION=\\\"$$VERSION\\\"" \
           "APP_NAME=\\\"$$APP_NAME\\\"" \
           "APP_DATA_DIR=\\\"$$APP_DATA_DIR\\\""

!isEmpty(APP_LANG):DEFINES += "APP_LANG=\\\"$$APP_LANG\\\""


SOURCES += \
    bakacommands.cpp \
    bakaengine.cpp \
    gesturehandler.cpp \
    main.cpp\
    mpvhandler.cpp \
    overlay.cpp \
    overlayhandler.cpp \
    pluginmanager.cpp \
    settings.cpp \
    ui/aboutdialog.cpp \
    ui/inputdialog.cpp \
    ui/jumpdialog.cpp \
    ui/keydialog.cpp \
    ui/locationdialog.cpp \
    ui/mainwindow.cpp \
    ui/preferencesdialog.cpp \
    ui/screenshotdialog.cpp \
    ui/updatedialog.cpp \
    updatemanager.cpp \
    util.cpp \
    versions/2_0_3.cpp \
    widgets/customlabel.cpp \
    widgets/customlineedit.cpp \
    widgets/custompushbutton.cpp \
    widgets/customslider.cpp \
    widgets/customsplitter.cpp \
    widgets/dimdialog.cpp \
    widgets/mpvglwidget.cpp \
    widgets/onlinewidget.cpp \
    widgets/playlistwidget.cpp \
    widgets/seekbar.cpp \
    delegates/pluginitemdelegate.cpp \
    ui/pluginconfigdialog.cpp \
    requestmanager.cpp \
    request.cpp \
    plugin.cpp \
    subtitleprovider.cpp \
    mediaprovider.cpp \
    worker.cpp


HEADERS  += \
    bakaengine.h \
    gesturehandler.h \
    mpvhandler.h \
    mpvtypes.h \
    overlay.h \
    overlayhandler.h \
    pluginmanager.h \
    plugintypes.h \
    pybind11/attr.h \
    pybind11/buffer_info.h \
    pybind11/cast.h \
    pybind11/chrono.h \
    pybind11/common.h \
    pybind11/complex.h \
    pybind11/detail/class.h \
    pybind11/detail/common.h \
    pybind11/detail/descr.h \
    pybind11/detail/init.h \
    pybind11/detail/internals.h \
    pybind11/detail/typeid.h \
    pybind11/eigen.h \
    pybind11/embed.h \
    pybind11/eval.h \
    pybind11/functional.h \
    pybind11/iostream.h \
    pybind11/numpy.h \
    pybind11/operators.h \
    pybind11/options.h \
    pybind11/pybind11.h \
    pybind11/pytypes.h \
    pybind11/stl.h \
    pybind11/stl_bind.h \
    recent.h \
    settings.h \
    ui/aboutdialog.h \
    ui/inputdialog.h \
    ui/jumpdialog.h \
    ui/keydialog.h \
    ui/locationdialog.h \
    ui/mainwindow.h \
    ui/preferencesdialog.h \
    ui/screenshotdialog.h \
    ui/updatedialog.h \
    updatemanager.h \
    util.h \
    widgets/customlabel.h \
    widgets/customlineedit.h \
    widgets/custompushbutton.h \
    widgets/customslider.h \
    widgets/customsplitter.h \
    widgets/dimdialog.h \
    widgets/mpvcocoawidget.h \
    widgets/mpvglwidget.h \
    widgets/mpvwidget.h \
    widgets/onlinewidget.h \
    widgets/playlistwidget.h \
    widgets/seekbar.h \
    delegates/pluginitemdelegate.h \
    ui/pluginconfigdialog.h \
    requestmanager.h \
    request.h \
    pycast.h \
    plugin.h \
    subtitleprovider.h \
    mediaprovider.h \
    worker.h


FORMS    += \
    ui/aboutdialog.ui \
    ui/inputdialog.ui \
    ui/jumpdialog.ui \
    ui/locationdialog.ui \
    ui/mainwindow.ui \
    ui/preferencesdialog.ui \
    ui/screenshotdialog.ui \
    ui/updatedialog.ui \
    ui/keydialog.ui \
    ui/pluginconfigdialog.ui
