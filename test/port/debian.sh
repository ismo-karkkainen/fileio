#!/bin/sh

set -eu
sudo apt-get update >/dev/null
sudo apt-get install -y -q cmake make clang ruby libtiff-dev libpng-dev >/dev/null
$1/test/port/clang-build.sh $1
