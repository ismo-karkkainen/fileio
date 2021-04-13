#!/bin/sh

set -eu
sudo yum install -y -q cmake make clang
sudo amazon-linux-extras install -y ruby2.6 >/dev/null
$1/test/port/clang-build.sh $1
