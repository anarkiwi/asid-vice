/*
 * mon_screen.c - Monitor screen-scrape command (machine-agnostic shell).
 *
 * The actual reading of VIC-II state, screen RAM, color RAM, and the
 * character set lives in the C64-specific provider (src/c64/c64screen.c)
 * which registers itself at machine init via mon_screen_register_provider().
 * This file just calls through and renders the result for the text monitor.
 *
 * See mon_screen.h for the wire format.
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "mon_screen.h"
#include "monitor.h"
#include "montypes.h"
#include "types.h"

static mon_screen_provider_t s_provider = NULL;

void mon_screen_register_provider(mon_screen_provider_t provider)
{
    s_provider = provider;
}

int mon_screen_binmon_get(uint8_t *out, uint32_t *out_length)
{
    uint32_t want = MON_SCREEN_BINMON_GET_RESPONSE_SIZE;
    if (out == NULL || out_length == NULL) {
        return -1;
    }
    if (*out_length < want) {
        return -1;
    }
    if (s_provider == NULL) {
        return -1;                    /* not supported on this machine */
    }
    return s_provider(out, *out_length, out_length);
}

/* Map a C64 screencode (upper-graphics charset) to the closest printable
   ASCII char so the text-mode dump is readable. Reverse-video bit is
   ignored. Anything not in the small mappable range becomes '.' so the
   grid stays aligned. */
static char screencode_to_ascii(uint8_t sc)
{
    sc &= 0x7f;                       /* strip reverse-video */
    if (sc == 0x00) {
        return '@';
    }
    if (sc <= 0x1a) {
        return (char)('A' + sc - 1);
    }
    if (sc == 0x1b) { return '['; }
    if (sc == 0x1c) { return '\\'; }  /* pound, no good ASCII */
    if (sc == 0x1d) { return ']'; }
    if (sc == 0x1e) { return '^'; }   /* up-arrow */
    if (sc == 0x1f) { return '_'; }   /* left-arrow */
    if (sc >= 0x20 && sc <= 0x3f) {
        return (char)sc;              /* ASCII-compatible punctuation */
    }
    return '.';
}

static const char *vic_mode_name(uint8_t m)
{
    switch (m) {
        case 0: return "normal-text";
        case 1: return "multicolor-text";
        case 2: return "hires-bitmap";
        case 3: return "multicolor-bitmap";
        case 4: return "extended-text";
        case 5: return "illegal-text";
        case 6: return "illegal-bitmap-1";
        case 7: return "illegal-bitmap-2";
        default: return "?";
    }
}

static const char *charset_kind_name(uint8_t k)
{
    switch (k) {
        case MON_SCREEN_CHARSET_ROM_UPPER_GFX:   return "ROM upper/graphics";
        case MON_SCREEN_CHARSET_ROM_UPPER_LOWER: return "ROM upper/lowercase";
        case MON_SCREEN_CHARSET_RAM:             return "custom (RAM)";
        default: return "?";
    }
}

static void show_state_header(const uint8_t *hdr)
{
    uint16_t screen_addr  = (uint16_t)hdr[14] | ((uint16_t)hdr[15] << 8);
    uint16_t charset_addr = (uint16_t)hdr[16] | ((uint16_t)hdr[17] << 8);
    uint16_t bitmap_addr  = (uint16_t)hdr[18] | ((uint16_t)hdr[19] << 8);
    uint32_t payload_len  = (uint32_t)hdr[20] | ((uint32_t)hdr[21] << 8) |
                            ((uint32_t)hdr[22] << 16) | ((uint32_t)hdr[23] << 24);
    mon_out("screen: vic_mode=%s rows=%u cols=%u vic_bank=%u\n",
            vic_mode_name(hdr[0]), hdr[1], hdr[2], hdr[4]);
    mon_out("        screen=$%04x charset=$%04x bitmap=$%04x\n",
            screen_addr, charset_addr, bitmap_addr);
    mon_out("        charset_kind=%s   payload_bytes=%u\n",
            charset_kind_name(hdr[3]), (unsigned)payload_len);
    mon_out("        D011=$%02x D016=$%02x D018=$%02x\n",
            hdr[10], hdr[11], hdr[12]);
    mon_out("        border=%u bg0=%u bg1=%u bg2=%u bg3=%u\n",
            hdr[5], hdr[6], hdr[7], hdr[8], hdr[9]);
}

void mon_screen_show(const char *args)
{
    uint8_t buf[MON_SCREEN_BINMON_GET_RESPONSE_SIZE];
    uint32_t got = sizeof(buf);
    int rc;
    int rows, cols, r, c;
    const uint8_t *screen;
    int raw = (args != NULL && *args != '\0');

    if (s_provider == NULL) {
        mon_out("screen: not supported on this machine "
                "(provider registered only by C64 builds)\n");
        return;
    }

    rc = s_provider(buf, sizeof(buf), &got);
    if (rc != 0 || got < MON_SCREEN_HEADER_BYTES) {
        mon_out("screen: provider returned error %d (got %u bytes)\n",
                rc, (unsigned)got);
        return;
    }

    show_state_header(buf);

    rows = buf[1];
    cols = buf[2];
    if (rows <= 0 || cols <= 0 ||
        (uint32_t)(MON_SCREEN_HEADER_BYTES + rows * cols) > got) {
        mon_out("screen: header reports rows=%d cols=%d which doesn't fit "
                "in %u-byte response\n", rows, cols, (unsigned)got);
        return;
    }

    screen = buf + MON_SCREEN_HEADER_BYTES;

    if (raw) {
        mon_out("\nscreen RAM hex (first %d bytes):\n", rows * cols);
        for (r = 0; r < rows * cols; r++) {
            if ((r % 16) == 0) {
                mon_out("\n  +%03x:", r);
            }
            mon_out(" %02x", screen[r]);
        }
        mon_out("\n");
    } else {
        mon_out("\n        +");
        for (c = 0; c < cols; c++) {
            mon_out("-");
        }
        mon_out("+\n");
        for (r = 0; r < rows; r++) {
            mon_out("  r%02d:  |", r);
            for (c = 0; c < cols; c++) {
                mon_out("%c", screencode_to_ascii(screen[r * cols + c]));
            }
            mon_out("|\n");
        }
        mon_out("        +");
        for (c = 0; c < cols; c++) {
            mon_out("-");
        }
        mon_out("+\n");
        mon_out("(approximate ASCII; '.' = unmappable. Use 'screen raw' for hex.)\n");
    }
}
