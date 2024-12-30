#!/bin/bash
set -e
sudo apt update && sudo apt install \
  autoconf \
  build-essential \
  byacc \
  dos2unix \
  flex \
  libasound2-dev \
  libcurl4-openssl-dev \
  libevdev-dev \
  libglew-dev \
  libglib2.0-dev \
  libgtk-3-dev \
  libpng-dev \
  libpulse-dev \
  texmaker \
  xa65
./autogen.sh
./configure --with-alsa
make -j
