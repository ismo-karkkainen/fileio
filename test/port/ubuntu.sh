#!/bin/sh

set -eu
sudo apt-get update
sudo apt-get install -y -q cmake build-essential ruby libtiff-dev libpng-dev >/dev/null
$1/test/port/gcc-build.sh $1
