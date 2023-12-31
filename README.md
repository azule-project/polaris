# GNU Polaris x86

[![C++](https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg?style=flat-square)](https://en.wikipedia.org/wiki/C%2B%2B)
[![TF2](https://img.shields.io/badge/game-TF2-blue.svg?style=flat-square)](https://store.steampowered.com/app/440/Team_Fortress_2/)
[![GNU/Linux](https://img.shields.io/badge/platform-GNU%2FLinux-ff69b4?style=flat-square)](https://www.gnu.org/gnu/linux-and-gnu.en.html)
[![x86](https://img.shields.io/badge/arch-x86-green.svg?style=flat-square)](https://en.wikipedia.org/wiki/X86)
[![License](https://img.shields.io/github/license/weebwares/cathook.svg?style=flat-square)](LICENSE)
[![Issues](https://img.shields.io/github/issues/weebwares/cathook.svg?style=flat-square)](https://github.com/weebwares/cathook/issues)

> **Warning**<br/> This version of cathook is outdated and broken.

## Requirements
* A linux computer with the minimum requirements to play TF2 at a playable framerate.
* Atleast 4 GB ram installed with 120 GB disk space per linux computer configuration.

## Downloading
Open a terminal window and enter this command to your terminal:

    bash <(wget -qO- https://raw.githubusercontent.com/azule-project/polaris-installer/master/install-all)

This bash script will automatically installs cathook to your linux computer.


## Attaching
When cathook has finished installing on your computer you need to go to the `cathook` and enter this terminal command:

    sudo ./attach

## Debugging
This version of cathook contains a debug builder (in terminal: `./debug-builder`) which builds a debug symbols version of cathook. This is helpful when submitting crash reports. But to do this you must enter this command in your terminal:


    sudo ./attach-gdb

This will attaches GDB and lets you see where and how did cathook crash.
