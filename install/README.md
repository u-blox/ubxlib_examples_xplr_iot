# Installation

This directory contains the operating system specific installation scripts for the development tools required to build the XPLR-IOT-1 examples.

These scripts installs all the necessary packages, tools and drivers. The main installation is made using the Nordic installation program for nRF Connect and is thereby made into a separate space not interfering with possible already installed or different versions of tools. It will be installed in the standard directory, *C:\ncs* for Windows and *~/ncs* for Linux.

The script will also install VS Code, JLink software and possible required drivers (unless already present). This will require administrator rights and hence a privilege an elevation prompt will occur.

Lastly the script will clone this repository. It will be stored in a sub directory to your home directory named *ubxlib_examples_xplr_iot*. Start a command prompt and change working directory to this and then you can start the operations described on the [main README page](https://github.com/u-blox/ubxlib_examples_xplr_iot).

The installation will take quite some time (10-20 minutes) so please be patient. The main affecting factor is the download speed.

Please note that this is a silent installation and that you by running it implicitly accept all possible license agreements for the installed tools.

## Windows

Start a command window and enter the following command:

    curl -JLs -o %TMP%\i.bat https://github.com/u-blox/ubxlib_examples_xplr_iot/raw/master/install/install_windows.bat && %TMP%\i.bat

**Please avoid** clicking in the window of this operation if you have *Quick Edit* for command windows enabled. Doing so may halt the downloading process which in turn can lead to timeouts and later problems for the installation.

## Linux

Start a command window and enter the following command:

    wget -qO - https://github.com/u-blox/ubxlib_examples_xplr_iot/raw/master/install/install_linux | $SHELL

