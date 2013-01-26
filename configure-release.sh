#!/bin/sh

top_dir=$(pwd)

export CXXFLAGS="-g0 -O3 -std=gnu++11 -Wno-unused-parameter -Wno-unused-variable"

mkdir -p $top_dir/build-release
cd $top_dir/build-release
$top_dir/configure --prefix=$top_dir/install

