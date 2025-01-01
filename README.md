# asid-vice
VICE with ASID support
======================

This is a fork of VICE, with [ASID](http://paulus.kapsi.fi/asid_protocol.txt) support (remote SID register control over MIDI),
originally posted as a patch in http://midibox.org/forums/topic/17538-vice-emulator-asid-hacks-linux-and-windows/.
This allows VICE to command an Elektron SIDStation, or a C64 with a Vessel interface and [VAP](https://github.com/anarkiwi/vap),
or a C64 with a regular MIDI interface and [Station64](https://csdb.dk/release/?id=142049).  ASID isn't fast enough for sample playback.

## Building
### Installing build dependencies
```
sudo apt update && sudo apt install \
	autoconf \
	build-essential \
	byacc \
	dos2unix \
	flex \
	libasound2-dev \
	libevdev-dev \
	libglew-dev \
	libglib2.0-dev \
	libpng-dev \
	libsdl2-dev \
	libsdl2-image-dev \
	texinfo \
	texmaker \
	xa65
```

### Compiling 
```
./autogen.sh && ./configure --with-alsa && make -j
```

### Installing
```
sudo make install
```

## Running
To enable the asid output, use the `-sounddev asid` command line option.
Select the midi port to use with `-soundarg`.
The list of ports is output when the asid driver is started (either on the command line, or in vice.log)
It defaults to port 0, which is usually some internal port.

```
vsid -sounddev asid -soundarg 1 Commando.sid
```

```
x64sc -sounddev asid -soundarg 1 Commando.d64
```

The midi port can also be set in `vice.ini`:
```
SoundDeviceName="asid"
SoundDeviceArg="1"
```

### 2SID support

2SID is supported with Vessel

```
vsid -sound -soundoutput 2 -sidextra 1 -sounddev asid -soundarg 1 Voice_2SID.sid 
```
