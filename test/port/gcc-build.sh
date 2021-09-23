#!/bin/sh

set -eu

gem install edicta
gem install specificjson
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release $1
make -j 3
make test
