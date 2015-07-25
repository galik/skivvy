#! /bin/sh
mkdir -p m4
mkdir -p config
autoheader --warnings=all
autoreconf --force --install -I config -I m4
