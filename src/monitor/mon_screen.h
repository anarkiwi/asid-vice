/*
 * mon_screen.h - Monitor screen-scrape command. Emits the C64 screen RAM
 *                (wherever the VIC-II is currently pointing it), the color
 *                RAM, and the active character set so an external app can
 *                reconstruct what's on the display without having to know
 *                what charset is in use.
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 */

#ifndef VICE_MON_SCREEN_H
#define VICE_MON_SCREEN_H

#include "types.h"

/* Wire-format constants. The response is a fixed 4072 bytes for any C64
   text-mode frame; the same size is used for bitmap mode (the trailing
   2048 "charset" bytes contain the lower 2 KiB of bitmap memory in that
   case — clients should consult `vic_mode` in the header). */
#define MON_SCREEN_HEADER_BYTES   24
#define MON_SCREEN_TEXT_COLS      40
#define MON_SCREEN_TEXT_ROWS      25
#define MON_SCREEN_TEXT_CELLS     (MON_SCREEN_TEXT_COLS * MON_SCREEN_TEXT_ROWS)
#define MON_SCREEN_CHARSET_BYTES  2048
#define MON_SCREEN_BINMON_GET_RESPONSE_SIZE \
    (MON_SCREEN_HEADER_BYTES + MON_SCREEN_TEXT_CELLS * 2 + MON_SCREEN_CHARSET_BYTES)

/* charset_kind values. */
#define MON_SCREEN_CHARSET_ROM_UPPER_GFX    0
#define MON_SCREEN_CHARSET_ROM_UPPER_LOWER  1
#define MON_SCREEN_CHARSET_RAM              2

/* Per-machine provider callback. Called by mon_screen_binmon_get() and
   mon_screen_show(). Fills `buf` with the wire-format response (header +
   screen + color + charset). On success returns 0 and sets *out_len to the
   number of bytes written. On failure (buffer too small or transient
   internal error) returns negative. */
typedef int (*mon_screen_provider_t)(uint8_t *buf, uint32_t buf_len,
                                     uint32_t *out_len);

/* Called once during machine init by C64-side code to plug the provider
   in. The monitor library is shared between every emulator binary and
   does not depend on any C64 symbol; the C64 build provides the body and
   registers it here. Other emulators leave this NULL and the screen
   command reports "not supported". */
void mon_screen_register_provider(mon_screen_provider_t provider);

/* Text-monitor handler. `args` may be:
   NULL/empty -> render a 40x25 ASCII grid plus a one-line state summary
   "raw"      -> dump the VIC-II registers and a hex view of screen RAM
*/
void mon_screen_show(const char *args);

/* Binary-monitor entry point. Writes a fixed-layout response into `out`
   (caller-allocated, MON_SCREEN_BINMON_GET_RESPONSE_SIZE bytes). On
   success returns 0 and sets *out_length to the number of bytes written.
   Returns negative if no provider is registered or the buffer is too
   small. */
int mon_screen_binmon_get(uint8_t *out, uint32_t *out_length);

#endif
