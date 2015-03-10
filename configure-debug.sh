#!/bin/bash

top_dir=$(pwd)

PREFIX=$HOME/dev
LIBDIR=$PREFIX/lib

export PKG_CONFIG_PATH=$LIBDIR/pkgconfig

CXXFLAGS="$CXXFLAGS -D DEBUG"
CXXFLAGS="$CXXFLAGS -g3"
CXXFLAGS="$CXXFLAGS -O0"
export CXXFLAGS

# recreate build folder
rm -fr $top_dir/build-debug
mkdir -p $top_dir/build-debug

# configure build folder
cd $top_dir/build-debug
$top_dir/configure --prefix=$PREFIX



