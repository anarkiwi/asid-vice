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


## Custom features

This fork carries a set of custom features whose logic now lives in the
[revice](https://github.com/anarkiwi/revice) submodule (`src/revice`) as
small, independently-tested libraries:

- **ASID sound** over MIDI — including 2SID and the shorter per-register form
  (`-soundarg +1024`) for a Vessel interface + VAP receiver
- **`keymatrix`** — keyboard-matrix injection (text monitor + binmon)
- **`screenscrape`** / **`SCREEN_GET`** — screen RAM + color RAM + active charset
- **`DRIVE_ATTACH`** — runtime disk image attach/detach over binmon
- **silent-checkpoint** — a per-checkpoint binmon flag for byte-granular polled
  coverage

The command syntax, binary-monitor opcodes (`0x74`-`0x78`), response layouts,
the `-soundarg` encoding and the binmon request/response framing are all
documented in **revice's README**:
[`src/revice/README.md`](src/revice/README.md)
([online](https://github.com/anarkiwi/revice#feature-reference)).

### Quick start: driving headlessly

```
x64sc -default -warp -silent \
      -binarymonitor -binarymonitoraddress ip4://127.0.0.1:6502
```

Connect a binmon client to TCP `6502`, send the `EXIT` opcode (`0xaa`) once to
resume the CPU, then drive the emulator with the opcodes documented in revice's
README. For a headless (no GUI) build, configure with
`./configure --with-alsa --enable-headlessui`; the provided `Dockerfile`
packages exactly that (see below).

### Running headlessly with Docker

A `Dockerfile` is provided at the repo root that builds VICE with
`--enable-headlessui` (no GTK / SDL window) and packages the resulting
`x64sc` binary together with the C64 ROMs into a small runtime image.
The container exposes the binary monitor on TCP `6502` by default;
your application connects to that port and drives the emulator using
the binmon protocol documented in revice's README.

```
# Build
docker build -t asid-vice .

# Run, exposing binmon on host port 6502
docker run --rm -p 6502:6502 asid-vice

# Or pass through a .d64 / .prg by mounting it and using -autostart:
docker run --rm -p 6502:6502 -v "$PWD/Commando.d64:/work/disk.d64:ro" \
    asid-vice -autostart /work/disk.d64
```

The container's default command is roughly:

```
x64sc -default -warp \
      -binarymonitor -binarymonitoraddress ip4://0.0.0.0:6502 \
      -silent
```

so a freshly-started container is already listening for binmon
clients. From there your app can `keymatrix tap` to feed input and
`screenscrape` (`SCREEN_GET`) to read the display every frame —
nothing in the data path requires a display, X server, or audio device.

---

### Caveats

- Symbolic key names and the screen-state provider are C64 only. On
  C128 / VIC-20 / PET / Plus-4 the `keymatrix poke` form still works
  (the matrix interface is shared); `keymatrix` symbolic names and the
  `screenscrape` / `SCREEN_GET` opcode are not implemented for those
  machines — `SCREEN_GET` returns `e_MON_ERR_CMD_FAILURE` (`0x8f`).
- The CIA1 observation hook for `keymatrix` is in `src/c64/c64cia1.c`
  and is therefore C64-only. On other machines `tap` always falls
  through to the safety-net timeout.
- `screenscrape` reflects what the VIC-II would *fetch* from memory at
  this instant. Mid-frame raster effects (e.g. demos that switch
  charsets per scan-line) only show the value at the time of capture;
  if you need per-line state, scrape on a raster IRQ checkpoint.
- If a host user is also pressing keys while you inject from the
  monitor, the host keyboard alarm may re-latch and clobber injected
  bits. Treat keymatrix as something for scripted/paused use; if the
  matrix gets stuck, `keymatrix release` (no args) is a hard reset.
