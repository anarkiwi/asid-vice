/** \file   ds12c887widget.c
 * \brief   DS12C887 RTC widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES DS12C887RTC         x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCbase     x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCSave     x64 x64sc xscpu64 x128 xvic
 * $VICERES DS12C887RTCRunMode  x64 x64sc xscpu64 x128 xvic
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
#include <stdlib.h>

#include "basedialogs.h"
#include "basewidgets.h"
#include "debug_gtk3.h"
#include "machine.h"
#include "resources.h"
#include "widgethelpers.h"

#include "ds12c887widget.h"


/** \brief  Values for I/O base on C64/C128
 *
 * The hardware appears to allow $d100, $d200 and $d300 as I/O-base but that's
 * not emulated and thus not in this list.
 */
static vice_gtk3_combo_entry_int_t c64_base[] = {
    { "$D500", 0xd500 },
    { "$D600", 0xd600 },
    { "$D700", 0xd700 },
    { "$DE00", 0xde00 },
    { "$DF00", 0xdf00 },
    { NULL, - 1 }
};


/** \brief  Values for I/O base on VIC-20
 */
static vice_gtk3_combo_entry_int_t vic20_base[] = {
    { "$9800", 0x9800 },
    { "$9C00", 0x9c00 },
    { NULL, - 1 }
};


/** \brief  Reference to the oscilattor checkbox */
static GtkWidget *oscil_widget = NULL;
/** \brief  Reference to the base widget */
static GtkWidget *base_widget = NULL;
/** \brief  Reference to the 'RTC' widget */
static GtkWidget *rtc_widget = NULL;


/** \brief  Handler for the "toggled" event of the enable widget
 *
 * \param[in,out]   widget      "enable" toggle button
 * \param[in]       user_data   extra event data (unused)
 */
static void on_enable_toggled(GtkWidget *widget, gpointer user_data)
{
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    gtk_widget_set_sensitive(oscil_widget, state);
    gtk_widget_set_sensitive(base_widget, state);
    gtk_widget_set_sensitive(rtc_widget, state);

}


/** \brief  Create widget to set I/O base for the RTC thing
 *
 * Sets the correct I/O base list for the current machine
 *
 * \return  GktComboBox
 */
static GtkWidget *create_base_widget(void)
{
    vice_gtk3_combo_entry_int_t *list;

    if (machine_class == VICE_MACHINE_VIC20) {
        list = vic20_base;
    } else {
        list = c64_base;
    }

    return vice_gtk3_resource_combo_box_int_new("DS12C887RTCbase", list);
}


/** \brief  Create widget to control the DS12C887 RTC
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *ds12c887_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *enable_widget;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    enable_widget = vice_gtk3_resource_check_button_new("DS12C887RTC",
            "Enable DS12C877 Real Time Clock");
    /* TODO: add event to enable/disable widgets */
    gtk_grid_attach(GTK_GRID(grid), enable_widget, 0, 0, 2, 1);

    oscil_widget = vice_gtk3_resource_check_button_new("DS12C887RTCRunMode",
            "Start with running oscillator");
    g_object_set(oscil_widget, "margin-left", 16, NULL);
    rtc_widget = vice_gtk3_resource_check_button_new("DS12C887RTCSave",
            "Enable RTC Saving");
    g_object_set(rtc_widget, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), oscil_widget, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), rtc_widget, 0, 2, 2, 1);

    label = gtk_label_new("Base address");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    base_widget = create_base_widget();
    gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), base_widget, 1, 3, 1, 1);

    g_signal_connect(enable_widget, "toggled", G_CALLBACK(on_enable_toggled),
            NULL);

    on_enable_toggled(enable_widget ,NULL);

    gtk_widget_show_all(grid);
    return grid;
}
