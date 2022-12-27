/*
 * partner128.h -- "Partner 128" cartridge emulation
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef PARTNER128_H_
#define PARTNER128_H_

extern int partner128_crt_attach(FILE *fd, uint8_t *rawcart);
extern int partner128_bin_attach(const char *filename, uint8_t *rawcart);
extern void partner128_detach(void);
extern void partner128_reset(void);
extern void partner128_freeze(void);
extern void partner128_powerup(void);

extern void partner128_config_setup(uint8_t *rawcart);

#endif
