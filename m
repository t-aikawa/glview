#!/usr/bin/bash

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ start @@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
date

opengl=$1

if [ -z "$opengl"]; then
    opengl="gles"
fi

rm -fr builddir

meson builddir -Dopengl=$opengl
#meson builddir -Dopengl=gles
#meson builddir -Dopengl=opengl

cd builddir

ninja

date
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@ end @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

exit 0
