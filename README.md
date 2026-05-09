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

## Running with Vessel/VAP

These features are only available using the Vessel interface and the VAP receiver.

### 2SID support

```
vsid -sound -soundoutput 2 -sidextra 1 -sounddev asid -soundarg 1 2sidfile.sid
```

### Lower-latency register updates

The ASID protocol is not efficient when updating only a few registers.

Adding `1024` to `-soundarg` enables use of shorter register update messages, which reduces MIDI bandwidth and processing.

```
vsid -sound -sounddev asid -soundarg 1025 sidfile.sid
```

## Headless control and screen-scraping (monitor)

This fork adds two C64-focused commands to VICE's built-in monitor that
make it usable as a backend for an external app — anything from a CI
test harness to a screen-reading game-bot. Both commands are exposed in
the interactive text monitor (for debugging) and as opcodes in the
binary monitor protocol (for automation).

| Command         | What it does                                              | When to use                                                   |
|-----------------|-----------------------------------------------------------|---------------------------------------------------------------|
| `keymatrix`     | Inject keypresses straight into the C64 keyboard matrix.  | Driving any C64 software, including games and demos that scan CIA1 directly and don't go through the KERNAL.|
| `screenscrape`  | Emit screen RAM + color RAM + active charset, with metadata about VIC-II state. | Reading what's on the C64 display from your application without OCR / image processing. |
| `DRIVE_ATTACH` (binmon `0x78`) | Attach or detach a disk image at runtime. Empty path detaches; non-empty path attaches (replacing whatever was previously on that drive). | Forcing the host-side .d64 file to be flushed after a guest-side save, or hot-swapping disks between automated test runs. |

The fastest path is the binary monitor: open one TCP socket, send a
3-byte command header + body, get a structured binary response. No text
parsing.

### Quick start: launching VICE in driver-friendly mode

```
x64sc -default \
      -warp \
      -binarymonitor -binarymonitoraddress ip4://127.0.0.1:6502 \
      -silent
```

- `-warp` runs as fast as the host can manage. Drop it for realtime.
- `-binarymonitor -binarymonitoraddress …` opens a TCP socket on
  `127.0.0.1:6502` for binmon clients.
- The text monitor on the same emulator is also reachable: add
  `-remotemonitor -remotemonitoraddress ip4://127.0.0.1:6510` if you
  want it.
- For a headless build (no GUI), reconfigure with
  `./configure --with-alsa --enable-headlessui` and rebuild. See the
  Docker section below for a one-shot recipe.

After connecting, send the `EXIT` opcode (`0xaa`) once so the CPU
starts running; binmon halts the CPU when a client connects.

---

### `keymatrix` — keyboard injection at the matrix layer

VICE already had `keybuf`, which writes ASCII into the KERNAL keyboard
buffer at `$0277`. That works for BASIC and any program that calls
KERNAL `GETIN` / `CHRIN`, and it fails for action games, copy-protected
loaders, music demos and anything else that polls `$DC00`/`$DC01`
directly.

`keymatrix` sets bits in the same `keyarr[]` array the CIA1 reads, so
every C64 program sees the press regardless of how it scans. The
default `tap` mode does **not** time-based-guess how long to hold a
key: a small hook in the CIA1 read paths counts how many `$DC00` /
`$DC01` reads actually sampled an injected bit, and the key releases
the moment the program is observed reading the matrix.

#### Sub-commands (text monitor)

```
keymatrix tap     <key> [<key> ...] [for <frames>]
keymatrix press   <key> [<key> ...]
keymatrix release [<key> [<key> ...]]
keymatrix poke    <row> <col> <0|1>
keymatrix show
keymatrix names
```

- `tap <key>...` — set the keys (chord), release on the **first**
  CIA1 read that sampled an injected bit, or after a 60-frame timeout.
- `tap <key>... for <N>` — fixed-duration hold, ignores observation.
  Use for sustained presses (held game-control keys, etc.).
- `press <key>...` — sticky chord; clear with `release`.
- `release` — clear listed keys, or every matrix bit if no keys.
- `poke <r> <c> <v>` — raw bit poke. Works on any keyarr-using
  machine; the symbolic name table is C64-specific.
- `show` — pretty-print live `keyarr[]` and the last/current tap report.
- `names` — list every recognised C64 key name.

