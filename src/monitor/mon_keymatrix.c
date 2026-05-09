/*
 * mon_keymatrix.c - Monitor keyboard-matrix injection with observation-based
 *                   verification.
 *
 * See mon_keymatrix.h for the public API and rationale.
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
 */

#include "vice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "alarm.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mon_keymatrix.h"
#include "monitor.h"
#include "montypes.h"
#include "types.h"

#define KMX_MAX_KEYS                16
#define KMX_DEFAULT_TIMEOUT_FRAMES  60

enum kmx_mode {
    KMX_MODE_IDLE = 0,
    KMX_MODE_TAP_OBSERVED,
    KMX_MODE_TAP_FIXED,
    KMX_MODE_PRESS_STICKY
};

enum kmx_release_reason {
    KMX_RELEASE_NONE     = 0,
    KMX_RELEASE_OBSERVED = 1,
    KMX_RELEASE_TIMEOUT  = 2,
    KMX_RELEASE_MANUAL   = 3
};

struct kmx_key {
    int  row;
    int  col;
    int  kind;        /* 0 = matrix bit, 1 = custom (RESTORE/CAPS/4080) */
    char name[16];
};

static struct {
    int active;                            /* master gate read by CIA1 hook */
    enum kmx_mode mode;
    struct kmx_key keys[KMX_MAX_KEYS];
    int n_keys;
    uint32_t cia1_reads_total;
    uint32_t cia1_reads_sampling;
    CLOCK timeout_clk;
    enum kmx_release_reason last_reason;
} kmx;

static struct {
    int valid;
    struct kmx_key keys[KMX_MAX_KEYS];
    int n_keys;
    uint32_t cia1_reads_total;
    uint32_t cia1_reads_sampling;
    enum kmx_release_reason reason;
    enum kmx_mode mode;
    uint16_t frames_held;
} last_tap_result;

static alarm_t *kmx_alarm = NULL;
static CLOCK kmx_press_clk;

struct kmx_keydef {
    const char *name;
    int row;
    int col;
    int kind;
};

/* C64 keyboard matrix. Names case-insensitive; aliases are separate
   entries pointing at the same (row,col). RESTORE/CAPS/4080 use negative
   row sentinels routed through keyboard_set_keyarr_any(). */
