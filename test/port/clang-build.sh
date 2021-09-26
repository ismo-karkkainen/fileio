#!/bin/sh
set -eu

echo 'Require edicta specificjson'
gem install edicta specificjson
CXX=clang++ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $1
make -j 3
make test
