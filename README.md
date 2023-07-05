# Rosnehook
[![C++](https://img.shields.io/badge/language-C%2B%2B-%23f34b7d.svg?style=flat-square)](https://en.wikipedia.org/wiki/C%2B%2B)
[![TF2](https://img.shields.io/badge/game-TF2-orange.svg?style=flat-square)](https://store.steampowered.com/app/440/Team_Fortress_2/)
[![GNU/Linux](https://img.shields.io/badge/platform-GNU%2FLinux-ff69b4?style=flat-square)](https://www.gnu.org/gnu/linux-and-gnu.en.html)
[![x86](https://img.shields.io/badge/arch-x86-red.svg?style=flat-square)](https://en.wikipedia.org/wiki/X86)
[![License](https://img.shields.io/github/license/explowz/cathook.svg?style=flat-square)](LICENSE)
[![Issues](https://img.shields.io/github/issues/explowz/cathook.svg?style=flat-square)](https://github.com/rosneburgerworks/cathook/issues)

Improved version of cathook without bloat.

## Requirements
* A linux computer capable of running TF2.
* Atleast 4 GB ram installed with 120 GB disk space per linux computer configuration.

## Downloading
Open a terminal window and enter this command to your terminal:

    bash <(wget -qO- https://raw.githubusercontent.com/rosneburgerworks/One-in-all-cathook-install/master/install-all)

This bash script will automatically installs cathook to your linux computer.


## Injecting

When ronsehook finished installing you go to the `cathook` folder and open a terminal and enter this command:

    sudo ./attach
If other injection or above command did not work you can try alternative injection method to let rosnehook into your `hl2_linux` process:
    
    sudo ./attach-gdb
    sudo ./attach-libnamed
