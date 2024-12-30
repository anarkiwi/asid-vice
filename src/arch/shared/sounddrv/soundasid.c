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

#include "sound.h"
#include "types.h"

#include <alsa/asoundlib.h>

#define SYSEX_START 0xf0
#define SYSEX_MAN_ID 0x2d
#define SYSEX_STOP 0xf7

#define ASID_START 0x4c
#define ASID_STOP 0x4d
#define ASID_UPDATE 0x4e

#define ALL_MIDI_PORTS -1
#define NO_PORT -1

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

static snd_seq_t *seq;
static int vport;
static snd_seq_port_subscribe_t *subscription;
static snd_midi_event_t *coder;

/* update preamble, mask/MSB, register map, stop. */
static uint8_t asid_buffer[sizeof(asid_update) + 8 + sizeof(regmap) + 1];
static uint8_t sid_register[sizeof(regmap)];
static bool sid_modified[sizeof(regmap)];
static bool sid_modified_flag = false;

/* TODO: refactor libmididrv API for cross platform support. */
static int _initialize_midi(void)
{
    int result = snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK);
    if (result < 0) {
        return -1;
    }

    snd_seq_set_client_name(seq, "asid");

    vport = NO_PORT;
    coder = 0;
    result = snd_midi_event_new(sizeof(asid_buffer), &coder);
    if (result < 0) {
        return -1;
    }
    snd_midi_event_init(coder);
    return 0;
}

static unsigned int _get_port_info(snd_seq_port_info_t *pinfo, unsigned int port_type, int port_number)
{
    snd_seq_client_info_t *cinfo;
    int client;
    int count = 0;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        client = snd_seq_client_info_get_client(cinfo);
        if (client) {
            snd_seq_port_info_set_client(pinfo, client);
            snd_seq_port_info_set_port(pinfo, -1);

            while (snd_seq_query_next_port(seq, pinfo) >= 0) {
                if ((snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {
                    if ((snd_seq_port_info_get_capability(pinfo) & port_type) == port_type) {
                        if (count == port_number) {
                            return 1;
                        }
                        ++count;
                    }
                }
            }
        }
    }

    if (port_number == ALL_MIDI_PORTS) {
        return count;
    }
    return 0;
}

static unsigned int _get_port_count(void)
{
    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);

    return _get_port_info(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, ALL_MIDI_PORTS);
}

static int _open_port(unsigned int port_number)
{
    unsigned int nports = _get_port_count();
    if (nports < 1) {
        return -1;
    }

    snd_seq_port_info_t *pinfo;
    snd_seq_port_info_alloca(&pinfo);

    if (_get_port_info(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, (int)port_number) == 0) {
        return -1;
    }

    snd_seq_addr_t sender, receiver;
    receiver.client = snd_seq_port_info_get_client(pinfo);
    receiver.port = snd_seq_port_info_get_port(pinfo);
    sender.client = snd_seq_client_id(seq);

    vport = snd_seq_create_simple_port(seq, "asid",
                                       SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                       SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    if (vport < 0) {
        return -1;
    }

    sender.port = vport;
    snd_seq_port_subscribe_malloc(&subscription);
    snd_seq_port_subscribe_set_sender(subscription, &sender);
    snd_seq_port_subscribe_set_dest(subscription, &receiver);
    snd_seq_port_subscribe_set_time_update(subscription, 1);
    snd_seq_port_subscribe_set_time_real(subscription, 1);

    if (snd_seq_subscribe_port(seq, subscription)) {
        return -1;
    }
    return 0;
}

static int _close_port(void)
{
    if (vport != NO_PORT) {
       snd_seq_unsubscribe_port(seq, subscription);
    }
    snd_seq_port_subscribe_free(subscription);
    if (vport != NO_PORT) {
        snd_seq_delete_port(seq, vport);
    }
    snd_midi_event_free(coder);
    snd_seq_close(seq);
    return 0;
}

static char *_get_port_name(unsigned int port_number, char *name_buffer, unsigned int max_name)
{
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    memset(name_buffer, 0, max_name);

    if (_get_port_info(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, (int)port_number)) {
        int cnum = snd_seq_port_info_get_client(pinfo);
        snd_seq_get_any_client_info(seq, cnum, cinfo);
        snprintf(name_buffer, max_name, "%s:%d", snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_port(pinfo));
    }

    return name_buffer;
}

static int _send_message(const uint8_t *message, uint8_t message_len)
{
    int result;
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, vport);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    result = snd_midi_event_encode(coder, message, message_len, &ev);

    if (result < (int)message_len) {
        return -1;
    }
    result = snd_seq_event_output(seq, &ev);
    if (result < 0) {
        return -1;
    }
    snd_seq_drain_output(seq);
    return 0;
}

static int asid_init(const char *param, int *speed,
                     int *fragsize, int *fragnr, int *channels)
{
    int i;
    int nports;
    int asid_port;
    char name_buffer[256];

    /* No stereo capability. */
    *channels = 1;
    memcpy(asid_buffer, asid_update, sizeof(asid_update));
    memset(sid_register, 0, sizeof(sid_register));
    memset(sid_modified, false, sizeof(sid_modified));
    sid_modified_flag = false;

    if (_initialize_midi()) {
        log_message(LOG_DEFAULT, "failed to initialize MIDI");
        return -1;
    }

    nports = _get_port_count();
    if (nports == 0) {
        log_message(LOG_DEFAULT, "No MIDI ports available");
        return -1;
    }

    log_message(LOG_DEFAULT,"asid open, available ports:");
    for (i = 0; i < nports; ++i)
        log_message(LOG_DEFAULT, "Port %d : %s", i, _get_port_name(i, name_buffer, sizeof(name_buffer)));

    if (!param) {
        log_message(LOG_DEFAULT, "-soundarg <n> is required");
        return -1;
    }

    asid_port = atoi(param);
    if (asid_port < 0 || asid_port > (nports - 1)) {
        log_message(LOG_DEFAULT, "invalid MIDI port in -soundarg");
        return -1;
    }

    log_message(LOG_DEFAULT, "Using port: %d %s", asid_port, _get_port_name(asid_port, name_buffer, sizeof(name_buffer)));
    if (_open_port(asid_port)) {
        return -1;
    }
    return _send_message(asid_start, sizeof(asid_start));
}

static int asid_flush(char *state)
{
    return 0;
}

static void _set_reg(uint8_t reg, uint8_t byte) {
    byte = regmask[reg] & byte;
    if (sid_register[reg] == byte) {
        return;
    }
    sid_register[reg] = byte;
    sid_modified[reg] = true;
    sid_modified_flag = true;
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
            // log_debug("reg %u -> %u", i, sid_register[mapped_reg]);
        }
    }
    asid_buffer[++p] = SYSEX_STOP;
    memset(sid_modified, false, sizeof(sid_modified));
    sid_modified_flag = false;
    return _send_message(asid_buffer, p + 1);
}

static int asid_dump(uint16_t addr, uint8_t byte, CLOCK clks)
{
    uint8_t reg = addr & 0x1f;
    if (reg > max_sid_reg) {
        return 0;
    }

    _set_reg(reg, byte);
    return 0;
}

static int asid_write(int16_t *pbuf, size_t nr) {
    return asid_write_();
}

static void asid_close(void)
{
    _send_message(asid_stop, sizeof(asid_stop));
    _close_port();
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
