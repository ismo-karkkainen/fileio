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

rwimageinputgen -i pspecs -w $W -h $H -c $C -d $D -f imagefile --format $F --read read.json --write write.json

$WI < write.json
$RI < read.json > out.json

pixeldiff --reference write.json --test out.json --depth $D
STATUS=$?

if [ -z $KEEP ]; then
    rm -f imagefile write.json read.json out.json
fi
exit $STATUS
