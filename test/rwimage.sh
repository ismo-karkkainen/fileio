#!/bin/sh

if [ $# -ne 7 ]; then
    echo "Usage: $(basename $0) width height components depth format readimage writeimage"
    exit 1
fi

W=$1
H=$2
C=$3
D=$4
F=$5
RI=$6
WI=$7

rwimageinputgen -i pspecs -w $W -h $H -c $C -d $D -f imagefile --format $F

$WI < writeimage_io.json
$RI < readimage_io.json > out.json

pixeldiff --reference writeimage_io.json --test out.json --depth $D
STATUS=$?

if [ -z $KEEP ]; then
    rm -f imagefile writeimage_io.json readimage_io.json split2planes_io.json out.json
fi
exit $STATUS
