#!/bin/bash
set -e
topdir=$(dirname `readlink -e $0`)
cd $topdir
# verify build directory exists
fwdir=$topdir/../out/current/firmware
if [ ! -d $fwdir ]; then
  echo "$fwdir does not exist."
  exit 1
fi
# create readable tag for flasher package
if [ -n "$1" ]; then 
  tag=_$(echo $1 |sed -e "s/\//-/g" -)
  echo $tag
fi
flasher=flashpack${tag}.zip

# copy .bin and partition.conf files into flasher package
rm -rf images
mkdir images/
rsync -avm --include='*.bin' --include='*partition.conf' -f 'hide,! */' $fwdir/ images/firmware/

# create flasher package
cd ..
if [ -n "$tag" ]; then
  rsync -rav --exclude='.git' --exclude='create_flasher.sh' arduino101_flashpack/ arduino101_flashpack${tag}
fi
rm -rf $flasher
zip -r $flasher arduino101_flashpack${tag}/ -x *.git/*
