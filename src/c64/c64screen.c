/*
 * c64screen.c - C64-specific screen-state provider for the monitor's
 *               `screen` / SCREEN_GET commands.
 *
 * Reads VIC-II registers and the appropriate region of memory (screen RAM,
 * color RAM, character set or bitmap) and packs them into the wire format
 * defined in src/monitor/mon_screen.h. Registered with the monitor via
 * mon_screen_register_provider() at machine init time.
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 */

#include "vice.h"

#include <string.h>

#include "c64mem.h"
#include "mem.h"
#include "monitor/mon_screen.h"
#include "types.h"
#include "viciitypes.h"

/* The VIC-II globals come from libviciisc.a (included in x64sc). The
   struct layout — in particular `regs[]`, `vbank_phi1/2`, and the
   `vaddr_chargen_*` fields we use — matches src/viciisc/viciitypes.h
   thanks to the c64 dir's include path. */

static void put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* Decode video mode from D011 bits 5-6 + D016 bit 4. Same formula
   vicii_update_video_mode() uses. */
static uint8_t derive_vic_mode(void)
{
    return (uint8_t)(((vicii.regs[0x11] & 0x60) | (vicii.regs[0x16] & 0x10)) >> 4);
}

/* CIA2 PA bits 0-1 select which 16K bank the VIC sees. We read it
   indirectly via vicii.vbank_phi2 (already kept in sync with CIA2). */
static uint8_t derive_vic_bank(void)
{
    return (uint8_t)((vicii.vbank_phi2 >> 14) & 0x03);
}

