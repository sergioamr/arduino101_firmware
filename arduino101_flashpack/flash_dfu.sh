#!/bin/sh 
set -e
#
# Script to flash Arduino 101 firmware via USB and dfu-util
#
PID="8087:0aba"
IMG="images/firmware"
os="$(uname)"

if [ x"$os" = x"Darwin" ]; then
  BIN="bin_osx/dfu-util"
  export DYLD_LIBRARY_PATH=bin_osx:$DYLD_LIBRARY_PATH
else
  DIR="$( dirname `readlink -f $0`)"
  cd "$DIR"

  BIN="bin/dfu-util"
  arch="$(uname -m)" 2>/dev/null
  if [ x"$arch" = x"i686" ]; then
    BIN="bin/dfu-util.32"
  fi
fi

help() {
  echo "Usage: $0 [options]
          -b                Flash bootloader
          -s serial_number  Only flash to board with specified serial number"
  exit 1
}

flash_bl() {
  echo "

** Flashing Bootloader **

"
  $DFU -a 7 -D $IMG/bootloader_quark.bin
  $DFU -a 2 -R -D $IMG/bootupdater.bin
  echo "*** Sleeping for 12 seconds..."
  sleep 12
}

flash() {
  echo "

** Flashing Quark **

"
  $DFU -a 2 -D $IMG/quark.bin
  $DFU -a 7 -D $IMG/arc.bin -R
}

trap_to_dfu() {
  # If trapped.bin already exists, clean up before starting the loop
  [ -f "trapped.bin" ] && rm -f "trapped.bin"

  # Loop to read from 101 so that it stays on DFU mode afterwards
  until $DFU -a 4 -U trapped.bin > /dev/null 2>&1
  do
    sleep 0.1
  done

  # Clean up
  [ -f "trapped.bin" ] && rm -f "trapped.bin"

  # If a serial number is not specified by the user, read it from the board
  if [ -z "$ser_num" ]; then
    x=$($DFU -l 2>/dev/null |grep sensor|head -1)
    ser_num="-S $(echo $x|awk -F= {'print $8'}|sed -e 's/\"//g')"
    DFU="$BIN $ser_num -d,$PID"
  fi
}

bl_warning() {
  echo " 
***************************************************************************
*                           *** WARNING ***                               *
* Flashing a bootloader may brick your board.                             *
* Do not flash bootloader unless you were explicitly instructed to do so. *
*                       PROCEED AT YOUR OWN RISK                          *
***************************************************************************

"
  read -p "Proceed with flashing bootloader? [y/N]" answer
  case $answer in
    [yY]*)
      ;;
    *)
      exit 1
    ;;
  esac
}

FLASH_BOOTLOADER=false
# Parse command args
while [ $# -gt 0 ]; do
  arg="$1"
  case $arg in
    -b)
      bl_warning
      FLASH_BOOTLOADER=true
      ;;
    -s)
      ser_num=$2
      if [ -z "$ser_num" ]; then help; fi
      ser_num_param="-S $ser_num"
      shift # past argument
      ;;
    *)
      help # unknown option
      ;;
  esac
  shift # past argument or value
done

DFU="$BIN $ser_num_param -d,$PID"

echo "*** Reset the board to begin..."
trap_to_dfu

echo Flashing board S/N: $ser_num
if $FLASH_BOOTLOADER ; then
  flash_bl
fi
flash

if [ $? -ne 0 ]; then
  echo
  echo "***ERROR***"
  exit 1
else
  echo
  echo "!!!SUCCESS!!!"
  exit 0
fi

