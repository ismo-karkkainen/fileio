#!/bin/sh

set -eu
sudo zypper install -y cmake make gcc-c++ ruby libtiff-devel libpng-devel >/dev/null
$1/test/port/gcc-build.sh $1
