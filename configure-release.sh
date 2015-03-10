#!/bin/bash

top_dir=$(pwd)

PREFIX=$HOME
LIBDIR=$PREFIX/lib

export PKG_CONFIG_PATH=$LIBDIR/pkgconfig

CXXFLAGS="$CXXFLAGS -g0"
CXXFLAGS="$CXXFLAGS -O3"
export CXXFLAGS

# recreate build folder
rm -fr $top_dir/build-release
mkdir -p $top_dir/build-release

# configure build folder
cd $top_dir/build-release
$top_dir/configure --prefix=$PREFIX --enable-silent-rules



