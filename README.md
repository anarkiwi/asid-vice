# asid-vice
VICE with ASID support
======================

This is a fork of VICE, with [ASID](http://paulus.kapsi.fi/asid_protocol.txt) support (remote SID register control over MIDI),
originally posted as a patch in http://midibox.org/forums/topic/17538-vice-emulator-asid-hacks-linux-and-windows/.
This allows VICE to command an Elektron SIDStation, or a C64 with a Vessel interface and [VAP](https://github.com/anarkiwi/vap),
or a C64 with a regular MIDI interface and [Station64](https://csdb.dk/release/?id=142049).

## Building

```
sudo apt-get update && sudo apt-get install -y libasound2-dev
./autogen.sh && ./configure --with-alsa && make -j && sudo make install
```

## Running

```
vsid -soundfragsize 2 -soundbufsize 22 -sounddev asid -soundarg 1 Commando.sid
```

```
x64 -soundfragsize 2 -soundbufsize 22 -sounddev asid -soundarg 1 Commando.d64
```

## Limitations

* Since ASID must tunnel 8 bit values over MIDI (which is not 7 bit clean), it has high overhead, aside from MIDI latency.
* ASID isn't fast/efficient enough for sample playback.
