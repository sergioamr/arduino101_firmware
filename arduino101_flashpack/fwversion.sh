#!/bin/bash
#
# Read version strings from DFU memory
#
file='/tmp/dfuver.txt'
readbin='bin/readbin'
DFU='bin/dfu-util'
os="$(uname)"

if [ x"$os" = x"Darwin" ]; then
  DFU="bin_osx/dfu-util"
  readbin="bin_osx/readbin"
  export DYLD_LIBRARY_PATH=bin_osx:$DYLD_LIBRARY_PATH
else
  arch="$(uname -m)" 2>/dev/null
  if [ x"$arch" = x"i686" ]; then
    DFU="bin/dfu-util.32"
    readbin='bin/readbin.32'
  fi
fi

ARGS=$*
if [ -z "$*" ] ; then
   ARGS="1 2"
fi

# wait for DFU mode
echo ""
echo "Reset the board to proceed..."
X=''
while [ -z "$X" ]; do
  X=$($DFU -l |grep "sensor")
done

# dump DFU partition memory and parse for version
for i in $ARGS;
do
  if [ -f $file ]; then
    rm $file
  fi
  $DFU -d8087:0ABA -a $i -t 1 -U $file >/dev/null
  $readbin $file
done
rm -f $file

# detatch from DFU
$DFU -e >/dev/null 2>&1

