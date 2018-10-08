#!/bin/sh

MXE_PATH='/Users/liberize/Code/GitHub/mxe'
TARGET=i686-w64-mingw32.static
JOBS=6

export PATH="$MXE_PATH/usr/bin:$PATH"

# edit $MXE_PATH
cd "$MXE_PATH"
make -j $JOBS MXE_TARGETS=$TARGET libzip qt5

cd -
QTROOT="$MXE_PATH/usr/$TARGET/qt5/bin"
lupdate="$QTROOT/lupdate" lrelease="$QTROOT/lrelease" "$QTROOT/qmake" src/Baka-MPlayer.pro 
make
make install
