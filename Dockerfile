# asid-vice headless image.
#
# Multi-stage build. The "builder" stage compiles VICE with --enable-headlessui
# (no GTK/SDL window) and `make install`s the resulting binaries plus the C64
# ROMs into /opt/vice. The "runtime" stage is a slim Debian image with only
# the runtime libraries x64sc actually needs, plus that /opt/vice tree.
#
# The container's CMD starts x64sc with the binary monitor listening on TCP
# 6502 (bound to 0.0.0.0 so connections from outside the container work).
# An external application can then connect and drive the emulator using the
# `keymatrix` (input) and `screenscrape` (output) commands documented in
# README.md.
#
# Build:
#   docker build -t asid-vice .
#
# Run (default: just listen on binmon):
#   docker run --rm -p 6502:6502 asid-vice
#
# Run with a disk image autostarted:
#   docker run --rm -p 6502:6502 \
#       -v "$PWD/Commando.d64:/work/disk.d64:ro" \
#       asid-vice -autostart /work/disk.d64
#
# The PRG/D64 path is appended to the default CMD, so any extra args you pass
# to `docker run` are passed straight to x64sc.

# ---------------------------------------------------------------------------
# Stage 1: build
# ---------------------------------------------------------------------------
FROM debian:bookworm-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Build deps. SDL/GLEW/GTK are *not* needed for --enable-headlessui, but
# we keep libpng (for screenshot subsystem) and libasound (for the audio
# pipeline; harmless even when no device is attached).
RUN apt-get update && apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        build-essential \
        byacc \
        ca-certificates \
        dos2unix \
        flex \
        libasound2-dev \
        libcurl4-openssl-dev \
        libevdev-dev \
        libglib2.0-dev \
        libpng-dev \
        libpulse-dev \
        pkg-config \
        texinfo \
        xa65 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

# `--prefix=/opt/vice` so the runtime stage can copy a single self-contained
# tree. Headless UI, no docs, no real-device (OpenCBM), no SDL/GTK.
RUN ./autogen.sh \
    && ./configure \
        --prefix=/opt/vice \
        --enable-headlessui \
        --with-alsa \
        --disable-html-docs \
        --disable-realdevice \
        --disable-rs232 \
    && make -j"$(nproc)" \
    && make install \
    && (strip /opt/vice/bin/x64sc || true)

# ---------------------------------------------------------------------------
# Stage 2: runtime
# ---------------------------------------------------------------------------
FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Just the shared libs x64sc actually links to (`ldd` of the headless binary).
RUN apt-get update && apt-get install -y --no-install-recommends \
        libasound2 \
        libcurl4 \
        libglib2.0-0 \
        libpng16-16 \
        libpulse0 \
        libusb-1.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Copy the installed VICE tree (binary + ROMs + data) from the builder.
COPY --from=builder /opt/vice /opt/vice

ENV PATH=/opt/vice/bin:$PATH

# Working directory for mounting disk/prg/snapshot images.
WORKDIR /work

# Binary monitor port. Use `-p 6502:6502` on `docker run` to expose it.
EXPOSE 6502

# Default invocation: headless x64sc with the binary monitor bound to
# 0.0.0.0 so external clients can connect. Any additional `docker run`
# arguments are appended (e.g. `-autostart /work/foo.d64`).
ENTRYPOINT ["x64sc"]
CMD [ \
    "-default", \
    "-warp", \
    "-binarymonitor", \
    "-binarymonitoraddress", "ip4://0.0.0.0:6502", \
    "-sounddev", "dummy", \
    "-silent" \
]
