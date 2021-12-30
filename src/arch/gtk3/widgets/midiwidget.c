/** \file   midiwidget.c
 * \brief   MIDI emulation settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES MIDIEnable      x64 x64sc xscpu64 x128 xvic
 * $VICERES MIDIMode        x64 x64sc xscpu64 x128 xvic
 * $VICERE$S MIDIDriver     x64 x64sc xscpu64 x128 xvic
 *  (Unix only)
 * $VICERES MIDIInDev       x64 x64sc xscpu64 x128 xvic
 * $VICERES MIDIOutDev      x64 x64sc xscpu64 x128 xvic
 * $VICERES MIDIName        x64 x64sc xscpu64 x128 xvic
 *  (MacOS only)
 * $VICEREs MIDIInName      x64 x64sc xscpu64 x128 xvic
 *  (MacOS only)
 * $VICERES MIDIOutName     x64 x64sc xscpu64 x128 xvic
 *  (MacOS only)
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

#ifdef HAVE_MIDI

#include <gtk/gtk.h>

#include "archdep_defs.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"
#include "vice_gtk3.h"

#include "midiwidget.h"


/** \brief  MIDI enable checkbutton */
static GtkWidget *midi_enable;
/** \brief  MIDI mode combobox */
static GtkWidget *midi_mode;
/** \brief  MIDI in device  */
static GtkWidget *midi_in_entry;
/** \brief  MIDI out device */
static GtkWidget *midi_out_entry;
#ifdef ARCHDEP_OS_UNIX
# ifdef ARCHDEP_OS_MACOS
/** \brief  MIDI name entry */
static GtkWidget *midi_name_entry;
# else
/** \brief  MIDI driver widget */
static GtkWidget *midi_driver;
/** \brief  MIDI in file browser */
static GtkWidget *midi_in_browse;
/** \brief  MIDI out file browser */
static GtkWidget *midi_out_browse;
# endif
#endif


/** \brief  Modes for MIDI support
 *
 * Seems to be a list of MIDI expansions
 */
static const vice_gtk3_combo_entry_int_t midi_modes[] = {
    { "Sequential",         0 },
    { "Passport/Syntech",   1 },
    { "DATEL/Siel/JMS",     2 },
    { "Namesoft",           4 },
    { "Maplin",             5 },
    { NULL, -1 }
};


#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)
/** \brief  List of MIDI drivers
 */
static const vice_gtk3_combo_entry_int_t midi_drivers[]= {
    { "OSS",    0 },
    { "ALSA",   1 },
    { NULL,     -1 }
};
#endif


/** \brief  Extra handler for the "toggled" event of the "Enable" check button
 *
 * \param[in]   widget      widget triggering the event
 * \param[in]   user_data   extra event data (unused)
 */
static void on_midi_enable_toggled(GtkWidget *widget, gpointer user_data)
{
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    gtk_widget_set_sensitive(midi_mode, state);
#ifdef ARCHDEP_OS_UNIX
# ifdef ARCHDEP_OS_MACOS
    gtk_widget_set_sensitive(midi_name_entry, state);
# else
    gtk_widget_set_sensitive(midi_driver, state);
# endif
#endif
    gtk_widget_set_sensitive(midi_in_entry, state);
    gtk_widget_set_sensitive(midi_out_entry, state);
#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)
    gtk_widget_set_sensitive(midi_in_browse, state);
    gtk_widget_set_sensitive(midi_out_browse, state);
#endif
}


#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)


