#! /bin/sh
mkdir -p m4
mkdir -p aux
autoheader --warnings=all
autoreconf --force --install -I aux -I m4
