/*
 * soundasid.c - Implementation of the asid protocol midi sound device.
 *
 * Example usage:
 *
 *    x64 -sounddev asid -soundarg 1
 *
 * Originally written by aTc <aTc@k-n-p.org>, updated by josh@vandervecken.com.
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "debug.h"
#include "log.h"

#include "maincpu.h"
#include "interrupt.h"

#include "sound.h"
#include "types.h"

#include <alsa/asoundlib.h>

#define SYSEX_START 0xf0
#define SYSEX_MAN_ID 0x2d
#define SYSEX_STOP 0xf7

#define ASID_START 0x4c
#define ASID_STOP 0x4d
#define ASID_UPDATE 0x4e

const uint8_t asid_start[] = {SYSEX_START, SYSEX_MAN_ID, ASID_START, SYSEX_STOP};
const uint8_t asid_stop[] = {SYSEX_START, SYSEX_MAN_ID, ASID_STOP, SYSEX_STOP};
const uint8_t asid_update[] = {SYSEX_START, SYSEX_MAN_ID, ASID_UPDATE};
const uint8_t max_sid_reg = 24;
/* IDs 25-27 not implemented. They are rumoured to make additional updates to
   registers 4, 11, and 18, but asidxp.exe doesn't seem to use them. */
const uint8_t regmap[] = {
     0,  1,  2,  3,  5,  6,  7,  8,
     9, 10, 12, 13, 14, 15, 16, 17,
    19, 20, 21, 22, 23, 24,  4, 11, 18};
const uint8_t regmask[] = {
    //0    1    2    3    4    5    6    7
    255, 255, 255,  15, 255, 255, 255, 255,
    //8    9   10   11   12   13   14   15
    255, 255,  15, 255, 255, 255, 255, 255,
    //16  17   18   19   20   21   22   23  24
    255,  15, 255, 255, 255,   7, 255, 255, 255};

/* update preamble, mask/MSB, register map, stop. */
static uint8_t asid_buffer[sizeof(asid_update) + 8 + sizeof(regmap) + 1];
static uint8_t sid_register[sizeof(regmap)];
static bool sid_modified[sizeof(regmap)];
static bool sid_modified_flag = false;
static CLOCK last_irq = 0;
snd_rawmidi_t *handle_out = 0;

/* TODO: refactor libmididrv API for cross platform support. */
static int _initialize_midi(int asid_port)
{
    char device_out[8];
    memset(&device_out, 0, sizeof(device_out));
    snprintf(&device_out, sizeof(device_out), "hw:%d", asid_port);
    log_message(LOG_DEFAULT, "opening asid device %s", device_out);
    int err = snd_rawmidi_open(NULL, &handle_out, device_out, 0);
    if (err) {
        log_message(LOG_DEFAULT, "snd_rawmidi_open %s failed: %d\n", device_out, err);
        return err;
    }
    return 0;
}

static int _close_port(void)
{
    if (handle_out) {
        snd_rawmidi_drain(handle_out); 
        snd_rawmidi_close(handle_out);
    }
    return 0;
}

static int _send_message(const uint8_t *message, uint8_t message_len)
{
    if (handle_out) {
        snd_rawmidi_write(handle_out, message, message_len);
        snd_rawmidi_drain(handle_out);
    }
    return 0;
}

static int asid_write_()
{
    uint8_t i;
    uint8_t mapped_reg;
    uint32_t mask = 0;
    uint32_t msb = 0;
    uint8_t p = sizeof(asid_update) - 1;

    if (!sid_modified_flag) {
        return 0;
    }

    /* set bits in mask for each register that has been written to
       write last bit of each register into msb. */
    //log_message(LOG_DEFAULT, "begin");
    for(i = 0; i < sizeof(regmap); ++i)
    {
        mapped_reg = regmap[i];
        if (sid_modified[mapped_reg])
        {
            mask = mask | (1<<i);
        }
        if (sid_register[mapped_reg] > 0x7f)
        {
            msb = msb | (1<<i);
        }
    }
    asid_buffer[++p] = mask & 0x7f;
    asid_buffer[++p] = (mask>>7) & 0x7f;
    asid_buffer[++p] = (mask>>14) & 0x7f;
    asid_buffer[++p] = (mask>>21) & 0x7f;
    asid_buffer[++p] = msb & 0x7f;
    asid_buffer[++p] = (msb>>7) & 0x7f;
    asid_buffer[++p] = (msb>>14) & 0x7f;
    asid_buffer[++p] = (msb>>21) & 0x7f;
    for(i = 0; i < sizeof(regmap); ++i)
    {
        mapped_reg = regmap[i];
        if (sid_modified[mapped_reg])
        {
            asid_buffer[++p] = sid_register[mapped_reg] & 0x7f;
            //log_message(LOG_DEFAULT, "reg %u -> %u", mapped_reg, sid_register[mapped_reg]);
        }
    }
    //log_message(LOG_DEFAULT, "end");
    asid_buffer[++p] = SYSEX_STOP;
    memset(sid_modified, false, sizeof(sid_modified));
    sid_modified_flag = false;
    return _send_message(asid_buffer, p + 1);
}

static int asid_init(const char *param, int *speed,
                     int *fragsize, int *fragnr, int *channels)
{
    int asid_port;
    memcpy(asid_buffer, asid_update, sizeof(asid_update));

    /* No stereo capability. */
    *channels = 1;

    if (!param) {
      log_message(LOG_DEFAULT, "-soundarg <n> is required");
      return -1;
    }

    asid_port = atoi(param);

    if (_initialize_midi(asid_port)) {
        log_message(LOG_DEFAULT, "failed to initialize MIDI");
        return -1;
    }
    if (_send_message(asid_start, sizeof(asid_start))) {
        return -1;
    }
    memset(sid_register, 0, sizeof(sid_register));
    memset(sid_modified, true, sizeof(sid_modified));
    sid_modified_flag = true;
    return asid_write_();
}

static void _set_reg(uint8_t reg, uint8_t byte) {
    byte = regmask[reg] & byte;
    if (sid_register[reg] == byte) {
        return;
    }
    // flush on change to control register.
    if ((reg == 4 || reg == 11 || reg == 18) && sid_modified[reg]) {
        // log_message(LOG_DEFAULT, "expedite control");
        asid_write_();
    }
    sid_register[reg] = byte;
    sid_modified[reg] = true;
    sid_modified_flag = true;
}

static int asid_dump(uint16_t addr, uint8_t byte, CLOCK clks)
{
    // Flush changes from previous IRQ.
    if (maincpu_int_status->irq_clk != last_irq) {
        last_irq = maincpu_int_status->irq_clk;
        asid_write_();
    }

    uint8_t reg = addr & 0x1f;
    if (reg > max_sid_reg) {
        return 0;
    }

    _set_reg(reg, byte);

    // Many playroutines write to all registers in sequence, so flush on the last register.
    if (reg == 24) {
        asid_write_();
    }
    return 0;
}

static int asid_write(int16_t *pbuf, size_t nr) {
    return 0;
}

static void asid_close(void)
{
    _send_message(asid_stop, sizeof(asid_stop));
    _close_port();
}

static int asid_flush(char *state)
{
    return 0;
}

static sound_device_t asid_device =
{
    "asid",
    asid_init,
    asid_write,
    asid_dump,
    NULL,
    asid_flush,
    NULL,
    asid_close,
    NULL,
    NULL,
    0,
    1,
    false
};

int sound_init_asid_device(void)
{
    return sound_register_device(&asid_device);
}
