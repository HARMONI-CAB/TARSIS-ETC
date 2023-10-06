#!/bin/bash
#
#  dist-common.sh: Build TARSIS-ETC
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

DISTROOT="$PWD"
OSTYPE=`uname -s`
ARCH=`uname -m`
RELEASE="0.1.0"
DISTFILENAME=TARSISETC-"$RELEASE"-"$ARCH"
PKGVERSION=""
MAKE="make"

if [ "x$BUILDTYPE" == "x" ]; then
    BUILDTYPE="Release"
fi


opt=$1
SCRIPTNAME="$0"

if [ "$OSTYPE" == "Linux" ]; then
    SCRIPTPATH=`realpath "$0"`
else
    SCRIPTPATH="`greadlink -f $0`"
fi

SCRIPTDIR=`dirname "$SCRIPTPATH"`
SRCDIR="${SCRIPTDIR}/.."
DEPLOYROOT="$DISTROOT/deploy-root"
BUILDROOT="$DISTROOT/build-root"

if [ "$OSTYPE" == "Linux" ] || is_mingw; then
  THREADS=`cat /proc/cpuinfo | grep processor | wc -l`
elif [ "$OSTYPE" == "Darwin" ]; then
  THREADS=`sysctl -n hw.ncpu`
else
  THREADS=1
fi

function try()
{
    echo -en "[  ....  ] $1 "
    shift
    
    STDOUT="$DISTROOT/$1-$$-stdout.log"
    STDERR="$DISTROOT/$1-$$-stderr.log"
    echo "Try: $@"    >> "$STDERR"
    echo "CWD: $PWD"  >> "$STDERR"
    
    "$@" > "$STDOUT" 2>> "$STDERR"
    
    if [ $? != 0 ]; then
	echo -e "\r[ \033[1;31mFAILED\033[0m ]"
	echo 
	echo '--------------8<----------------------------------------'
	cat "$STDERR"
	echo '--------------8<----------------------------------------'

	if [ "$BUILDTYPE" == "Debug" ]; then
	    echo
	    echo 'Debug mode - attatching standard output:'
	    echo '--------------8<----------------------------------------'
	    cat "$STDOUT"
	    echo '--------------8<----------------------------------------'
	    echo ''
	    echo 'Deploy root filelist: '
	    echo '--------------8<----------------------------------------'
	    ls -lR "$DEPLOYROOT"
	    echo '--------------8<----------------------------------------'

   	    echo 'Build root filelist: '
	    echo '--------------8<----------------------------------------'
	    find "$BUILDROOT"
	    echo '--------------8<----------------------------------------'
	    echo ''
	fi
	
	echo
	echo "Standard output and error were saved respectively in:"
	echo " - $STDOUT"
	echo " - $STDERR"
	echo
	echo "Fix errors and try again"
	echo
	exit 1
    fi

    echo -e "\r[   \033[1;32mOK\033[0m   ]"
    rm "$STDOUT"
    rm "$STDERR"
}

function skip()
{
    echo -e "[  \033[1;33mSKIP\033[0m  ] $1"
}

function notice()
{
    echo -e "[ \033[1;33mNOTICE\033[0m ] $1"
}

function locate_qt()
{
    if [ "x$QT_BIN_PATH" == "x" ]; then
      try "Locating Qt..." which qmake
    
      export QT_QMAKE_PATH=`which qmake`
      export QT_BIN_PATH=`dirname "$QT_QMAKE_PATH"`
    fi
}

function locate_sdk()
{
  try "Locate Git..." which git

  export CMAKE_EXTRA_OPTS='-GUnix Makefiles'

  try "Locate Make..." which $MAKE
  try "Ensuring pkgconfig availability... " which pkg-config
  
  locate_qt
  
  try "Locating CMake..." which cmake 
}

function build()
{
    if [ "$BUILDTYPE" == "Debug" ]; then
      notice 'Build with debug symbols is ON!'
      CMAKE_BUILDTYPE=Debug
      QMAKE_BUILDTYPE=debug
      export VERBOSE=1
    else
      CMAKE_BUILDTYPE=Release
      QMAKE_BUILDTYPE=release
    fi

    locate_sdk

    export PKG_CONFIG_PATH="$DEPLOYROOT/usr/lib/pkgconfig:$PKG_CONFIG_PATH"
    export LD_LIBRARY_PATH="$DEPLOYROOT/usr/lib:$LD_LIBRARY_PATH"
    try "Cleaning deploy directory..." rm -Rfv  "$DEPLOYROOT"
    try "Cleaning buildroot..."        rm -Rfv  "$BUILDROOT"
    try "Recreating directories..."    mkdir -p "$DEPLOYROOT" "$BUILDROOT"
    
    cd "$BUILDROOT"
    
    try "Running CMake ..."    cmake "${SRCDIR}" $CMAKE_SUSCAN_EXTRA_ARGS -DCMAKE_INSTALL_PREFIX="/usr" -DPKGVERSION="$PKGVERSION" -DCMAKE_BUILD_TYPE=$CMAKE_BUILDTYPE "$CMAKE_EXTRA_OPTS" -DCMAKE_SKIP_RPATH=ON -DCMAKE_SKIP_INSTALL_RPATH=ON
    try "Building..."           $MAKE -j $THREADS
    try "Deploying..."          $MAKE DESTDIR="$DEPLOYROOT" -j $THREADS install

    cd "$SCRIPTDIR"
}


ESCAPE=`echo -en '\033'`
echo -en '\033[0;1m'

cat << EOF
Welcome to $ESCAPE[1;36mTARSIS-ETC$ESCAPE[0;1m...multi platform deployment script.$ESCAPE[0m
EOF

echo
echo "Attempting deployment on $OSTYPE ($ARCH) with $THREADS threads"
echo "Date: "`date`
echo "TARSIS-ETC release to be built: $RELEASE"
echo