static const struct kmx_keydef c64_keys[] = {
    /* Row 0: INST/DEL, RETURN, CRSR LR, F7, F1, F3, F5, CRSR UD */
    { "INSTDEL",   0, 0, 0 }, { "DEL",       0, 0, 0 },
    { "INST",      0, 0, 0 }, { "BACKSPACE", 0, 0, 0 }, { "BS", 0, 0, 0 },
    { "RETURN",    0, 1, 0 }, { "ENTER",     0, 1, 0 }, { "RTN", 0, 1, 0 },
    { "CRSRLR",    0, 2, 0 }, { "CR",        0, 2, 0 }, { "RIGHT", 0, 2, 0 },
    { "F7",        0, 3, 0 },
    { "F1",        0, 4, 0 },
    { "F3",        0, 5, 0 },
    { "F5",        0, 6, 0 },
    { "CRSRUD",    0, 7, 0 }, { "CD",        0, 7, 0 }, { "DOWN", 0, 7, 0 },

    /* Row 1: 3, W, A, 4, Z, S, E, LSHIFT */
    { "3",      1, 0, 0 }, { "W", 1, 1, 0 }, { "A", 1, 2, 0 }, { "4", 1, 3, 0 },
    { "Z",      1, 4, 0 }, { "S", 1, 5, 0 }, { "E", 1, 6, 0 },
    { "LSHIFT", 1, 7, 0 }, { "SHIFT",     1, 7, 0 }, { "LEFTSHIFT", 1, 7, 0 },

    /* Row 2: 5, R, D, 6, C, F, T, X */
    { "5", 2, 0, 0 }, { "R", 2, 1, 0 }, { "D", 2, 2, 0 }, { "6", 2, 3, 0 },
    { "C", 2, 4, 0 }, { "F", 2, 5, 0 }, { "T", 2, 6, 0 }, { "X", 2, 7, 0 },

    /* Row 3: 7, Y, G, 8, B, H, U, V */
    { "7", 3, 0, 0 }, { "Y", 3, 1, 0 }, { "G", 3, 2, 0 }, { "8", 3, 3, 0 },
    { "B", 3, 4, 0 }, { "H", 3, 5, 0 }, { "U", 3, 6, 0 }, { "V", 3, 7, 0 },

    /* Row 4: 9, I, J, 0, M, K, O, N */
    { "9", 4, 0, 0 }, { "I", 4, 1, 0 }, { "J", 4, 2, 0 }, { "0", 4, 3, 0 },
    { "M", 4, 4, 0 }, { "K", 4, 5, 0 }, { "O", 4, 6, 0 }, { "N", 4, 7, 0 },

    /* Row 5: +, P, L, -, ., :, @, , */
    { "PLUS",   5, 0, 0 },
    { "P",      5, 1, 0 },
    { "L",      5, 2, 0 },
    { "MINUS",  5, 3, 0 },
    { "PERIOD", 5, 4, 0 }, { "DOT", 5, 4, 0 },
    { "COLON",  5, 5, 0 },
    { "AT",     5, 6, 0 },
    { "COMMA",  5, 7, 0 },

    /* Row 6: pound, *, ;, CLR/HOME, RSHIFT, =, up-arrow, / */
    { "POUND",     6, 0, 0 }, { "STERLING",   6, 0, 0 },
    { "TIMES",     6, 1, 0 }, { "STAR",       6, 1, 0 }, { "ASTERISK", 6, 1, 0 },
    { "SEMICOLON", 6, 2, 0 }, { "SEMI",       6, 2, 0 },
    { "CLR",       6, 3, 0 }, { "HOME",       6, 3, 0 }, { "CLRHOME",  6, 3, 0 },
    { "RSHIFT",    6, 4, 0 }, { "RIGHTSHIFT", 6, 4, 0 },
    { "EQUALS",    6, 5, 0 }, { "EQ",         6, 5, 0 },
    { "UPARROW",   6, 6, 0 }, { "UP",         6, 6, 0 }, { "EXPONENT", 6, 6, 0 },
    { "SLASH",     6, 7, 0 },

    /* Row 7: 1, left-arrow, CTRL, 2, SPACE, CBM, Q, RUN/STOP */
    { "1",         7, 0, 0 },
    { "LEFTARROW", 7, 1, 0 }, { "LEFT",      7, 1, 0 },
    { "CTRL",      7, 2, 0 },
    { "2",         7, 3, 0 },
    { "SPACE",     7, 4, 0 }, { "SP",        7, 4, 0 },
    { "CBM",       7, 5, 0 }, { "COMMODORE", 7, 5, 0 },
    { "Q",         7, 6, 0 },
    { "RUNSTOP",   7, 7, 0 }, { "STOP",      7, 7, 0 }, { "RUN", 7, 7, 0 },

    /* Special keys not in keyarr; routed via keyboard_set_keyarr_any. */
    { "RESTORE",  KBD_ROW_RESTORE_1,  KBD_COL_RESTORE_1,  1 },
    { "CAPSLOCK", KBD_ROW_CAPSLOCK,   KBD_COL_CAPSLOCK,   1 },
    { "CAPS",     KBD_ROW_CAPSLOCK,   KBD_COL_CAPSLOCK,   1 },
    { "4080",     KBD_ROW_4080COLUMN, KBD_COL_4080COLUMN, 1 },

    { NULL, 0, 0, 0 }
};

