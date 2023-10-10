#!/bin/bash
#
#  dist-appimage.sh: Deploy TARSIS-ETC in AppImage format
#
#  Copyright (C) 2023 Gonzalo José Carracedo Carballal
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>
#
#

. dist-common.sh

function update_excludelist()
{
    URL="https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist"
    LIST="$DISTROOT/excludelist"
    if [ ! -f "$LIST" ]; then
	try "Downloading exclude list from GitHub..." curl -o "$LIST" "$URL"
    else
	try "Upgrading exclude list from GitHub..." curl -o "$LIST" -z "$LIST" "$URL"
    fi

    echo "libusb-0.1.so.4" >> "$LIST"
    
    cat "$LIST" | sed 's/#.*$//g' | grep -v '^$' | sort | uniq > "$LIST".unique
    NUM=`cat "$LIST".unique | wc -l`
    try "Got $NUM excluded libraries" true 
}

function excluded()
{
    grep "$1" "$DISTROOT/excludelist.unique" > /dev/null 2> /dev/null
    return $?
}

function assert_symlink()
{
  rm -f "$2" 2> /dev/null
  ln -s "$1" "$2"
  return $?
}

function embed_calculator_deps()
{
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$DEPLOYROOT/usr/lib"

    DEPS=`ldd "$DEPLOYROOT"/usr/bin/Calculator | grep '=>' | sed 's/^.*=> \(.*\) .*$/\1/g' | tr -d '[ \t]' | sort | uniq`

    for i in $DEPS; do
	name=`basename "$i"`
	if [ ! -f "$DEPLOYROOT"/usr/lib/"$name" ] && ! excluded "$name"; then
	    try "Bringing $name..." cp -L "$i" "$DEPLOYROOT"/usr/lib
	else
	    rm -f "$DEPLOYROOT"/usr/lib/"$name"
	    skip "Skipping $name..."
	fi
    done
}

function build_fixups()
{
    cp -L "${BUILDROOT}/LibETC/libETC.so" "${DEPLOYROOT}/usr/lib"
    return 0
}

update_excludelist

build

try "Creating appdir..."    mkdir -p "$DEPLOYROOT"/usr/share/applications
try "Creating datadir..."   mkdir -p "$DEPLOYROOT"/usr/share/TARSIS
try "Copying data..."       cp -Rfv "${SRCDIR}/data" "$DEPLOYROOT"/usr/share/TARSIS
# try "Creating metainfo..."  mkdir -p "$DEPLOYROOT"/usr/share/metainfo
# try "Copying metainfo..."   cp "$SCRIPTDIR/SigDigger.appdata.xml" "$DEPLOYROOT"/usr/share/metainfo/org.actinid.SigDigger.xml
try "Creating icondir..."   mkdir -p "$DEPLOYROOT"/usr/share/icons/hicolor/256x256/apps
 try "Copying icons..." cp "${SRCDIR}/CalGUI/icons/TARSISETC.png" "$DEPLOYROOT"/usr/share/icons/hicolor/256x256/apps
echo "[Desktop Entry]
Type=Application
Name=TARSIS ETC
Comment=TARSIS Exposure Time Calculation tool
Exec=TARSISETC
Icon=TARSISETC
Categories=Science;" > "$DEPLOYROOT"/usr/share/applications/TARSISETC.desktop

embed_calculator_deps

try "Executing build fixups..." build_fixups
try "Running silly linuxdeployqt workaround..." cp "$DEPLOYROOT"/usr/bin/CalGUI "$DEPLOYROOT"/usr/bin/TARSISETC
try "Calling linuxdeployqt..." linuxdeployqt "$DEPLOYROOT"/usr/share/applications/TARSISETC.desktop -bundle-non-qt-libs -extra-plugins=iconengines
try "Copying wrapper script..." cp "$SCRIPTDIR"/AppRun "$DEPLOYROOT"/usr/bin/TARSISETC
try "Setting permissions to wrapper script..." chmod a+x "$DEPLOYROOT"/usr/bin/TARSISETC
try "Calling AppImageTool and finishing..." appimagetool "$DEPLOYROOT"
