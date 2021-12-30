/** \file   reuwidget.c
 * \brief   Widget to control RAM Expansion Module resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES REU             x64 x64sc xscpu64 x128
 * $VICERES REUsize         x64 x64sc xscpu64 x128
 * $VICERES REUfilename     x64 x64sc xscpu64 x128
 * $VICERES REUImageWrite   x64 x64sc xscpu64 x128
 * $VICERES REUIOSwap       xvic
 *  (FIXME: check this one, seems odd, no mention in vice.texi)
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

#include "vice.h"
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "machine.h"
#include "resources.h"
#include "cartridge.h"

#include "reuwidget.h"


/** \brief  List of supported RAM sizes in KiB/MiB
 *
 * REU sizes tend to be specified in MiB when being 1MiB or higher, not KiB.
 */
static const vice_gtk3_radiogroup_entry_t ram_sizes[] = {
    { "128KiB",     128 },
    { "256KiB",     256 },
    { "512KiB",     512 },
    { "1MiB",       1024 },
    { "2MiB",       2048 },
    { "4MiB",       4096 },
    { "8MiB",       8192 },
    { "16MiB",      16384 },
    { NULL,         -1 }
};


/** \brief  Create IO-swap check button (seems to be valid for xvic only)
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_reu_ioswap_widget(void)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new(
            "REUIOSwap", "MasC=uarade I/O swap");
    return check;
}


/** \brief  Create radio button group to determine REU RAM size
 *
 * \return  GtkGrid
 */
static GtkWidget *create_reu_size_widget(void)
{
    GtkWidget *grid;
    GtkWidget *radio_group;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "RAM Size", 1);
    radio_group = vice_gtk3_resource_radiogroup_new("REUsize", ram_sizes,
            GTK_ORIENTATION_VERTICAL);
    g_object_set(radio_group, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to load/save REU image file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_reu_image_widget(void)
{
    return cart_image_widget_create(
            NULL, "REU image",
            "REUfilename", "REUImageWrite",
            carthelpers_save_func, carthelpers_flush_func,
            carthelpers_can_save_func, carthelpers_can_flush_func,
            CARTRIDGE_NAME_REU, CARTRIDGE_REU);
}


/** \brief  Create widget to control RAM Expansion Module resources
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *reu_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *reu_enable_widget;
    GtkWidget *reu_size;
    GtkWidget *reu_ioswap;
    GtkWidget *reu_image;

    grid = vice_gtk3_grid_new_spaced(8, 8);

    reu_enable_widget = carthelpers_create_enable_check_button(
            CARTRIDGE_NAME_REU, CARTRIDGE_REU);
    gtk_grid_attach(GTK_GRID(grid), reu_enable_widget, 0, 0, 2, 1);

    if (machine_class == VICE_MACHINE_VIC20) {
        reu_ioswap = create_reu_ioswap_widget();
        gtk_grid_attach(GTK_GRID(grid), reu_ioswap, 0, 1, 1, 1);
    }

    reu_size = create_reu_size_widget();
    gtk_grid_attach(GTK_GRID(grid), reu_size, 0, 1, 1, 1);

    reu_image = create_reu_image_widget();
    gtk_grid_attach(GTK_GRID(grid), reu_image, 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
