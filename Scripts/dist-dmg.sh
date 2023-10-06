#!/bin/bash
#
#  dist-dmg.sh: Deploy SigDigger in MacOS disk image format
#
#  Copyright (C) 2021 Gonzalo José Carracedo Carballal
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

BUNDLEID="es.inta-csic.cab.TARSISETC"

BUNDLEPATH="$DEPLOYROOT/usr/bundles/CalGUI.app"
PLISTPATH="$BUNDLEPATH/Contents/Info.plist"
RSRCPATH="$BUNDLEPATH/Contents/Resources"
LIBPATH="$BUNDLEPATH/Contents/Frameworks"
BINPATH="$BUNDLEPATH/Contents/MacOS"
STAGINGDIR="$DEPLOYROOT/TARSISETC.dir"
DMG_NAME="$DISTFILENAME".dmg

function locate_macdeploy()
{
  locate_qt
  
  export PATH="$QT_BIN_PATH:$PATH"
  
  try "Locating hdiutil..." which hdiutil
  try "Locating macdeployqt..." which macdeployqt
}

function bundle_libs()
{
  name="$1"
  shift
  
  try "Bundling $name..." cp -RLfv "$@" "$LIBPATH"
}

function excluded()
{
    excludelist="libc++.1.dylib libSystem.B.dylib"

    for ef in `echo $excludelist`; do
	if [ "$ef" == "$1" ]; then
	   return 0
	fi
    done

    return 1
}

function find_lib()
{
  SANE_DIRS="/usr/lib/`uname -m`-linux-gnu /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 $LIBPATH"
    
  for i in $SANE_DIRS; do
	  if [ -f "$i/$1" ]; then
      echo "$i/$1"
      return 0
    fi
  done

  return 1
}


function deploy_deps()
{
  bundle_libs "yaml-cpp libraries" /usr/local/lib/libyaml-cpp*dylib
  bundle_libs "GCC support libraries"   /usr/local/opt/gcc/lib/gcc/*/libgcc_s*.dylib
}

function remove_full_paths()
{
  FULLPATH="$2"
  RPATHNAME='@executable_path/../Frameworks/'`basename "$FULLPATH"`

  install_name_tool -change "$FULLPATH" "$RPATHNAME" "$1"
}

function remove_full_path_stdin () {
    while read line; do
    remove_full_paths "$1" "$line"
  done
}

function ensure_rpath()
{
  for i in "$LIBPATH"/*.dylib "$BUNDLEPATH"/Contents/MacOS/*; do
      if ! [ -L "$i" ]; then
	  chmod u+rw "$i"
	  try "Fixing "`basename $i`"..." true
	  otool -L "$i" | grep '\t/usr/local/' | tr -d '\t' | cut -f1 -d ' ' | remove_full_path_stdin "$i";
	  otool -L "$i" | grep '\t@rpath/.*\.dylib' | tr -d '\t' | cut -f1 -d ' ' | remove_full_path_stdin "$i";
      fi
  done
}

function create_dmg()
{
  try "Cleaning up old files..." rm -Rfv "$STAGINGDIR"
  try "Creating staging directory..." mkdir -p "$STAGINGDIR"
  try "Copying bundle to staging dir..."   cp -Rfv "$BUNDLEPATH" "$STAGINGDIR"
  try "Creating .dmg file and finishing..." hdiutil create -verbose -volname TARSIS_ETC -srcfolder "$STAGINGDIR" -ov -format UDZO "$DISTROOT/$DMG_NAME"
}

function fix_plist()
{
  try "Setting bundle ID..."                 plutil -replace CFBundleIdentifier -string "$BUNDLEID" "$PLISTPATH"
  try "Setting bundle name..."               plutil -replace CFBundleName -string "TARSISETC" "$PLISTPATH"
  try "Setting bundle version ($RELEASE)..." plutil -replace CFBundleShortVersionString -string "$RELEASE" "$PLISTPATH"
  try "Setting bundle language..."           plutil -replace CFBundleDevelopmentRegion -string "en" "$PLISTPATH"
  try "Setting NOTE..."                      plutil -replace NOTE -string "Bundled width TARSISETC's deployment script" "$PLISTPATH"
}

function deploy()
{
  locate_macdeploy

  try "Deploying via macdeployqt..." macdeployqt "$BUNDLEPATH"
  
  try "Copyingdata directory to bundle..." cp -Rfv "$SRCDIR/data" "$RSRCPATH"
  try "Copying ETC cli tool to bundle..." cp -fv "$DEPLOYROOT/usr/bin/Calculator" "$BINPATH"
  try "Bundling built libraries..." cp -fv "$DEPLOYROOT/usr/lib/"*.dylib "$LIBPATH"

  deploy_deps
  ensure_rpath
  fix_plist
}

build
deploy
create_dmg




