#!/bin/sh

if [ $# -ne 6 ]; then
    echo "Usage: $(basename $0) width height components depth index split2planes"
    exit 1
fi

W=$1
H=$2
C=$3
D=$4
I=$5
SP=$6

rwimageinputgen -i readimage_io.pspec -w $W -h $H -c $C -d $D -f imagefile
rwimageinputgen -i writeimage_io.pspec -w $W -h $H -c $C -d $D -f imagefile
rwimageinputgen -i split2planes_io.pspec -w $W -h $H -c $C -d $D -f imagefile

$SP < split2planes_io.json > out.json

pixeldiff --reference split2planes_io.json --test out.json --depth $D --channel $I
STATUS=$?

if [ -z $KEEP ]; then
    rm -f writeimage_io.json readimage_io.json split2planes_io.json out.json
fi
exit $STATUS
