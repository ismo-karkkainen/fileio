#!/bin/sh
set -eu

clone () {
    git clone --branch master --depth 1 $1 >/dev/null
}

clone https://github.com/ismo-karkkainen/edicta.git
clone https://github.com/ismo-karkkainen/specificjson.git
cd edicta
sudo rake install >/dev/null
cd ..
mkdir sjbuild
cd sjbuild
CXX=clang++ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../specificjson >/dev/null
make -j 3 >/dev/null
sudo make install >/dev/null
cd ..
CXX=clang++ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $1
make -j 3
make test
