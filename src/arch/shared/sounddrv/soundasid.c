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

#include "debug.h"
#include "log.h"
#include "vice.h"

#include "interrupt.h"
#include "maincpu.h"

#include "sound.h"
#include "types.h"

#include <alsa/asoundlib.h>

#define SYSEX_START 0xf0
#define SYSEX_MAN_ID 0x2d
#define SYSEX_STOP 0xf7
#define MIDI_CLOCK 0xf8
#define NOTEOFF16 0x8f
#define NOTEOFF15 0x8e
#define NOTELEN 3

#define ASID_START 0x4c
#define ASID_STOP 0x4d
#define ASID_UPDATE 0x4e
#define ASID_UPDATE2 0x50
#define ASID_UPDATE_REG 0x6c
#define ASID_UPDATE2_REG 0x6d

#define ALL_MIDI_PORTS -1
#define NO_PORT -1

#define CHIPS 2

const uint8_t asid_start[] = {SYSEX_START, SYSEX_MAN_ID, ASID_START,
                              SYSEX_STOP};
const uint8_t asid_stop[] = {SYSEX_START, SYSEX_MAN_ID, ASID_STOP, SYSEX_STOP};
const uint8_t asid_prefix[] = {SYSEX_START, SYSEX_MAN_ID};
const uint8_t asid_update[] = {ASID_UPDATE, ASID_UPDATE2};
const uint8_t asid_update_reg[] = {ASID_UPDATE_REG, ASID_UPDATE2_REG};
const uint8_t asid_single_reg[] = {NOTEOFF16, NOTEOFF15};
const uint8_t asid_clock[] = {MIDI_CLOCK};
const uint8_t max_sid_reg = 24;
/* IDs 25-27 not implemented. They are rumoured to make additional updates to
   registers 4, 11, and 18, but asidxp.exe doesn't seem to use them. */
const uint8_t regmap[] = {0,  1,  2,  3,  5,  6,  7,  8,  9,  10, 12, 13, 14,
                          15, 16, 17, 19, 20, 21, 22, 23, 24, 4,  11, 18};
const uint8_t regmask[] = {
    // 0    1    2    3    4    5    6    7
    255, 255, 255, 15, 255, 255, 255, 255,
    // 8    9   10   11   12   13   14   15
    255, 255, 15, 255, 255, 255, 255, 255,
    // 16  17   18   19   20   21   22   23  24
    255, 15, 255, 255, 255, 7, 255, 255, 255};

static snd_seq_t *seq;
static int vport, queue_id;
static snd_seq_port_subscribe_t *subscription;
static snd_midi_event_t *coder;

/* update preamble, mask/MSB, register map, stop. */
#define ASID_BUFFER_SIZE 256

typedef struct {
  uint8_t single_buffer[ASID_BUFFER_SIZE];
  uint8_t update_buffer[ASID_BUFFER_SIZE];
  uint8_t update_reg_buffer[ASID_BUFFER_SIZE];
  uint8_t sid_register[sizeof(regmap)];
  uint8_t sid_modified[sizeof(regmap)];
  bool sid_modified_flag;
  CLOCK last_irq;
  uint64_t start_clock;
} asid_state_t;

static asid_state_t asid_state[CHIPS];
static uint32_t bytes_saved = 0;
static uint32_t bytes_total = 0;
static bool use_update_reg = false;

static uint64_t get_clock(void) {
  struct timespec res;
  clock_gettime(CLOCK_MONOTONIC, &res);
  return (res.tv_sec * 1e9) + res.tv_nsec;
}

static int _send_message(const uint8_t *message, uint8_t message_len,
                         uint64_t nsec);
static int asid_write_(uint8_t chip, uint64_t nsec);

