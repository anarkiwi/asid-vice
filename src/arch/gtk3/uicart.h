/** \file   uicart.h
 * \brief   Widget to attach carts - header
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

#ifndef VICE_UICART_H
#define VICE_UICART_H

#include "vice.h"
#include <gtk/gtk.h>

#include "cartridge.h"

void ui_cart_set_list_func(cartridge_info_t *(*func)(void));
void ui_cart_set_detect_func(int (*func)(const char *));
void ui_cart_set_attach_func(int (*func)(int, const char *));
void ui_cart_set_freeze_func(void (*func)(void));
void ui_cart_set_detach_func(void (*func)(int));
void ui_cart_set_set_default_func(void (*func)(void));
void ui_cart_set_unset_default_func(void (*func)(void));

gboolean ui_cart_trigger_freeze(void);
gboolean ui_cart_detach(void);

gboolean ui_cart_show_dialog(GtkWidget *widget, gpointer data);
void ui_cart_default_attach(GtkWidget *widget, void (*callback)(void));


void ui_cart_shutdown(void);

#endif
