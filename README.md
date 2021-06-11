# asid-vice
VICE with ASID support
======================

This is a fork of VICE 3.4, with [ASID](http://paulus.kapsi.fi/asid_protocol.txt) support (remote SID register control over MIDI),
originally posted as a patch in http://midibox.org/forums/topic/17538-vice-emulator-asid-hacks-linux-and-windows/.
This allows VICE to command an Elektron SIDStation, or a C64 with a Vessel interface and [VAP](https://github.com/anarkiwi/vap),
or a C64 with a regular MIDI interface and [Station64](https://csdb.dk/release/?id=142049).  ASID isn't fast enough for sample playback.

TODO: Currently incompatible with VICE 3.5 (due an an architecture change, involving synchronization between frames and sound fragments),

## Building

```
sudo apt-get update && sudo apt-get install -y libasound2-dev
./autogen.sh && ./configure --with-alsa && make -j && sudo make install
```

## Running

```
vsid -sounddev asid -soundarg 1 Commando.sid
```

```
x64 -sounddev asid -soundarg 1 Commando.d64
```
