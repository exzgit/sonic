#!/bin/bash

set e;

meson compile -C build
sudo cp ./build/sonic /usr/bin/sonic