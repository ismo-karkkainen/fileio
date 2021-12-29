#!/bin/sh

set -u
export D=$1
R=$2

(
case "$D" in
Fedora)
    yum install -y libtiff-devel libpng-devel
    ;;
openSUSE*)
    zypper install -y libtiff-devel libpng-devel
    ;;
Debian|Ubuntu)
    apt-get update
    apt-get install -y libtiff-dev libpng-dev
    ;;
esac
) >/dev/null

export C="gem install edicta specificjson"
$C
cd $R

for X in clang++ g++
do
    export X
    mkdir build
    (
        echo "Build $(cat _logs/commit.txt) on $D using $X at $(date '+%Y-%m-%d %H:%M')"
        (
            set -eu
            echo "$C"
            cd build
            CXX=$X cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
            make -j 2
            make test
        )
        echo "Build and test exit code: $?"
    ) 2>&1 | tee -a "$R/_logs/$D-$X.log"
    rm -rf build
done
