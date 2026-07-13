#!/bin/sh
# Entrypoint for the X11 (GTK3) asid-vice image.
#
# The GTK3 UI needs an X server. If the caller supplies one (DISPLAY set,
# e.g. the host X socket bind-mounted in), use it as-is. Otherwise start a
# virtual framebuffer (Xvfb) so the emulator runs in a bare container with no
# host X server, then exec the requested command against it.
set -e

if [ -z "$DISPLAY" ]; then
    export DISPLAY=:99
    Xvfb "$DISPLAY" -screen 0 1280x1024x24 -nolisten tcp >/dev/null 2>&1 &
    # Wait for the X socket to appear before starting the GTK client.
    i=0
    while [ ! -e /tmp/.X11-unix/X99 ] && [ "$i" -lt 50 ]; do
        i=$((i + 1))
        sleep 0.1
    done
fi

exec "$@"
