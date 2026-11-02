#!/bin/bash
set -e
sudo apt-get -qy update
sudo apt-get -qy install \
  autoconf \
  build-essential \
  byacc \
  dos2unix \
  file \
  flex \
  libasound2-dev \
  libcurl4-openssl-dev \
  libevdev-dev \
  libglew-dev \
  libglib2.0-dev \
  libgtk-3-dev \
  libpng-dev \
  libpulse-dev \
  texinfo \
  texmaker \
  xa65
./autogen.sh
./configure --with-alsa --disable-html-docs
make -j
