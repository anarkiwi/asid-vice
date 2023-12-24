/** \file   settings_ramlink.c
 * \brief   Settings widget to control RAMLink resources
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES RAMLINK             x64 x64sc xscpu64 x128
 * $VICERES RAMLINKfilename     x64 x64sc xscpu64 x128
 * $VICERES RAMLINKImageWrite   x64 x64sc xscpu64 x128
 * $VICERES RAMLINKsize         x64 x64sc xscpu64 x128
 * $VICERES RAMLINKmode         x64 x64sc xscpu64 x128
 * $VICERES RAMLINKRTCSave      x64 x64sc xscpu64 x128
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

#include "carthelpers.h"
#include "cartridge.h"
#include "debug_gtk3.h"
#include "ramlink.h"
#include "resources.h"
#include "vice_gtk3.h"

#include "settings_ramlink.h"


/** \brief  RAMLINK modes
 */
static const vice_gtk3_radiogroup_entry_t ramlink_modes[] = {
    { "Normal",   RL_MODE_NORMAL },
    { "Direct",   RL_MODE_DIRECT },
    { NULL,      -1 }
};


/** \brief  Create left-aligned label with Pango markup
 *
 * \param[in]   text    label text (uses Pango markup)
 *
 * \return  GtkLabel
 */
static GtkWidget *label_helper(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);

    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

/** \brief  Create RAMLink BIOS image widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_primary_image_widget(void)
{
    return cart_image_widget_new(CARTRIDGE_RAMLINK,
                                 CARTRIDGE_NAME_RAMLINK,
                                 CART_IMAGE_PRIMARY,
                                 "BIOS",
                                 "RAMLINKBIOSfilename",
                                 FALSE,
                                 FALSE);
}

/** \brief  Create RAMLink RAM image widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_secondary_image_widget(void)
{
    GtkWidget *image;

    image = cart_image_widget_new(CARTRIDGE_RAMLINK,
                                  CARTRIDGE_NAME_RAMLINK,
                                  CART_IMAGE_SECONDARY,
                                  "RAM",
                                  "RAMLINKfilename",
                                  TRUE,
                                  TRUE);
    cart_image_widget_append_check(image,
                                   "RAMLINKImageWrite",
                                   "Write image on detach/emulator exit");
    return image;
}


/** \brief  Create RAMLink widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ramlink_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *primary;
    GtkWidget *secondary;
    GtkWidget *label;
    GtkWidget *enable;
    GtkWidget *rtc_save;
    GtkWidget *mode;
    GtkWidget *size;
    GtkWidget *wrapper;
    int        row = 0;

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 16);

    /* create 'enable ramlink' checkbox */
    enable    = vice_gtk3_resource_check_button_new("RAMLINK",
                                                    "Enable " CARTRIDGE_NAME_RAMLINK
                                                    " emulation");
    primary   = create_primary_image_widget();
    secondary = create_secondary_image_widget();

    gtk_grid_attach(GTK_GRID(grid), enable,    0, row++, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), primary,   0, row++, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), secondary, 0, row++, 1, 1);

    /* add wrapper grid to avoid the layout engine adding too much space
     * between the widgets and their labels */
    wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(wrapper), 16);
    gtk_grid_set_row_spacing(GTK_GRID(wrapper), 8);

    /* create size widget */
    size  = vice_gtk3_resource_spin_int_new("RAMLINKsize", 0, 16, 1);
    label = label_helper(CARTRIDGE_NAME_RAMLINK " Size (MiB)");
    gtk_grid_attach(GTK_GRID(wrapper), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(wrapper), size,  1, 0, 1, 1);

    /* create mode widget */
    label = label_helper(CARTRIDGE_NAME_RAMLINK " mode");
    mode  = vice_gtk3_resource_radiogroup_new("RAMLINKmode",
                                              ramlink_modes,
                                              GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(wrapper), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(wrapper), mode,  1, 1, 1, 1);
    row++;

    /* create 'RTC Save' checkbox */
    rtc_save = vice_gtk3_resource_check_button_new("RAMLINKRTCSave",
                                                   "Save RTC data");
    gtk_grid_attach(GTK_GRID(wrapper), rtc_save, 0, 2, 2, 1);

    /* add wrapper to main grid */
    gtk_grid_attach(GTK_GRID(grid), wrapper, 0, row, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