static int find_key_by_name(const char *name, struct kmx_key *out)
{
    int i;
    for (i = 0; c64_keys[i].name != NULL; i++) {
        if (strcasecmp(name, c64_keys[i].name) == 0) {
            out->row  = c64_keys[i].row;
            out->col  = c64_keys[i].col;
            out->kind = c64_keys[i].kind;
            strncpy(out->name, c64_keys[i].name, sizeof(out->name) - 1);
            out->name[sizeof(out->name) - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

static int parse_rc_pair(const char *tok, struct kmx_key *out)
{
    char *end;
    long r, c;
    r = strtol(tok, &end, 10);
    if (end == tok || *end != ',') {
        return -1;
    }
    c = strtol(end + 1, &end, 10);
    if (*end != '\0') {
        return -1;
    }
    if (r < -8 || r > 15 || c < 0 || c > 7) {
        return -1;
    }
    out->row  = (int)r;
    out->col  = (int)c;
    out->kind = (r < 0) ? 1 : 0;
    snprintf(out->name, sizeof(out->name), "%ld,%ld", r, c);
    return 0;
}

static int parse_key_token(const char *tok, struct kmx_key *out)
{
    if (strchr(tok, ',') != NULL) {
        return parse_rc_pair(tok, out);
    }
    return find_key_by_name(tok, out);
}

/* Parse a whitespace-tokenised arg string into a list of keys plus an
   optional "for <frames>" qualifier. Buffer is modified in place. */
static int parse_key_list(char *buf,
                          struct kmx_key *out_keys, int max_keys,
                          int *out_n_keys, int *out_for_frames)
{
    char *saveptr = NULL;
    char *tok;
    int n = 0;
    int for_frames = -1;

    for (tok = strtok_r(buf, " \t", &saveptr);
         tok != NULL;
         tok = strtok_r(NULL, " \t", &saveptr)) {
        if (strcasecmp(tok, "for") == 0) {
            char *frames_tok = strtok_r(NULL, " \t", &saveptr);
            char *end;
            long v;
            if (frames_tok == NULL) {
                mon_out("error: 'for' must be followed by a frame count\n");
                return -1;
            }
            v = strtol(frames_tok, &end, 0);
            if (end == frames_tok || *end != '\0' || v < 1 || v > 65535) {
                mon_out("error: invalid frame count '%s'\n", frames_tok);
                return -1;
            }
            for_frames = (int)v;
            continue;
        }
        if (n >= max_keys) {
            mon_out("error: too many keys (max %d)\n", max_keys);
            return -1;
        }
        if (parse_key_token(tok, &out_keys[n]) != 0) {
            mon_out("error: unknown key '%s' (try 'keymatrix names')\n", tok);
            return -1;
        }
        n++;
    }
    *out_n_keys = n;
    *out_for_frames = for_frames;
    return 0;
}

static void kmx_alarm_callback(CLOCK offset, void *data);

static void kmx_ensure_alarm(void)
{
    if (kmx_alarm == NULL) {
        kmx_alarm = alarm_new(maincpu_alarm_context, "MonitorKeymatrix",
                              kmx_alarm_callback, NULL);
    }
}

static void kmx_apply_bits(struct kmx_key *keys, int n_keys, int value)
{
    int i;
    for (i = 0; i < n_keys; i++) {
        keyboard_set_keyarr_any(keys[i].row, keys[i].col, value);
    }
}

static const char *kmx_reason_str(enum kmx_release_reason r)
{
    switch (r) {
        case KMX_RELEASE_OBSERVED: return "observed";
        case KMX_RELEASE_TIMEOUT:  return "timeout";
        case KMX_RELEASE_MANUAL:   return "manual";
        case KMX_RELEASE_NONE:     return "none";
    }
    return "?";
}

static const char *kmx_mode_str(enum kmx_mode m)
{
    switch (m) {
        case KMX_MODE_TAP_OBSERVED: return "tap-observed";
        case KMX_MODE_TAP_FIXED:    return "tap-fixed";
        case KMX_MODE_PRESS_STICKY: return "press";
        case KMX_MODE_IDLE:         return "idle";
    }
    return "?";
}

static void kmx_release_now(enum kmx_release_reason reason)
{
    int i;
    long cyc_per_frame;

    if (!kmx.active) {
        return;
    }

    kmx_apply_bits(kmx.keys, kmx.n_keys, 0);

    last_tap_result.valid = 1;
    last_tap_result.n_keys = kmx.n_keys;
    for (i = 0; i < kmx.n_keys; i++) {
        last_tap_result.keys[i] = kmx.keys[i];
    }
    last_tap_result.cia1_reads_total = kmx.cia1_reads_total;
    last_tap_result.cia1_reads_sampling = kmx.cia1_reads_sampling;
    last_tap_result.reason = (kmx.last_reason != KMX_RELEASE_NONE)
                              ? kmx.last_reason : reason;
    last_tap_result.mode = kmx.mode;

    cyc_per_frame = machine_get_cycles_per_frame();
    if (cyc_per_frame > 0 && maincpu_clk >= kmx_press_clk) {
        uint64_t held = (uint64_t)(maincpu_clk - kmx_press_clk);
        uint64_t f = held / (uint64_t)cyc_per_frame;
        last_tap_result.frames_held = (f > 65535) ? 65535 : (uint16_t)f;
    } else {
        last_tap_result.frames_held = 0;
    }

    kmx.active = 0;
    kmx.n_keys = 0;
    kmx.mode = KMX_MODE_IDLE;
    kmx.cia1_reads_total = 0;
    kmx.cia1_reads_sampling = 0;
    kmx.last_reason = KMX_RELEASE_NONE;

    if (kmx_alarm != NULL) {
        alarm_unset(kmx_alarm);
    }

    log_message(LOG_DEFAULT,
        "keymatrix: released after %u frames; cia1 reads: %u, sampling: %u (%s)",
        (unsigned)last_tap_result.frames_held,
        (unsigned)last_tap_result.cia1_reads_total,
        (unsigned)last_tap_result.cia1_reads_sampling,
        kmx_reason_str(last_tap_result.reason));
}

static void kmx_alarm_callback(CLOCK offset, void *data)
{
    enum kmx_release_reason r;
    (void)offset;
    (void)data;
    r = (kmx.last_reason != KMX_RELEASE_NONE)
        ? kmx.last_reason : KMX_RELEASE_TIMEOUT;
    kmx_release_now(r);
}

static void kmx_supersede_active(void)
{
    if (kmx.active) {
        kmx_release_now(KMX_RELEASE_MANUAL);
    }
}

void mon_keymatrix_tap(const char *args)
{
    char *buf;
    struct kmx_key keys[KMX_MAX_KEYS];
    int n_keys = 0;
    int for_frames = -1;
    long cyc_per_frame;
    int frames;
    int i;

    if (args == NULL || *args == '\0') {
        mon_out("usage: keymatrix tap <key> [<key> ...] [for <frames>]\n");
        return;
    }
    buf = lib_strdup(args);
    if (parse_key_list(buf, keys, KMX_MAX_KEYS, &n_keys, &for_frames) != 0) {
        lib_free(buf);
        return;
    }
    lib_free(buf);
    if (n_keys == 0) {
        mon_out("error: no keys specified\n");
        return;
    }

    kmx_supersede_active();
    kmx_ensure_alarm();

    for (i = 0; i < n_keys; i++) {
        kmx.keys[i] = keys[i];
    }
    kmx.n_keys = n_keys;
    kmx_apply_bits(kmx.keys, kmx.n_keys, 1);

    if (for_frames > 0) {
        kmx.mode = KMX_MODE_TAP_FIXED;
        frames = for_frames;
    } else {
        kmx.mode = KMX_MODE_TAP_OBSERVED;
        frames = KMX_DEFAULT_TIMEOUT_FRAMES;
    }
    kmx.cia1_reads_total = 0;
    kmx.cia1_reads_sampling = 0;
    kmx.last_reason = KMX_RELEASE_NONE;
    kmx.active = 1;
    kmx_press_clk = maincpu_clk;

    cyc_per_frame = machine_get_cycles_per_frame();
    if (cyc_per_frame <= 0) {
        cyc_per_frame = 19656;            /* PAL fallback */
    }
    kmx.timeout_clk = maincpu_clk +
        (CLOCK)((uint64_t)frames * (uint64_t)cyc_per_frame);
    alarm_set(kmx_alarm, kmx.timeout_clk);

    mon_out("keymatrix: tap %d key%s, mode=%s, max %d frame%s\n",
            n_keys, n_keys == 1 ? "" : "s",
            kmx_mode_str(kmx.mode),
            frames, frames == 1 ? "" : "s");
}

void mon_keymatrix_press(const char *args)
{
    char *buf;
    struct kmx_key keys[KMX_MAX_KEYS];
    int n_keys = 0;
    int for_frames = -1;
    int i;

    if (args == NULL || *args == '\0') {
        mon_out("usage: keymatrix press <key> [<key> ...]\n");
        return;
    }
    buf = lib_strdup(args);
    if (parse_key_list(buf, keys, KMX_MAX_KEYS, &n_keys, &for_frames) != 0) {
        lib_free(buf);
        return;
    }
    lib_free(buf);
    if (for_frames > 0) {
        mon_out("note: 'for <frames>' has no effect on press; use 'tap' for that\n");
    }
    if (n_keys == 0) {
        mon_out("error: no keys specified\n");
        return;
    }

    kmx_supersede_active();
    kmx_ensure_alarm();

    for (i = 0; i < n_keys; i++) {
        kmx.keys[i] = keys[i];
    }
    kmx.n_keys = n_keys;
    kmx_apply_bits(kmx.keys, kmx.n_keys, 1);

    kmx.mode = KMX_MODE_PRESS_STICKY;
    kmx.cia1_reads_total = 0;
    kmx.cia1_reads_sampling = 0;
    kmx.last_reason = KMX_RELEASE_NONE;
    kmx.active = 1;
    kmx_press_clk = maincpu_clk;
    /* No release alarm: clear via 'keymatrix release'. */

    mon_out("keymatrix: pressed %d key%s (sticky; clear with 'keymatrix release')\n",
            n_keys, n_keys == 1 ? "" : "s");
}

void mon_keymatrix_release(const char *args)
{
    char *buf;
    struct kmx_key keys[KMX_MAX_KEYS];
    int n_keys = 0;
    int for_frames = -1;
    int i;

    if (args == NULL || *args == '\0') {
        if (kmx.active) {
            kmx_release_now(KMX_RELEASE_MANUAL);
        }
        keyboard_clear_keymatrix();
        mon_out("keymatrix: released all keys\n");
        return;
    }

    buf = lib_strdup(args);
    if (parse_key_list(buf, keys, KMX_MAX_KEYS, &n_keys, &for_frames) != 0) {
        lib_free(buf);
        return;
    }
    lib_free(buf);
    if (n_keys == 0) {
        mon_out("error: no keys specified\n");
        return;
    }

    for (i = 0; i < n_keys; i++) {
        keyboard_set_keyarr_any(keys[i].row, keys[i].col, 0);
    }

    /* Drop matching keys from the active tracking set so observation
       counts stay correct. */
    if (kmx.active) {
        struct kmx_key surviving[KMX_MAX_KEYS];
        int n_surviving = 0;
        int j, k;
        for (j = 0; j < kmx.n_keys; j++) {
            int dropped = 0;
            for (k = 0; k < n_keys; k++) {
                if (kmx.keys[j].row == keys[k].row &&
                    kmx.keys[j].col == keys[k].col) {
                    dropped = 1;
                    break;
                }
            }
            if (!dropped) {
                surviving[n_surviving++] = kmx.keys[j];
            }
        }
        kmx.n_keys = n_surviving;
        for (j = 0; j < n_surviving; j++) {
            kmx.keys[j] = surviving[j];
        }
        if (kmx.n_keys == 0) {
            kmx_release_now(KMX_RELEASE_MANUAL);
        }
    }
    mon_out("keymatrix: released %d key%s\n",
            n_keys, n_keys == 1 ? "" : "s");
}

void mon_keymatrix_poke(const char *args)
{
    char *buf;
    char *saveptr = NULL;
    char *tok;
    char *end;
    long row, col, value;

    if (args == NULL || *args == '\0') {
        mon_out("usage: keymatrix poke <row> <col> <0|1>\n");
        return;
    }
    buf = lib_strdup(args);

    tok = strtok_r(buf, " \t", &saveptr);
    if (tok == NULL) { goto bad; }
    row = strtol(tok, &end, 0);
    if (*end != '\0') { goto bad; }

    tok = strtok_r(NULL, " \t", &saveptr);
    if (tok == NULL) { goto bad; }
    col = strtol(tok, &end, 0);
    if (*end != '\0') { goto bad; }

    tok = strtok_r(NULL, " \t", &saveptr);
    if (tok == NULL) { goto bad; }
    value = strtol(tok, &end, 0);
    if (*end != '\0' || (value != 0 && value != 1)) { goto bad; }

    if (col < 0 || col > 7 || row < -8 || row > 15) {
        mon_out("error: row must be -8..15, col must be 0..7\n");
        lib_free(buf);
        return;
    }

    keyboard_set_keyarr_any((int)row, (int)col, (int)value);
    mon_out("keymatrix: poked (%ld,%ld) <- %ld\n", row, col, value);
    lib_free(buf);
    return;

bad:
    mon_out("usage: keymatrix poke <row> <col> <0|1>\n");
    lib_free(buf);
}

void mon_keymatrix_show(void)
{
    int r, c;

    mon_out("keymatrix: live keyarr (rows down, cols right):\n");
    mon_out("       ");
    for (c = 0; c < 8; c++) {
        mon_out("%d ", c);
    }
    mon_out("\n");
    for (r = 0; r < 8; r++) {
        mon_out("  r%d:  ", r);
        for (c = 0; c < 8; c++) {
            mon_out("%c ", (keyarr[r] & (1 << c)) ? '*' : '.');
        }
        mon_out("\n");
    }
    mon_out("  RESTORE: %s   CAPSLOCK: %s   4080: %s\n",
            keyboard_custom_key_get(KBD_CUSTOM_RESTORE1) ? "on" : "off",
            keyboard_custom_key_get(KBD_CUSTOM_CAPS) ? "on" : "off",
            keyboard_custom_key_get(KBD_CUSTOM_4080) ? "on" : "off");

    if (kmx.active) {
        mon_out("\nactive injection: %s, %d key%s\n",
                kmx_mode_str(kmx.mode),
                kmx.n_keys, kmx.n_keys == 1 ? "" : "s");
        for (r = 0; r < kmx.n_keys; r++) {
            mon_out("  %s (%d,%d)\n", kmx.keys[r].name,
                    kmx.keys[r].row, kmx.keys[r].col);
        }
        mon_out("  cia1 reads: %u total, %u sampled injected bits\n",
                (unsigned)kmx.cia1_reads_total,
                (unsigned)kmx.cia1_reads_sampling);
    } else {
        mon_out("\nno active injection\n");
    }

    if (last_tap_result.valid) {
        mon_out("\nlast tap: %d key%s (%s)\n",
                last_tap_result.n_keys,
                last_tap_result.n_keys == 1 ? "" : "s",
                kmx_mode_str(last_tap_result.mode));
        for (r = 0; r < last_tap_result.n_keys; r++) {
            mon_out("  %s (%d,%d)\n", last_tap_result.keys[r].name,
                    last_tap_result.keys[r].row,
                    last_tap_result.keys[r].col);
        }
        mon_out("  released after %u frame%s; reason: %s\n",
                (unsigned)last_tap_result.frames_held,
                last_tap_result.frames_held == 1 ? "" : "s",
                kmx_reason_str(last_tap_result.reason));
        mon_out("  cia1 reads: %u total, %u sampled injected bits\n",
                (unsigned)last_tap_result.cia1_reads_total,
                (unsigned)last_tap_result.cia1_reads_sampling);
    }
}

void mon_keymatrix_names(void)
{
    int i;
    int col = 0;
    mon_out("keymatrix: known C64 key names (case-insensitive):\n  ");
    for (i = 0; c64_keys[i].name != NULL; i++) {
        mon_out("%-12s", c64_keys[i].name);
        col++;
        if (col == 5) {
            mon_out("\n  ");
            col = 0;
        }
    }
    if (col != 0) {
        mon_out("\n");
    }
    mon_out("\n  Use 'keymatrix poke <row> <col> <0|1>' for raw matrix bits.\n");
    mon_out("  Combine names for chords, e.g. 'keymatrix tap lshift a'.\n");
}

/* CIA1 observation hooks. Called from src/c64/c64cia1.c. Both are no-ops
   when no injection is active; that is the only path taken on every CIA1
   read while the feature is unused. */

void mon_keymatrix_cia1_pa_read(uint8_t row_scan_mask)
{
    int i;
    int sampled = 0;
    if (!kmx.active) {
        return;
    }
    kmx.cia1_reads_total++;
    for (i = 0; i < kmx.n_keys; i++) {
        if (kmx.keys[i].kind != 0) {
            continue;
        }
        if (kmx.keys[i].row < 0 || kmx.keys[i].row > 7) {
            continue;
        }
        if (!(row_scan_mask & (1 << kmx.keys[i].row))) {
            sampled = 1;
            break;
        }
    }
    if (sampled) {
        kmx.cia1_reads_sampling++;
        if (kmx.mode == KMX_MODE_TAP_OBSERVED &&
            kmx.last_reason == KMX_RELEASE_NONE) {
            kmx.last_reason = KMX_RELEASE_OBSERVED;
            if (kmx_alarm != NULL) {
                alarm_set(kmx_alarm, maincpu_clk + 1);
            }
        }
    }
}

void mon_keymatrix_cia1_pb_read(uint8_t col_scan_mask)
{
    int i;
    int sampled = 0;
    if (!kmx.active) {
        return;
    }
    kmx.cia1_reads_total++;
    for (i = 0; i < kmx.n_keys; i++) {
        if (kmx.keys[i].kind != 0) {
            continue;
        }
        if (kmx.keys[i].col < 0 || kmx.keys[i].col > 7) {
            continue;
        }
        if (!(col_scan_mask & (1 << kmx.keys[i].col))) {
            sampled = 1;
            break;
        }
    }
    if (sampled) {
        kmx.cia1_reads_sampling++;
        if (kmx.mode == KMX_MODE_TAP_OBSERVED &&
            kmx.last_reason == KMX_RELEASE_NONE) {
            kmx.last_reason = KMX_RELEASE_OBSERVED;
            if (kmx_alarm != NULL) {
                alarm_set(kmx_alarm, maincpu_clk + 1);
            }
        }
    }
}

/* ---- Binary monitor entry points ---- */

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

int mon_keymatrix_binmon_set(const uint8_t *body, uint32_t length)
{
    uint8_t count;
    uint32_t needed;
    uint32_t i;
    if (length < 1) {
        return -1;
    }
    count = body[0];
    needed = 1U + (uint32_t)count * 3U;
    if (length < needed) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        int8_t  row   = (int8_t)body[1 + i * 3 + 0];
        int8_t  col   = (int8_t)body[1 + i * 3 + 1];
        uint8_t value = body[1 + i * 3 + 2];
        keyboard_set_keyarr_any((int)row, (int)col, value ? 1 : 0);
    }
    return 0;
}

int mon_keymatrix_binmon_tap(const uint8_t *body, uint32_t length)
{
    uint8_t mode;
    uint16_t frames;
    uint8_t count;
    uint32_t needed;
    uint32_t i;
    long cyc_per_frame;
    int frames_int;

    if (length < 4) {
        return -1;
    }
    mode = body[0];
    frames = (uint16_t)body[1] | ((uint16_t)body[2] << 8);
    count = body[3];
    needed = 4U + (uint32_t)count * 2U;
    if (length < needed) {
        return -1;
    }
    if (count == 0 || count > KMX_MAX_KEYS) {
        return -1;
    }

    kmx_supersede_active();
    kmx_ensure_alarm();

    for (i = 0; i < count; i++) {
        int8_t row = (int8_t)body[4 + i * 2 + 0];
        int8_t col = (int8_t)body[4 + i * 2 + 1];
        kmx.keys[i].row  = (int)row;
        kmx.keys[i].col  = (int)col;
        kmx.keys[i].kind = (row < 0) ? 1 : 0;
        snprintf(kmx.keys[i].name, sizeof(kmx.keys[i].name),
                 "%d,%d", (int)row, (int)col);
    }
    kmx.n_keys = (int)count;
    kmx_apply_bits(kmx.keys, kmx.n_keys, 1);
    kmx.mode = (mode == 1) ? KMX_MODE_TAP_FIXED : KMX_MODE_TAP_OBSERVED;
    kmx.cia1_reads_total = 0;
    kmx.cia1_reads_sampling = 0;
    kmx.last_reason = KMX_RELEASE_NONE;
    kmx.active = 1;
    kmx_press_clk = maincpu_clk;

    cyc_per_frame = machine_get_cycles_per_frame();
    if (cyc_per_frame <= 0) {
        cyc_per_frame = 19656;
    }
    frames_int = (frames > 0) ? frames : KMX_DEFAULT_TIMEOUT_FRAMES;
    kmx.timeout_clk = maincpu_clk +
        (CLOCK)((uint64_t)frames_int * (uint64_t)cyc_per_frame);
    alarm_set(kmx_alarm, kmx.timeout_clk);
    return 0;
}

int mon_keymatrix_binmon_get(uint8_t *out, uint32_t *out_length)
{
    int i;
    uint8_t custom = 0;
    uint16_t frames_left = 0;

    if (out == NULL || out_length == NULL) {
        return -1;
    }
    if (*out_length < MON_KEYMATRIX_BINMON_GET_RESPONSE_SIZE) {
        return -1;
    }

    for (i = 0; i < 8; i++) {
        out[i] = (uint8_t)(keyarr[i] & 0xff);
    }
    if (keyboard_custom_key_get(KBD_CUSTOM_RESTORE1)) custom |= 1U << 0;
    if (keyboard_custom_key_get(KBD_CUSTOM_RESTORE2)) custom |= 1U << 1;
    if (keyboard_custom_key_get(KBD_CUSTOM_CAPS))     custom |= 1U << 2;
    if (keyboard_custom_key_get(KBD_CUSTOM_4080))     custom |= 1U << 3;
    out[8]  = custom;
    out[9]  = 0;
    out[10] = 0;
    out[11] = 0;

    if (kmx.active) {
        put_le32(&out[12], kmx.cia1_reads_total);
        put_le32(&out[16], kmx.cia1_reads_sampling);
        out[20] = (uint8_t)kmx.last_reason;
        out[21] = (uint8_t)kmx.n_keys;
    } else if (last_tap_result.valid) {
        put_le32(&out[12], last_tap_result.cia1_reads_total);
        put_le32(&out[16], last_tap_result.cia1_reads_sampling);
        out[20] = (uint8_t)last_tap_result.reason;
        out[21] = (uint8_t)last_tap_result.n_keys;
    } else {
        put_le32(&out[12], 0);
        put_le32(&out[16], 0);
        out[20] = 0;
        out[21] = 0;
    }

    if (kmx.active && kmx.timeout_clk > maincpu_clk) {
        long cpf = machine_get_cycles_per_frame();
        if (cpf > 0) {
            uint64_t left = (uint64_t)(kmx.timeout_clk - maincpu_clk);
            uint64_t f = left / (uint64_t)cpf;
            frames_left = (f > 65535) ? 65535 : (uint16_t)f;
        }
    }
    put_le16(&out[22], frames_left);

    *out_length = MON_KEYMATRIX_BINMON_GET_RESPONSE_SIZE;
    return 0;
}