static void midi_in_filename_callback(GtkDialog *dialog,
                                      gchar *filename,
                                      gpointer data)
{
    if (filename != NULL) {
        vice_gtk3_resource_entry_full_set(midi_in_browse, filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}



/** \brief  Handler for the "clicked" event of the MIDI-In "Browse" button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   text entry to store new filename
 */
static void on_midi_in_browse(GtkWidget *widget, gpointer user_data)
{
    const char *filters[] = { "mi*", NULL };

    vice_gtk3_open_file_dialog(
            "Select MIDI In device",
            "MIDI devices", filters, "/dev",
            midi_in_filename_callback,
            NULL);
}


static void midi_out_filename_callback(GtkDialog *dialog,
                                       gchar *filename,
                                       gpointer data)
{
    if (filename != NULL) {
        vice_gtk3_resource_entry_full_set(midi_out_browse, filename);
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the 'clicked' event of the MIDI-Out "Browse" button
 *
 * \param[in]   widget      button
 * \param[in]   user_data   text entry to store new filename
 */
static void on_midi_out_browse(GtkWidget *widget, gpointer user_data)
{
    const char *filters[] = { "mi*", NULL };

    vice_gtk3_open_file_dialog(
            "Select MIDI Out device",
            "MIDI devices", filters, "/dev",
            midi_out_filename_callback,
            NULL);
}
#endif


/** \brief  Create check button to enable/disable MIDI emulation
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_midi_enable_widget(void)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("MIDIEnable",
            "Enable MIDI emulation");
    g_signal_connect(check, "toggled", G_CALLBACK(on_midi_enable_toggled),
            NULL);
    return check;
}


/** \brief  Create MIDI emulation mode widget
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_midi_mode_widget(void)
{
    return vice_gtk3_resource_combo_box_int_new("MIDIMode", midi_modes);
}


#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)
/** \brief  Create MIDI driver selection widget
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_midi_driver_widget(void)
{
    return vice_gtk3_resource_combo_box_int_new("MIDIDriver", midi_drivers);
}
#endif


/** \brief  Create MIDI settings widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *midi_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    int row;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    midi_enable = create_midi_enable_widget();
    gtk_grid_attach(GTK_GRID(grid), midi_enable, 0, 0, 3, 1);

    label = gtk_label_new("MIDI mode");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    midi_mode = create_midi_mode_widget();
    gtk_grid_attach(GTK_GRID(grid), midi_mode, 1, 1, 1, 1);

    row = 2;

#ifdef ARCHDEP_OS_UNIX
# ifdef ARCHDEP_OS_MACOS
    label = gtk_label_new("MIDI Name");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    midi_name_entry = vice_gtk3_resource_entry_full_new("MIDIName");
    gtk_widget_set_hexpand(midi_name_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), midi_name_entry, 1, row, 1, 1);
# else
    label = gtk_label_new("MIDI driver");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    midi_driver = create_midi_driver_widget();
    gtk_grid_attach(GTK_GRID(grid), midi_driver, 1, 2, 1, 1);
# endif
    row++;
#endif

    /* TODO: seems like Windows uses a combobox with a list of drivers, so this
     *       code only works for Unix and OSX.
     */

    label = gtk_label_new("MIDI In");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
#ifdef ARCHDEP_OS_MACOS
    midi_in_entry = vice_gtk3_resource_entry_full_new("MIDIInName");
#else
    midi_in_entry = vice_gtk3_resource_entry_full_new("MIDIInDev");
#endif
    gtk_widget_set_hexpand(midi_in_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), midi_in_entry, 1, row, 1, 1);
#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)
    midi_in_browse = gtk_button_new_with_label("Browse ...");
    g_signal_connect(midi_in_browse, "clicked", G_CALLBACK(on_midi_in_browse),
            (gpointer)midi_in_entry);
    gtk_grid_attach(GTK_GRID(grid), midi_in_browse, 2, row, 1, 1);
#endif
    row++;

    label = gtk_label_new("MIDI Out");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
#ifdef ARCHDEP_OS_MACOS
    midi_out_entry = vice_gtk3_resource_entry_full_new("MIDIOutName");
#else
    midi_out_entry = vice_gtk3_resource_entry_full_new("MIDIOutDev");
#endif
    gtk_widget_set_hexpand(midi_out_entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), midi_out_entry, 1, row, 1, 1);
#if defined(ARCHDEP_OS_UNIX) && !defined(ARCHDEP_OS_MACOS)
    midi_out_browse = gtk_button_new_with_label("Browse ...");
    g_signal_connect(midi_out_browse, "clicked",
            G_CALLBACK(on_midi_out_browse), (gpointer)midi_out_entry);
    gtk_grid_attach(GTK_GRID(grid), midi_out_browse, 2, row, 1, 1);
#endif
    row++;

    on_midi_enable_toggled(midi_enable, NULL);

    gtk_widget_show_all(grid);
    return grid;
}

#endif /* HAVE_MIDI */