A key is either a symbolic name (`A`, `F1`, `LSHIFT`, `RUNSTOP`,
`RESTORE`, `CBM`, `RETURN`, `SPACE`, …; case-insensitive — see
`keymatrix names` for the full table) or a `<row>,<col>` pair like
`7,7`. Combine for chords: `keymatrix tap lshift a`.

#### Verification model

Each tap reports back:

- `release_reason` — `observed` (program polled and saw the bit;
  this is the common case), `timeout` (no qualifying CIA1 read in the
  60-frame window — try a different key or use `for N`), `manual`
  (cleared explicitly or superseded by another tap), or `none` (no
  completed tap yet).
- `cia1_reads_total` — every read of `$DC00` / `$DC01` while
  injection was active.
- `cia1_reads_sampling` — the subset of those that actually carried
  an injected bit (joystick-port reads of the same registers don't
  count).

#### Binary protocol opcodes

| Opcode | Name             | Body                                                                              | Response body                                                                 |
|--------|------------------|-----------------------------------------------------------------------------------|-------------------------------------------------------------------------------|
| `0x74` | KEYMATRIX_SET    | `count:u8`, then `count × {row:i8, col:i8, value:u8}`                             | empty                                                                         |
| `0x75` | KEYMATRIX_TAP    | `mode:u8` (0=observed, 1=fixed), `frames:u16`, `count:u8`, then `count × {row:i8, col:i8}` | empty                                                                         |
| `0x76` | KEYMATRIX_GET    | empty                                                                              | 24 bytes — see [Binmon response layout](#binmon-response-layout) below.       |

All multi-byte fields are little-endian. `row` is `i8` so the negative
custom-key sentinels (`-3` for RESTORE etc.) round-trip cleanly.

##### Binmon response layout

`KEYMATRIX_GET` returns 24 bytes:

| Offset | Type   | Field                                                                          |
|--------|--------|--------------------------------------------------------------------------------|
| 0–7    | u8×8   | `keyarr[0..7]` — live matrix rows                                              |
| 8      | u8     | custom-key bitmap: bit0=RESTORE1, bit1=RESTORE2, bit2=CAPS, bit3=4080          |
| 9–11   | —      | padding (zero)                                                                 |
| 12–15  | u32    | `cia1_reads_total`                                                             |
| 16–19  | u32    | `cia1_reads_sampling`                                                          |
| 20     | u8     | `release_reason` (0=none, 1=observed, 2=timeout, 3=manual)                     |
| 21     | u8     | `n_keys` (active or last tap)                                                  |
| 22–23  | u16    | `frames_until_timeout` (active tap only, else 0)                               |

#### Worked example

A trivial test program polls CIA1 directly without KERNAL or IRQ:

```asm
        * = $c000
        sei                ; no IRQ-driven SCNKEY interference
        lda #$00
        sta $dc00          ; drive all columns low
loop    lda $dc01          ; read row state
        sta $0400          ; latch to top-left of screen
        jmp loop
```

`SYS 49152` to run it, then in the monitor:

```
(C:$e5cf) keymatrix tap a
keymatrix: tap 1 key, mode=tap-observed, max 60 frames
(C:$e5cf) x

(C:$e5cf) keymatrix show
... live matrix ...
last tap: 1 key (tap-observed)
  A (1,2)
  released after 0 frames; reason: observed
  cia1 reads: 117 total, 117 sampled injected bits
```

For comparison, `keybuf "a"` then `x` will leave `$0400` at `$ff` (the
matrix is untouched) even though BASIC input still works. That is the
whole reason `keymatrix` exists.

---

### `screenscrape` — read the screen and the active charset

`screenscrape` takes a snapshot of:

- the current VIC-II video mode (text / multicolor text / hires bitmap
  / multicolor bitmap / extended colour text);
- the screen RAM at the live VIC pointer (D018 + CIA2 PA bank);
- the color RAM (`$D800-$DBE7`);
- the 2 KiB character set the VIC is currently reading from — *with
  metadata identifying which built-in ROM charset is active or that a
  custom RAM charset is in use*, so a client can map screencodes to
  glyphs (or to ASCII) without parsing the charset bytes;
- border / background colours;
- the raw `$D011`, `$D016`, `$D018` register values for clients that
  need finer detail.

The text monitor command renders the screen as a 40×25 ASCII grid
(plus a state header). The binmon `SCREEN_GET` opcode returns the
exact same data in a single fixed-size 4072-byte response — one round
trip per frame, no parsing.

#### Text monitor

```
(C:$e5cf) screenscrape
screen: vic_mode=normal-text rows=25 cols=40 vic_bank=0
        screen=$0400 charset=$1000 bitmap=$0000
        charset_kind=ROM upper/graphics   payload_bytes=4048
        D011=$1b D016=$08 D018=$14
        border=14 bg0=6 bg1=1 bg2=2 bg3=3

        +----------------------------------------+
  r00:  |                                        |
  r01:  |    **** COMMODORE 64 BASIC V2 ****     |
  r02:  |                                        |
  r03:  | 64K RAM SYSTEM  38911 BASIC BYTES FREE |
  r04:  |                                        |
  r05:  |READY.                                  |
  r06:  |.                                       |
  ...
        +----------------------------------------+
(approximate ASCII; '.' = unmappable. Use 'screen raw' for hex.)
```

Pass `screenscrape raw` to dump screen RAM as hex instead of ASCII.

#### Binary protocol opcode

| Opcode | Name        | Body  | Response body                                |
|--------|-------------|-------|----------------------------------------------|
| `0x77` | SCREEN_GET  | empty | 4072 bytes — fixed layout (see below).       |

---

### `DRIVE_ATTACH` — runtime image attach / detach (binmon only)

Standard binmon `RESOURCE_SET` (`0x52`) cannot reach the disk-image
slot at all: VICE wires the cmdline `-8 image.d64` directly through
`file_system_attach_disk()`, not through a resource. And the
RESOURCE_SET handler refuses zero-length values, which would otherwise
be the natural "detach" syntax.

`DRIVE_ATTACH` (`0x78`) is a thin shim over `file_system_attach_disk()`
and `file_system_detach_disk()`:

| Opcode | Name          | Body                                                                  | Response body |
|--------|---------------|-----------------------------------------------------------------------|---------------|
| `0x78` | DRIVE_ATTACH  | `unit:u8` (8..11), `drive:u8` (0/1), `path_len:u8`, `path:u8 × path_len` | empty         |

`path_len == 0` ⇒ detach the slot (same as `attach 0` in the text monitor).

`path_len > 0` ⇒ attach the host-side image at `path`. VICE detaches
any previous image first, which closes open files and writes the BAM
back to disk — so a same-path "attach" is also the canonical way to
**force a flush** of pending writes after a guest-side save without
disturbing the running CPU.

Use `path_len > 0` re-attach as the integration hook for an external
analyser: between the implicit detach and the re-attach the host
file is consistent and not being written to by anyone, so a
`shutil.copy()` or `c1541 -list` taken inside that window is safe.

Returns the same 4072-byte blob whatever the video mode. The header
tells the client what's in the payload bytes; in bitmap modes the
trailing 2 KiB labelled "charset" is actually the lower half of the
8 KiB bitmap region (clients that need full bitmap should fetch the
upper 6 KiB via standard `MEM_GET`).

##### SCREEN_GET response layout (4072 bytes)

Header (24 bytes):

| Offset | Type   | Field                                                                                  |
|--------|--------|----------------------------------------------------------------------------------------|
| 0      | u8     | `vic_mode`: 0=normal-text, 1=mc-text, 2=hires-bitmap, 3=mc-bitmap, 4=ext-text, 5–7=illegal |
| 1      | u8     | `rows` (always 25 for C64)                                                             |
| 2      | u8     | `cols` (always 40)                                                                     |
| 3      | u8     | `charset_kind`: 0=ROM upper/graphics, 1=ROM upper/lowercase, 2=custom RAM charset      |
| 4      | u8     | `vic_bank` (0..3 — 0 = `$0000-$3FFF`, 3 = `$C000-$FFFF`)                               |
| 5      | u8     | `border_color` (low nibble of `$D020`)                                                 |
| 6–9    | u8×4   | `bg_color[0..3]` (low nibbles of `$D021`–`$D024`)                                      |
| 10     | u8     | raw `$D011`                                                                            |
| 11     | u8     | raw `$D016`                                                                            |
| 12     | u8     | raw `$D018`                                                                            |
| 13     | u8     | reserved (zero)                                                                        |
| 14–15  | u16 LE | `screen_addr` (CPU-equivalent address of screen RAM)                                   |
| 16–17  | u16 LE | `charset_addr` (VIC-bank address of charset; for ROM, the offset within `$1000-$1FFF`) |
| 18–19  | u16 LE | `bitmap_addr` (VIC-bank address of bitmap base; 0 in text modes)                       |
| 20–23  | u32 LE | `payload_len` (always 4048: 1000 + 1000 + 2048)                                        |

Payload (4048 bytes):

| Offset (from body start) | Bytes | Field                                                |
|--------------------------|-------|------------------------------------------------------|
| 24                       | 1000  | screen RAM (screen codes in text modes)              |
| 1024                     | 1000  | color RAM (low nibble = foreground colour)           |
| 2024                     | 2048  | character set (256 chars × 8 rows) — from chargen ROM if `charset_kind` is 0/1, else from RAM at `charset_addr` |

A character at `(row, col)` uses screencode `screen[row*40 + col]`,
foreground colour `color[row*40 + col]`, glyph
`charset[screencode * 8 .. screencode * 8 + 7]` (each row is 8
horizontal pixels, MSB = leftmost). Background is `bg_color[0]` in
normal text mode.

---

### Driving from your application

#### Binary monitor — request and response framing

Every binmon request has this header:

```
byte 0 : 0x02                          (STX)
byte 1 : 0x02                          (API version)
bytes 2..5 : body length (u32 LE)
bytes 6..9 : request id (u32 LE; you choose, server echoes back)
byte 10  : command opcode
bytes 11..(10+body length) : body
```

Every response (whether solicited or unsolicited stop/resume events):

```
byte 0 : 0x02                          (STX)
byte 1 : 0x02                          (API version)
bytes 2..5 : body length (u32 LE)
byte 6 : response opcode
byte 7 : error code (0x00 = OK, see below)
bytes 8..11 : echoed request id (or 0 for unsolicited events)
bytes 12..(11+body length) : body
```

Standard error codes worth handling: `0x00` OK, `0x80` invalid length,
`0x81` invalid parameter, `0x8d` invalid api version, `0x8f`
command failure (e.g. `SCREEN_GET` on a non-C64 build).

Unsolicited responses you may see at any time:

- `0x62` STOPPED — the CPU has halted (binmon connect, breakpoint).
- `0x63` RESUMED — the CPU is running again.
- `0x61` JAM — the CPU JAM'd.

A robust client matches `request_id` and ignores responses whose IDs
it didn't send.

#### Python sketch

```python
import socket, struct

s = socket.socket(); s.connect(("127.0.0.1", 6502))

def cmd(op, body=b"", req=1):
    s.send(bytes([0x02, 0x02])
           + struct.pack("<II", len(body), req)
           + bytes([op]) + body)

def read_one():
    h = s.recv(12)
    _stx, _ver, blen, op, err, req = struct.unpack("<BBIBBI", h)
    body = b""
    while len(body) < blen:
        body += s.recv(blen - len(body))
    return op, err, req, body

def call(op, body=b"", req=1):
    cmd(op, body, req)
    while True:
        r = read_one()
        if r[2] == req: return r        # match the request id

# Drain the initial STOPPED then resume the CPU:
s.settimeout(0.2)
try:
    while True: read_one()
except socket.timeout: pass
s.settimeout(5)
call(0xaa)                              # EXIT (resume CPU)

# Tap "A" with observation-based release:
call(0x75, struct.pack("<BHB", 0, 60, 1) + bytes([1, 2]))

# Read the screen (4072 bytes back):
op, err, req, body = call(0x77)
assert op == 0x77 and err == 0
header, payload = body[:24], body[24:]
vic_mode = header[0]
rows, cols = header[1], header[2]
screen   = payload[: rows*cols]
color    = payload[rows*cols : 2*rows*cols]
charset  = payload[2*rows*cols :]
```

#### Text monitor over TCP

For interactive use you can also enable the text monitor on a socket
(`-remotemonitor -remotemonitoraddress ip4://127.0.0.1:6510`) and
`telnet`/`nc` into it. All of the commands above type the same:

```
$ nc 127.0.0.1 6510
(C:$e5cf) keymatrix tap lshift a
(C:$e5cf) screenscrape
(C:$e5cf) x
```

The text monitor is friendlier for debugging and inspection; the
binary monitor is what your app should use in production because it
never has to parse human-readable strings.

---

### Running headlessly with Docker

A `Dockerfile` is provided at the repo root that builds VICE with
`--enable-headlessui` (no GTK / SDL window) and packages the resulting
`x64sc` binary together with the C64 ROMs into a small runtime image.
The container exposes the binary monitor on TCP `6502` by default;
your application connects to that port and drives the emulator using
the protocol described above.

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
