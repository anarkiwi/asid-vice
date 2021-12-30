/** \file   widgethelpers.h
 * \brief   GTK3 helper functions for widgets - header
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
 */

#ifndef VICE_WIDGETHELPERS_H
#define VICE_WIDGETHELPERS_H

#include "vice.h"
#include <gtk/gtk.h>
#include <stdbool.h>

#include "basewidget_types.h"

#ifdef MACOSX_SUPPORT

/** \brief  Unicode text for the VICE_MOD_MASK (Command/⌘) modifier
 */
# define VICE_MOD_MASK_TEXT "⌘"

/** \brief  HTML text for the VICE_MOD_MASK (Command/⌘) modifier
 */
# define VICE_MOD_MASK_HTML "&#8984;"

#else

/** \brief  Unicode text for the VICE_MOD_MASK (Alt) modifier
 */
# define VICE_MOD_MASK_TEXT "Alt"

/** \brief  HTML text for the VICE_MOD_MASK (Alt) modifier
 */
# define VICE_MOD_MASK_HTML "Alt"

#endif


/* only useful for custom built radio groups, don't use on resourceradiogroup.c
 * generated stuff */
int vice_gtk3_radiogroup_get_list_index(
        const vice_gtk3_radiogroup_entry_t *list,
        int value);

void vice_gtk3_radiogroup_set_index(GtkWidget *grid, int index);


/* pretty useless, should be removed */
GtkWidget *vice_gtk3_create_indented_label(const char *text);


/* new stuff */

GtkWidget *vice_gtk3_grid_new_spaced(int column_spacing, int row_spacing);

GtkWidget *vice_gtk3_grid_new_spaced_with_label(int column_spacing,
                                                int row_spacing,
                                                const char *label,
                                                int span);

void vice_gtk3_grid_set_margins(GtkWidget *grid,
                                gint top,
                                gint bottom,
                                gint left,
                                gint right);

unsigned char *vice_gtk3_petscii_to_utf8(unsigned char *s,
                                         bool inverted,
                                         bool lowercase);

#endif
