#!/bin/sh

set -eu

sudo yum install -y -q cmake make clang
sudo amazon-linux-extras install -y ruby2.6 >/dev/null
git clone --branch master --depth 1 https://github.com/onqtam/doctest.git dtroot >/dev/null
mv dtroot/doctest .
git clone --branch master --depth 1 https://github.com/ismo-karkkainen/edicta.git >/dev/null
cd edicta
sudo rake install >/dev/null
cd ..
git clone --branch master --depth 1 https://github.com/ismo-karkkainen/specificjson.git >/dev/null
mkdir sjbuild
cd sjbuild
cp -r ../doctest .
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../specificjson >/dev/null
make -j 3 >/dev/null
sudo make install >/dev/null
cd ..
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $1
make -j 3
make test
