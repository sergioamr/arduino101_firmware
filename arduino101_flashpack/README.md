# Arduino/Genuino 101 flashpack

This framework is used to package the firmware and flashing scripts. The
scripts flash the required firmware images via DFU, using dfu-util.

See DFU-UTIL.README for license and source information

## Use
After building the firmware source code, generate a flashpack by:

1. ./create_flasher.sh [tag]

    where "tag" is an optional string that will be appended to the flashpack
filename.

#License

The bash scripts are GPLv2 licensed. Every other software used by these bash
scripts has its own license. Consult them to know the terms.

