# asid-vice
VICE with ASID support
======================

This is a fork of VICE, with [ASID](http://paulus.kapsi.fi/asid_protocol.txt) support (remote SID register control over MIDI),
originally posted as a patch in http://midibox.org/forums/topic/17538-vice-emulator-asid-hacks-linux-and-windows/.
This allows VICE to command an Elektron SIDStation, or a C64 with a Vessel interface and [VAP](https://github.com/anarkiwi/vap),
or a C64 with a regular MIDI interface and [Station64](https://csdb.dk/release/?id=142049).  ASID isn't fast enough for sample playback.

## Docker image

The published image (`anarkiwi/asid-vice` on Docker Hub) is built from
[`Dockerfile.x11`](Dockerfile.x11): the full **GTK3 (X11) VICE** with the widest
practical set of options enabled — reSID + HardSID + ParSID, ASID-over-MIDI and
the emulated MIDI cartridge, The Final Ethernet, FLAC / Ogg-Vorbis / MP3
(mpg123 decode, LAME encode), PNG + GIF screenshots, libcurl, and `ffmpeg`
video capture. It is a multi-stage build, so the runtime image carries only the
binaries, ROMs and shared libraries. Each `v*` release tag (e.g. `v3.10.0.0`)
publishes it via
[`.github/workflows/release.yml`](.github/workflows/release.yml), which needs
the repo secrets `DOCKER_USERNAME` and `DOCKER_TOKEN`.

### Pull from Docker Hub
```
docker pull anarkiwi/asid-vice:latest
```

### Build locally
```
docker build -f Dockerfile.x11 -t asid-vice:x11 .
```

### Running X11 inside a container

The GTK3 UI needs an X server. The entrypoint auto-starts `Xvfb` when no
`DISPLAY` is set, so the image runs in a bare container out of the box; the
binary monitor is exposed on TCP `6502`.
```
# Virtual framebuffer, no host X server needed
docker run --rm -p 6502:6502 anarkiwi/asid-vice:latest

# Show a real window on the host X server (Linux)
docker run --rm -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix \
    -p 6502:6502 anarkiwi/asid-vice:latest

# Autostart a disk/prg by mounting it (extra args replace the default CMD,
# so start them with the emulator binary):
docker run --rm -p 6502:6502 -v "$PWD/Commando.d64:/work/disk.d64:ro" \
    anarkiwi/asid-vice:latest x64sc -autostart /work/disk.d64
```

For ASID over a real MIDI device, pass the host ALSA devices through with
`--device /dev/snd` and select the port with `-sounddev asid -soundarg <n>`
(see below).

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
This minimal `Dockerfile` is the alternative to the published X11 image
([Docker image](#docker-image) above) when all you need is binmon driving.

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