static int c64screen_get_state(uint8_t *buf, uint32_t buf_len, uint32_t *out_len)
{
    uint8_t  vic_mode;
    uint8_t  vic_bank;
    uint16_t screen_addr;
    uint16_t charset_addr;
    uint16_t bitmap_addr;
    uint16_t charset_vic_addr;
    int      charset_in_chargen_window;
    uint8_t  charset_kind;
    uint8_t *screen_dst;
    uint8_t *color_dst;
    uint8_t *charset_dst;
    int      i;

    if (buf == NULL || out_len == NULL) {
        return -1;
    }
    if (buf_len < MON_SCREEN_BINMON_GET_RESPONSE_SIZE) {
        return -1;
    }

    vic_mode = derive_vic_mode();
    vic_bank = derive_vic_bank();

    /* Screen RAM base: D018 bits 4-7 give a 1 KiB offset within the VIC
       bank. The CPU sees screen RAM at the same physical address. */
    screen_addr = (uint16_t)(vicii.vbank_phi2 + ((vicii.regs[0x18] & 0xf0) << 6));

    /* Charset/bitmap base: D018 bits 1-3 give a 2 KiB offset within the
       VIC bank. */
    charset_vic_addr = (uint16_t)(((vicii.regs[0x18] & 0x0e) << 10) +
                                  vicii.vbank_phi1);
    charset_addr = charset_vic_addr;

    /* Bitmap base: D018 bit 3 selects the lower or upper 8 KiB of the VIC
       bank. (Only meaningful when in a bitmap mode.) */
    bitmap_addr = (uint16_t)(vicii.vbank_phi1 + ((vicii.regs[0x18] & 0x08) << 10));

    /* If the charset address falls inside the VIC's chargen-ROM window
       (typically $1000-$1FFF in bank 0 / $9000-$9FFF in bank 2) the VIC
       reads from chargen ROM, not RAM. */
    charset_in_chargen_window =
        ((charset_vic_addr & vicii.vaddr_chargen_mask_phi1) ==
         vicii.vaddr_chargen_value_phi1);

    if (charset_in_chargen_window) {
        /* Within chargen ROM: low 2 KiB is upper/graphics charset; high
           2 KiB is upper/lowercase charset. */
        if ((charset_vic_addr & 0x0800) == 0) {
            charset_kind = MON_SCREEN_CHARSET_ROM_UPPER_GFX;
        } else {
            charset_kind = MON_SCREEN_CHARSET_ROM_UPPER_LOWER;
        }
    } else {
        charset_kind = MON_SCREEN_CHARSET_RAM;
    }

    /* ---- Header (24 bytes) ---- */
    memset(buf, 0, MON_SCREEN_HEADER_BYTES);
    buf[0]  = vic_mode;
    buf[1]  = MON_SCREEN_TEXT_ROWS;
    buf[2]  = MON_SCREEN_TEXT_COLS;
    buf[3]  = charset_kind;
    buf[4]  = vic_bank;
    buf[5]  = (uint8_t)(vicii.regs[0x20] & 0x0f);  /* border colour */
    buf[6]  = (uint8_t)(vicii.regs[0x21] & 0x0f);  /* bg #0 */
    buf[7]  = (uint8_t)(vicii.regs[0x22] & 0x0f);  /* bg #1 */
    buf[8]  = (uint8_t)(vicii.regs[0x23] & 0x0f);  /* bg #2 */
    buf[9]  = (uint8_t)(vicii.regs[0x24] & 0x0f);  /* bg #3 */
    buf[10] = vicii.regs[0x11];
    buf[11] = vicii.regs[0x16];
    buf[12] = vicii.regs[0x18];
    /* buf[13] reserved */
    put_le16(&buf[14], screen_addr);
    put_le16(&buf[16], charset_addr);
    /* VICII_IS_BITMAP_MODE(x) == (x & 0x02) */
    put_le16(&buf[18], (vic_mode & 0x02) ? bitmap_addr : (uint16_t)0);
    put_le32(&buf[20],
             MON_SCREEN_TEXT_CELLS + MON_SCREEN_TEXT_CELLS + MON_SCREEN_CHARSET_BYTES);

    screen_dst  = buf + MON_SCREEN_HEADER_BYTES;
    color_dst   = screen_dst + MON_SCREEN_TEXT_CELLS;
    charset_dst = color_dst  + MON_SCREEN_TEXT_CELLS;

    /* ---- Screen RAM (1000 bytes) ----
       Always read from main RAM at the CPU-equivalent address. The VIC
       reads from its own bank's underlying RAM; mem_ram[] is that RAM. */
    for (i = 0; i < MON_SCREEN_TEXT_CELLS; i++) {
        screen_dst[i] = mem_ram[(screen_addr + i) & 0xffff];
    }

    /* ---- Color RAM (1000 bytes, low nibble = fg colour) ----
       colorram_read() returns (color | (vic noise & 0xf0)); mask away the
       noise so the wire data is deterministic. */
    for (i = 0; i < MON_SCREEN_TEXT_CELLS; i++) {
        color_dst[i] = (uint8_t)(colorram_read((uint16_t)(0xd800 + i)) & 0x0f);
    }

    /* ---- Character set (2048 bytes) ----
       From chargen ROM if D018 selects the chargen window; otherwise
       from main RAM at the equivalent CPU address. In bitmap mode the
       same 2 KiB read still happens; clients should look at vic_mode
       and bitmap_addr instead of treating it as a charset. */
    if (charset_in_chargen_window) {
        /* Chargen ROM is 4 KiB; the 2 KiB charset offset is bit 11 of
           the in-bank address. */
        uint16_t rom_base = (uint16_t)(charset_vic_addr & 0x0800);
        memcpy(charset_dst, mem_chargen_rom + rom_base,
               MON_SCREEN_CHARSET_BYTES);
    } else {
        for (i = 0; i < MON_SCREEN_CHARSET_BYTES; i++) {
            charset_dst[i] = mem_ram[(charset_addr + i) & 0xffff];
        }
    }

    *out_len = MON_SCREEN_BINMON_GET_RESPONSE_SIZE;
    return 0;
}

void c64screen_init(void)
{
    mon_screen_register_provider(c64screen_get_state);
}
