/*
 * spi-flash.h
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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

#ifndef VICE_SPI_FLASH
#define VICE_SPI_FLASH

#include "types.h"

extern uint8_t spi_flash_read_data(void);
extern void spi_flash_write_data(uint8_t value);
extern void spi_flash_write_select(uint8_t value);
extern void spi_flash_write_clock(uint8_t value);

extern void spi_flash_set_image(uint8_t *img, uint32_t size);

struct snapshot_s;
extern int spi_flash_snapshot_read_module(struct snapshot_s *s);
extern int spi_flash_snapshot_write_module(struct snapshot_s *s);

#endif
