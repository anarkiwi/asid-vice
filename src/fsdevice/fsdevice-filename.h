/*
 * fsdevice-filename.h - File system device.
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

#ifndef VICE_FSDEVICE_FILENAME_H
#define VICE_FSDEVICE_FILENAME_H


#include "vdrive.h"

extern int fsdevice_limit_createnamelength(vdrive_t *vdrive, char *name);

extern int fsdevice_limit_namelength(vdrive_t *vdrive, uint8_t *name);
extern int fsdevice_limit_namelength_ascii(vdrive_t *vdrive, char *name);

extern char *fsdevice_expand_shortname(vdrive_t *vdrive, char *name);
extern char *fsdevice_expand_shortname_ascii(vdrive_t *vdrive, char *name);

#endif
