/** \file   userportprinterwidget.c
 * \brief   Widget to control userport printer
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* The machines mentioned here may not be correct:
 *
 * $VICERES UserportDevice              -vsid
 * $VICERES PrinterUserPort             -vsid
 * $VICERES PrinterUserportDriver       -vsid
 * $VICERES PrinterUserportOutput       -vsid
 * $VICERES PrinterUserportTextDevice   -vsid
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vice_gtk3.h"
#include "resources.h"
#include "printer.h"
#include "userport.h"

#include "userportprinterwidget.h"


/** \brief  List of text output devices
 */
static const vice_gtk3_radiogroup_entry_t text_devices[] = {
    { "#1", 0 },
    { "#2", 1 },
    { "#3", 2 },
    { NULL, -1 }
};


/** \brief  Handler for the "toggled" event of the driver radio buttons
 *
 * \param[in]   radio       radio button
 * \param[in]   user_data   new value for the resource (`const char *`)
 */
static void on_driver_toggled(GtkRadioButton *radio, gpointer user_data)
{

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
        const char *driver = (const char *)user_data;

        resources_set_string("PrinterUserportDriver", driver);
    }
}


/** \brief  Handler for the "toggled" event of the output mode widget
 *
 * \param[in]   radio       radio button
 * \param[in]   user_data   new value for the resource (`const char *`)
 */
static void on_output_mode_toggled(GtkRadioButton *radio, gpointer user_data)
{

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
        const char *mode = (const char *)user_data;

        resources_set_string("PrinterUserportOutput", mode);
    }
}


/** \brief  Handler for the 'toggled' event of the userport emulation checkbox
 *
 * \param[in]   self    checkbox
 * \param[in]   data    extra event data (unused)
 */
static void on_userport_emulation_toggled(GtkWidget *self, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self))) {
        resources_set_int("UserportDevice", USERPORT_DEVICE_PRINTER);
    } else {
        resources_set_int("UserportDevice", USERPORT_DEVICE_NONE);
    }
}


/** \brief  Create checkbox to control the UserportDevice resource
 *
 * Set UserportDevice to either PRINTER or NONE.
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_userport_emulation_widget(void)
{
    GtkWidget *check;
    int device;

    if (resources_get_int("UserportDevice", &device) < 0) {
        device = USERPORT_DEVICE_PRINTER;
    }

    check = gtk_check_button_new_with_label("Enable userport printer emulation");
    g_object_set(check, "margin-left", 16, NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                device == USERPORT_DEVICE_PRINTER);
    g_signal_connect(check,
                     "toggled",
                     G_CALLBACK(on_userport_emulation_toggled),
                     NULL);

    return check;
}


/** \brief  Create printer driver selection widget
 *
 * Creates a group of radio buttons to select the driver of the Userport
 * printer.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_driver_widget(void)
{
    GtkWidget *grid;
    GtkWidget *radio_ascii = NULL;
    GtkWidget *radio_nl10 = NULL;
    GtkWidget *radio_raw = NULL;
    GSList *group = NULL;
    const char *driver;

    /* build grid */
    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "Driver", 1);
    /* set DeviceNumber property to allow the update function to work */

    /* ASCII */
    radio_ascii = gtk_radio_button_new_with_label(group, "ASCII");
    g_object_set(radio_ascii, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_ascii, 0, 1, 1, 1);

    /* NL10 */
    radio_nl10 = gtk_radio_button_new_with_label(group, "NL10");
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_nl10),
            GTK_RADIO_BUTTON(radio_ascii));
    g_object_set(radio_nl10, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_nl10, 0, 3, 1, 1);

    /* RAW */
    radio_raw = gtk_radio_button_new_with_label(group, "RAW");
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_raw),
            GTK_RADIO_BUTTON(radio_nl10));
    g_object_set(radio_raw, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_raw, 0, 4, 1, 1);

    /* set current driver from resource */
    resources_get_string("PrinterUserPortDriver", &driver);
    /* printer_driver_widget_update(grid, driver); */

    /* connect signal handlers */
    g_signal_connect(radio_raw, "toggled", G_CALLBACK(on_driver_toggled),
            (gpointer)"raw");
    g_signal_connect(radio_ascii, "toggled", G_CALLBACK(on_driver_toggled),
            (gpointer)"ascii");
    g_signal_connect(radio_nl10, "toggled", G_CALLBACK(on_driver_toggled),
            (gpointer)"nl10");

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create userport printer output mode widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_output_mode_widget(void)
{
    GtkWidget *grid;
    GtkWidget *radio_text;
    GtkWidget *radio_gfx;
    GSList *group = NULL;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "Output mode", 1);

    radio_text = gtk_radio_button_new_with_label(group, "Text");
    g_object_set(radio_text, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_text, 0, 1, 1, 1);

    radio_gfx = gtk_radio_button_new_with_label(group, "Graphics");
    g_object_set(radio_gfx, "margin-left", 16, NULL);
    gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio_gfx),
            GTK_RADIO_BUTTON(radio_text));
    gtk_grid_attach(GTK_GRID(grid), radio_gfx, 0, 2, 1, 1);


    g_signal_connect(radio_text, "toggled", G_CALLBACK(on_output_mode_toggled),
            (gpointer)"text");
    g_signal_connect(radio_gfx, "toggled", G_CALLBACK(on_output_mode_toggled),
            (gpointer)"graphics");

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create text output device selection widget for Userport printer
 *
 * \return  GtkGrid
 */
static GtkWidget *create_text_device_widget(void)
{
    GtkWidget *grid;
    GtkWidget *group;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "Output device", 1);
    group = vice_gtk3_resource_radiogroup_new(
            "PrinterUserPortTextDevice",
            text_devices,
            GTK_ORIENTATION_VERTICAL);
    g_object_set(group, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), group, 0, 1, 1, 1);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget to control Userport printer settings
 *
 * \return  GtkGrid
 */
GtkWidget *userport_printer_widget_create(void)
{
    GtkWidget *grid;

    grid = vice_gtk3_grid_new_spaced_with_label(
            -1, -1, "Userport printer settings", 3);

    gtk_grid_attach(GTK_GRID(grid), create_userport_emulation_widget(),
            0, 1, 3, 1);

    gtk_grid_attach(GTK_GRID(grid), create_driver_widget(),
            0, 2, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), create_output_mode_widget(),
            1, 2, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), create_text_device_widget(),
            2, 2,1,1);

    gtk_widget_show_all(grid);
    return grid;
}
