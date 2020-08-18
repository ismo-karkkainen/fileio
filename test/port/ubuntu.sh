#!/bin/sh
set -eu
sudo apt-get install -y -q cmake build-essential ruby libtiff-dev libpng-dev >/dev/null
git clone --branch master --depth 1 https://github.com/onqtam/doctest.git dtroot
mv dtroot/doctest .
git clone --branch master --depth 1 https://github.com/ismo-karkkainen/edicta.git
cd edicta
sudo rake install
cd ..
git clone --branch master --depth 1 https://github.com/ismo-karkkainen/specificjson.git
mkdir sjbuild
cd sjbuild
cp -r ../doctest .
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../specificjson
make -j 3
sudo make install
cd ..
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $1
make -j 3
make test
