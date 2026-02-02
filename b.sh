#!/bin/bash

set -e

clear
meson compile -C build
sudo cp ./build/sonic /usr/bin/sonic
mkdir -p ~/.local/sonic/lib

clear

sonic compile examples
