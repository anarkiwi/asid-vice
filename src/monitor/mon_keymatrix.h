/*
 * mon_keymatrix.h - Monitor keyboard-matrix injection with observation-based
 *                   verification.
 *
 * Lets the monitor poke bits into the C64 keyboard matrix directly (i.e. at
 * the CIA1 layer, below the KERNAL keyboard buffer at $277). Programs that
 * scan the matrix themselves (games, demos, copy-protected loaders) see the
 * injected keys reliably. A small hook in the CIA1 read paths counts how
 * many of those reads actually sampled an injected bit, so a `tap` can
 * release on confirmed observation rather than on a guessed timeout.
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 */

#ifndef VICE_MON_KEYMATRIX_H
#define VICE_MON_KEYMATRIX_H

#include "types.h"

/* Parser-action entry points. The strings are the rest-of-line argument as
   captured by the `R_O_L` lexer rule. NULL/empty strings mean "no
   arguments" where the verb allows it. */
void mon_keymatrix_tap(const char *args);
void mon_keymatrix_press(const char *args);
void mon_keymatrix_release(const char *args);
void mon_keymatrix_poke(const char *args);
void mon_keymatrix_show(void);
void mon_keymatrix_names(void);

/* CIA1 observation hooks. Called from src/c64/c64cia1.c read_ciapa /
   read_ciapb. Each hook early-returns when no injection is active, so the
   steady-state cost is one predictable-branch test per matrix-register read.

   `scan_mask` is the value the CIA1 read function uses to decide which
   rows/columns are being driven low (active when bit == 0):
     - PA-read scan iterates rows  (mask comes from PB-driven rows)
     - PB-read scan iterates cols  (mask comes from PA-driven columns)
*/
void mon_keymatrix_cia1_pa_read(uint8_t row_scan_mask);
void mon_keymatrix_cia1_pb_read(uint8_t col_scan_mask);

/* Binary-monitor entry points. Called from monitor_binary.c. The buffers
   are the raw command body (caller-validated for length).

   keymatrix_set: count:u8, then count records of (row:i8, col:i8, value:u8)
       Atomically applies the bit changes. No release scheduled.
   keymatrix_tap: mode:u8 (0=observed-with-timeout, 1=fixed-frames),
       frames:u16, count:u8, then count records of (row:i8, col:i8)
       Sets bits and arms release per the chosen mode.
   keymatrix_get: no body. Output written to *out (40 bytes):
       keyarr[0..7] (8 bytes), custom-key state (1 byte: bit0=RESTORE1,
       bit1=RESTORE2, bit2=CAPS, bit3=4080), pad (3 bytes),
       cia1_reads_total:u32, cia1_reads_sampling:u32,
       release_reason:u8 (0=none, 1=observed, 2=timeout, 3=manual),
       last_tap_keys:u8, frames_until_release:u16, pad:u2.
       Total laid out little-endian; see mon_keymatrix.c for the writer.
   Return: 0 on success, negative on validation failure (caller maps to
       protocol error). */
int mon_keymatrix_binmon_set(const uint8_t *body, uint32_t length);
int mon_keymatrix_binmon_tap(const uint8_t *body, uint32_t length);
int mon_keymatrix_binmon_get(uint8_t *out, uint32_t *out_length);

#define MON_KEYMATRIX_BINMON_GET_RESPONSE_SIZE 24

#endif
