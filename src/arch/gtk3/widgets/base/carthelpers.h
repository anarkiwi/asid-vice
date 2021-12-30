/** \file   carthelpers.h
 * \brief   Cartridge helper functions - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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

#ifndef VICE_CARTHELPERS_H
#define VICE_CARTHELPERS_H

#include "vice.h"
#include <gtk/gtk.h>
#include "cartridge.h"

extern int (*carthelpers_save_func)(int type, const char *filename);
extern int (*carthelpers_flush_func)(int type);
extern int (*carthelpers_is_enabled_func)(int type);
extern int (*carthelpers_enable_func)(int type);
extern int (*carthelpers_disable_func)(int type);
extern int (*carthelpers_can_save_func)(int type);
extern int (*carthelpers_can_flush_func)(int type);
extern void (*carthelpers_set_default_func)(void);
extern void (*carthelpers_unset_default_func)(void);
extern cartridge_info_t * (*carthelpers_info_list_func)(void);


void carthelpers_set_functions(
        int (*save_func)(int, const char *),
        int (*flush_func)(int),
        int (*is_enabled_func)(int),
        int (*enable_func)(int),
        int (*disable_func)(int),
        int (*can_save_func)(int),
        int (*can_flush_func)(int),
        void (*set_default_func)(void),
        void (*unset_default_func)(void),
        cartridge_info_t * (*info_list_func)(void));

GtkWidget *carthelpers_create_enable_check_button(const char *cart_name,
                                                  int cart_id);
#endif
