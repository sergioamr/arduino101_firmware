﻿                           Arduino 101 Firmware Update

Table of Contents

    1. INTRODUCTION
    2. DOWNLOADING THE SOURCE CODE
    3. INSTALLING ALL PREREQUISITE PACKAGES
    4. BUILDING THE IMAGES
    5. CREATING THE FLASHPACK.ZIP
    6. FLASHING IMAGES TO THE BOARD
        6.1 Windows
        6.2 Linux / Mac
------------------------------------------------------------------------------
1. INTRODUCTION
------------------------------------------------------------------------------

###############################################################################

                                   WARNING!!!

    This firmware is targeted to be built only on Ubuntu 64 bit.
    If your native machine is not as mentioned above you can still perform
    the firmware building process using an Ubuntu 14 – 64 bit OS in a virtual
    machine with 15GB of HDD space allocated. We advise against building using
    a Live-USB or Live CD because step 3 below will fail as it cannot download
    the required packages.

###############################################################################

If you have not configured the Arduino101 board yet, please follow the steps in
the link below before downloading the source code. Please verify that you can
successfully run a blink sketch to ensure DFU is operating correctly.
This is important as it will provide you with the platform to flash the
binaries.

   (https://www.arduino.cc/en/Guide/Arduino101)

------------------------------------------------------------------------------
2. DOWNLOADING THE SOURCE CODE
------------------------------------------------------------------------------
Please visit (https://downloadcenter.intel.com/download/25832) and download
the tar-ball located there. When the downloading is complete, move into folder
where the tar-ball is saved and execute the following commands:

    $ tar -xf arduino101_firmware_source-v1.tar.bz2
    $ cd arduino101_firmware_source-v1
    $ project_directory=$(pwd)/arduino101_firmware/projects/arduino101/

------------------------------------------------------------------------------
3. INSTALLING ALL PREREQUISITE PACKAGES
------------------------------------------------------------------------------
Make sure packages from all repositories are up-to-date

    $ sudo apt-get update

Ensure you have all the required packages before compiling. As the target
suggests, this is only required the first time you compile.

    $ sudo make one_time_setup -C $project_directory

This installs the following packages:

    gawk wget git-core diffstat unzip texinfo
    gcc-multilib build-essential chrpath libsdl1.2-dev xterm
    libqtgui4:i386 libtool libc6:i386
    libc6-dev-i386 autoconf libtool pkg-config gperf flex bison

------------------------------------------------------------------------------
4. BUILDING THE IMAGES
------------------------------------------------------------------------------
Perform this step each time you modify the code, to update
the images.

    $ make clean setup image -C $project_directory

------------------------------------------------------------------------------
5. CREATING THE FLASHPACK.ZIP
------------------------------------------------------------------------------
This creates "flashpack.zip" which is used for flashing the board.

    $ arduino101_flashpack/create_flasher.sh

------------------------------------------------------------------------------
6. FLASHING IMAGES TO THE BOARD
------------------------------------------------------------------------------
Ensure the board is connected via USB.

Now move the zip file to a machine where the Arduino IDE is installed
and extract there.

6.1 Windows

    Shift+Ctrl+right click mouse on extracted folder and click "Open command
    window here".
    Execute command "flash_dfu.bat"

6.2 Linux / Mac

    $ cd arduino101_flashpack
    $ ./flash_dfu.sh

Press the reset button on the board to begin the flash process.