/* TODO: refactor libmididrv API for cross platform support. */
static int _initialize_midi(void) {
  int result =
      snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, SND_SEQ_NONBLOCK);
  if (result < 0) {
    log_message(LOG_DEFAULT, "snd_seq_open() failed");
    return -1;
  }

  snd_seq_set_client_name(seq, "asid");

  vport = NO_PORT;
  coder = 0;
  result = snd_midi_event_new(ASID_BUFFER_SIZE, &coder);
  if (result < 0) {
    log_message(LOG_DEFAULT, "snd_midi_event_new() failed");
    return -1;
  }
  snd_midi_event_init(coder);
  snd_seq_set_client_pool_output(seq, ASID_BUFFER_SIZE);
  return 0;
}

static unsigned int _get_port_info(snd_seq_port_info_t *pinfo,
                                   int port_number) {
  unsigned int port_type = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
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
        if ((snd_seq_port_info_get_type(pinfo) &
             SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {
          if ((snd_seq_port_info_get_capability(pinfo) & port_type) ==
              port_type) {
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

static unsigned int _get_port_count(void) {
  snd_seq_port_info_t *pinfo;
  snd_seq_port_info_alloca(&pinfo);
  unsigned int nports = _get_port_info(pinfo, ALL_MIDI_PORTS);
  return nports;
}

static int _open_port(unsigned int port_number) {
  bytes_total = 0;
  bytes_saved = 0;

  unsigned int nports = _get_port_count();
  if (nports < 1) {
    return -1;
  }

  snd_seq_port_info_t *pinfo;
  snd_seq_port_info_alloca(&pinfo);

  if (_get_port_info(pinfo, (int)port_number) == 0) {
    return -1;
  }

  snd_seq_addr_t sender, receiver;
  receiver.client = snd_seq_port_info_get_client(pinfo);
  receiver.port = snd_seq_port_info_get_port(pinfo);
  sender.client = snd_seq_client_id(seq);

  vport = snd_seq_create_simple_port(
      seq, "asid", SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
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

  queue_id = snd_seq_alloc_queue(seq);
  snd_seq_start_queue(seq, queue_id, NULL);

  if (_send_message(asid_start, sizeof(asid_start), 0)) {
    log_message(LOG_DEFAULT, "asid start failed");
    return -1;
  }

  for (int chip = 0; chip < CHIPS; ++chip) {
    asid_state_t *state = &asid_state[chip];
    memset(&(state->single_buffer), 0, sizeof(state->single_buffer));
    memcpy(&(state->update_buffer), asid_prefix, sizeof(asid_prefix));
    memcpy(&(state->update_reg_buffer), asid_prefix, sizeof(asid_prefix));
    state->update_buffer[sizeof(asid_prefix)] = asid_update[chip];
    state->update_reg_buffer[sizeof(asid_prefix)] = asid_update_reg[chip];
    memset(&(state->sid_register), 0, sizeof(state->sid_register));
    memset(&(state->sid_modified), true, sizeof(state->sid_modified));
    state->sid_modified_flag = true;
    state->last_irq = 0;
    state->start_clock = 0;
    if (asid_write_(chip, 0)) {
      log_message(LOG_DEFAULT, "initial write failed");
      return -1;
    }
  }

  return 0;
}

static char *_get_port_name(unsigned int port_number, char *name_buffer,
                            unsigned int max_name) {
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;
  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);
  memset(name_buffer, 0, max_name);

  if (_get_port_info(pinfo, (int)port_number)) {
    int cnum = snd_seq_port_info_get_client(pinfo);
    snd_seq_get_any_client_info(seq, cnum, cinfo);
    snprintf(name_buffer, max_name, "%s:%d",
             snd_seq_client_info_get_name(cinfo),
             snd_seq_port_info_get_port(pinfo));
  }

  return name_buffer;
}

static int _send_message(const uint8_t *message, uint8_t message_len,
                         uint64_t nsec) {
  int result;
  snd_seq_event_t ev;
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, vport);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  result = snd_midi_event_encode(coder, message, message_len, &ev);

  if (result < (int)message_len) {
    log_message(LOG_DEFAULT, "snd_midi_event_encode() failed");
    return -1;
  }
  snd_seq_real_time_t time;
  time.tv_sec = nsec / 1e9;
  time.tv_nsec = nsec - (time.tv_sec * 1e9);
  snd_seq_ev_schedule_real(&ev, queue_id, SND_SEQ_TIME_MODE_REL, &time);
  if (snd_seq_event_output_direct(seq, &ev) < 0) {
    log_message(LOG_DEFAULT, "snd_seq_ev_schedule_real() %lu failed", nsec);
    return -1;
  }
  snd_seq_drain_output(seq);

  bytes_total += message_len;

  // for (int i = 0; i < message_len; ++i) {
  //   log_message(LOG_DEFAULT, "%2u %x", i, message[i]);
  // }
  return 0;
}

static int _close_port(void) {
  log_message(LOG_DEFAULT, "%u asid bytes sent, %u bytes saved", bytes_total,
              bytes_saved);
  if (vport != NO_PORT) {
    _send_message(asid_stop, sizeof(asid_stop), 0);
  }
  snd_seq_stop_queue(seq, queue_id, NULL);
  snd_seq_free_queue(seq, queue_id);
  if (vport != NO_PORT) {
    snd_seq_unsubscribe_port(seq, subscription);
    snd_seq_port_subscribe_free(subscription);
    snd_seq_delete_port(seq, vport);
    vport = NO_PORT;
  }
  return 0;
}

static int asid_write_(uint8_t chip, uint64_t nsec) {
  asid_state_t *state = &(asid_state[chip]);

  if (!state->sid_modified_flag) {
    return 0;
  }

  uint8_t i;
  uint8_t s = 0;
  uint8_t t = sizeof(asid_prefix) + 1;
  uint8_t c = 0;

  for (i = 0; i < sizeof(regmap); ++i) {
    if (!state->sid_modified[i]) {
      continue;
    }
    uint8_t val = state->sid_register[i];
    state->single_buffer[s++] = asid_single_reg[chip];
    uint8_t reg = i;
    if (val > 0x7f) {
      reg |= (1 << 6);
    }
    val &= 0x7f;
    state->update_reg_buffer[t++] = reg;
    state->single_buffer[s++] = reg;
    state->update_reg_buffer[t++] = val;
    state->single_buffer[s++] = val;
    ++c;
  }
  state->update_reg_buffer[t++] = SYSEX_STOP;

  uint8_t mapped_reg;
  uint8_t m = sizeof(asid_prefix) + 1;
  uint8_t p = m + 8;
  uint32_t mask = 0;
  uint32_t msb = 0;

  /* set bits in mask for each register that has been written to
     write last bit of each register into msb. */
  for (i = 0; i < sizeof(regmap); ++i) {
    mapped_reg = regmap[i];
    if (!state->sid_modified[mapped_reg]) {
      continue;
    }
    uint8_t val = state->sid_register[mapped_reg];
    mask |= (1 << i);
    if (val > 0x7f) {
      msb |= (1 << i);
    }
    state->update_buffer[p++] = val & 0x7f;
  }
  for (i = 0; i < sizeof(mask); ++i) {
    state->update_buffer[m++] = mask & 0x7f;
    mask >>= 7;
  }
  for (i = 0; i < sizeof(msb); ++i) {
    state->update_buffer[m++] = msb & 0x7f;
    msb >>= 7;
  }
  state->update_buffer[p++] = SYSEX_STOP;
  state->sid_modified_flag = false;
  memset(&(state->sid_modified), false, sizeof(state->sid_modified));
  if (use_update_reg && (t < p)) {
    // if (s < t) {
    //   for (i = 0; i < s; i += NOTELEN) {
    //     if (_send_message((state->single_buffer) + i, NOTELEN)) {
    //       return -1;
    //     }
    //   }
    //   bytes_saved += (p - s);
    // } else {
    if (_send_message(state->update_reg_buffer, t, nsec)) {
      return -1;
    }
    bytes_saved += (p - t);
    // }
  } else {
    if (_send_message(state->update_buffer, p, nsec)) {
      return -1;
    }
  }
  return 0;
}

static int asid_init(const char *param, int *speed, int *fragsize, int *fragnr,
                     int *channels) {
  int i;
  int nports;
  int asid_param;
  int asid_port;
  char name_buffer[256];

  *channels = 2;

  if (_initialize_midi()) {
    log_message(LOG_DEFAULT, "failed to initialize MIDI");
    return -1;
  }

  nports = _get_port_count();
  if (nports == 0) {
    log_message(LOG_DEFAULT, "No MIDI ports available");
    return -1;
  }

  log_message(LOG_DEFAULT, "asid open, available ports");
  for (i = 0; i < nports; ++i)
    log_message(LOG_DEFAULT, "Port %d : %s", i,
                _get_port_name(i, name_buffer, sizeof(name_buffer)));

  if (!param) {
    log_message(LOG_DEFAULT, "-soundarg <n> is required");
    return -1;
  }

  asid_param = atoi(param);
  asid_port = asid_param & 1023;
  use_update_reg = asid_param & 1024;

  if (asid_port < 0 || asid_port > (nports - 1)) {
    log_message(LOG_DEFAULT, "invalid MIDI port in -soundarg");
    return -1;
  }

  if (use_update_reg) {
    log_message(LOG_DEFAULT, "Using asid register update messages");
  }

  log_message(LOG_DEFAULT, "Using asid port: %d %s", asid_port,
              _get_port_name(asid_port, name_buffer, sizeof(name_buffer)));
  if (_open_port(asid_port)) {
    log_message(LOG_DEFAULT, "Open port failed");
    return -1;
  }

  return 0;
}

static void _set_reg(uint8_t reg, uint8_t byte, uint8_t chip) {
  asid_state_t *state = &asid_state[chip];
  byte = regmask[reg] & byte;
  if (state->sid_register[reg] == byte) {
    return;
  }
  // flush on change to control register.
  if (((reg == 4 || reg == 11 || reg == 18) || use_update_reg) &&
      state->sid_modified[reg]) {
    asid_write_(chip, 0);
  }
  state->sid_register[reg] = byte;
  state->sid_modified[reg] = true;
  state->sid_modified_flag = true;
}

static uint64_t clock_to_nanos(uint64_t clock) {
  return clock / (17.734475 / 18 * 1e6) * 1e9;
}

static int asid_dump2(CLOCK clks, CLOCK irq_clks, CLOCK nmi_clks,
                      uint8_t chipno, uint16_t addr, uint8_t byte) {
  asid_state_t *state = &asid_state[chipno];
  CLOCK irq_diff = maincpu_int_status->irq_clk - state->last_irq;

  // Flush changes from previous IRQ.
  if (irq_diff > 256) {
    uint64_t now = get_clock();
    if (state->start_clock == 0) {
      state->start_clock = now;
    }
    state->last_irq = maincpu_int_status->irq_clk;
    int64_t n = clock_to_nanos(state->last_irq) - (now - state->start_clock);
    if (n < 0) {
      float slip_ms = labs(n) / 1e6;
      if (slip_ms > 1) {
        log_message(LOG_DEFAULT, "asid slip by %fms", slip_ms);
      }
      n = 0;
    }
    asid_write_(chipno, n);
  }

  uint8_t reg = addr & 0x1f;
  if (reg > max_sid_reg) {
    return 0;
  }

  _set_reg(reg, byte, chipno);

  // Many playroutines write to all registers in sequence, so flush on the last
  // register.
  // if (reg == 24) {
  //   asid_write_(chipno);
  // }
  return 0;
}

static int asid_write(int16_t *pbuf, size_t nr) { return 0; }

static void asid_close(void) {
  _close_port();
  snd_midi_event_free(coder);
  snd_seq_close(seq);
}

static int asid_flush(char *state) { return 0; }

static sound_device_t asid_device = {
    "asid",     asid_init, asid_write, NULL, asid_dump2, asid_flush, NULL,
    asid_close, NULL,      NULL,       0,    2,          false};

int sound_init_asid_device(void) { return sound_register_device(&asid_device); }
