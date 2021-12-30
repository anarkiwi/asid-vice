/** \file   driverpmwidget.c
 * \brief   Drive RPM settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES Drive8RPM                -vsid
 * $VICERES Drive9RPM                -vsid
 * $VICERES Drive10RPM               -vsid
 * $VICERES Drive11RPM               -vsid
 * $VICERES Drive8WobbleFrequency    -vsid
 * $VICERES Drive9WobbleFrequency    -vsid
 * $VICERES Drive10WobbleFrequency   -vsid
 * $VICERES Drive11WobbleFrequency   -vsid
 * $VICERES Drive8WobbleAmplitude    -vsid
 * $VICERES Drive9WobbleAmplitude    -vsid
 * $VICERES Drive10WobbleAmplitude   -vsid
 * $VICERES Drive11WobbleAmplitude   -vsid
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
#include "drivewidgethelpers.h"
#include "machine.h"
#include "resources.h"

#include "driverpmwidget.h"


/* Please note I pulled the following values from my backside, so feel free to
 * alter them to more sensible values   -- compyx
 */

#define RPM_MIN     26000       /**< Drive RPM minimum */
#define RPM_MAX     34000       /**< Drive RPM maximum */
#define RPM_STEP    100         /**< Drive RPM stepping for the widget */

#define WOBBLE_FREQ_MIN  0      /**< Drive RPM wobble freq minimum */
#define WOBBLE_FREQ_MAX  10000  /**< Drive RPM wobble freq maximum */
#define WOBBLE_FREQ_STEP 10     /**< Drive RPM wobble freq stepping for the widget */

#define WOBBLE_AMP_MIN  0       /**< Drive RPM wobble amplitude minimum */
#define WOBBLE_AMP_MAX  5000    /**< Drive RPM wobble amplitude maximum */
#define WOBBLE_AMP_STEP 10      /**< Drive RPM wobble amplitude stepping for the widget */


/** \brief  Create widget to control drive RPM and wobble
 *
 * \param[in]   unit    unit number
 *
 * \return  GtkGrid
 */
GtkWidget *drive_rpm_widget_create(int unit)
{
    GtkWidget *grid;
    GtkWidget *rpm;
    GtkWidget *wobble_freq;
    GtkWidget *wobble_amp;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, "RPM settings", 2);
    g_object_set_data(G_OBJECT(grid), "UnitNumber", GINT_TO_POINTER(unit));

    /* RPM */
    label = gtk_label_new("RPM");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    rpm = vice_gtk3_resource_spin_int_new_sprintf("Drive%dRPM",
            RPM_MIN, RPM_MAX, RPM_STEP, unit);
    vice_gtk3_resource_spin_int_set_fake_digits(rpm, 2);
    g_object_set(rpm, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rpm, 1, 1, 1, 1);

    /* Wobble Frequency */
    label = gtk_label_new("Wobble Frequency");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    wobble_freq = vice_gtk3_resource_spin_int_new_sprintf("Drive%dWobbleFrequency",
            WOBBLE_FREQ_MIN, WOBBLE_FREQ_MAX, WOBBLE_FREQ_STEP, unit);
    vice_gtk3_resource_spin_int_set_fake_digits(wobble_freq, 0);
    g_object_set(wobble_freq, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), wobble_freq, 1, 2, 1, 1);

    /* Wobble Amplitude */
    label = gtk_label_new("Wobble Amplitude");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    wobble_amp = vice_gtk3_resource_spin_int_new_sprintf("Drive%dWobbleAmplitude",
            WOBBLE_AMP_MIN, WOBBLE_AMP_MAX, WOBBLE_AMP_STEP, unit);
    vice_gtk3_resource_spin_int_set_fake_digits(wobble_amp, 0);
    g_object_set(wobble_amp, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), wobble_amp, 1, 3, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


