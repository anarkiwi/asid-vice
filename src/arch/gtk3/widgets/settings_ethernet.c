/** \file   settings_ethernet.c
 * \brief   GTK3 ethernet settings widget
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES ETHERNET_INTERFACE  x64 x64sc xscpu64 x128 xvic
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

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <stdbool.h>

#include "vice_gtk3.h"
#include "resources.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#ifdef HAVE_RAWNET
# include "rawnet.h"
#endif
#include "archdep_defs.h"
#include "uisettings.h"
#include "archdep_ethernet_available.h"


#include "settings_ethernet.h"


#ifdef HAVE_RAWNET
static void clean_iface_list(void);
static void clean_driver_list(void);
#endif



/** \brief  Handler for the 'destroy' event of the main widget
 *
 * \param[in]   widget  main widget (grid)
 * \param[in]   data    extra event data (unused)
 */
static void on_settings_ethernet_destroy(GtkWidget *widget, gpointer data)
{
#ifdef HAVE_RAWNET
    clean_iface_list();
    clean_driver_list();
#endif
}



#ifdef HAVE_RAWNET

/** \brief  List of available interfaces
 *
 * This list is dynamically generated and destroyed when the main widget
 * is destroyed.
 */
static vice_gtk3_combo_entry_str_t *iface_list;

/** \brief  List of available drivers
 *
 * This list is dynamically generated and destroyed when the main widget
 * is destroyed.
 */
static vice_gtk3_combo_entry_str_t *driver_list;


/** \brief  Build interface list for the combo box
 *
 * \return  bool
 */
static gboolean build_iface_list(void)
{
    int num = 0;
    char *if_name;
    char *if_desc;

    /* get number of adapters */
    if (!rawnet_enumadapter_open()) {
        return FALSE;
    }
    while (rawnet_enumadapter(&if_name, &if_desc)) {
        lib_free(if_name);
        if (if_desc != NULL) {
            lib_free(if_desc);
        }
        num++;
    }
    rawnet_enumadapter_close();

    /* allocate memory for list */
    iface_list = lib_malloc((size_t)(num + 1) * sizeof *iface_list);

    /* now add the list items */
    if (!rawnet_enumadapter_open()) {
        lib_free(iface_list);
        iface_list = NULL;
        return FALSE;
    }

    num = 0;
    while (rawnet_enumadapter(&if_name, &if_desc)) {
        iface_list[num].id = lib_strdup(if_name);
        /*
         * On Windows, the description string seems to be always present, on
         * Unix this isn't the case and NULL can be returned.
         */
        if (if_desc == NULL) {
            iface_list[num].name = lib_strdup(if_name);
        } else {
            iface_list[num].name = lib_msprintf("%s (%s)", if_name, if_desc);
        }
        lib_free(if_name);
        if (if_desc != NULL) {
            lib_free(if_desc);
        }

        num++;
    }
    iface_list[num].id = NULL;
    iface_list[num].name = NULL;
    rawnet_enumadapter_close();
    return TRUE;
}


/** \brief  Build driver list for the combo box
 *
 * \return  TRUE if the list was generated
 */
static gboolean build_driver_list(void)
{
    int num = 0;
    char *driver_name;
    char *driver_desc;

    /* get number of adapters */
    if (!rawnet_enumdriver_open()) {
        return FALSE;
    }
    while (rawnet_enumdriver(&driver_name, &driver_desc)) {
        lib_free(driver_name);
        if (driver_desc != NULL) {
            lib_free(driver_desc);
        }
        num++;
    }
    rawnet_enumdriver_close();

    /* allocate memory for list */
    driver_list = lib_malloc((size_t)(num + 1) * sizeof *driver_list);

    /* now add the list items */
    if (!rawnet_enumdriver_open()) {
        lib_free(driver_list);
        driver_list = NULL;
        return FALSE;
    }

    num = 0;
    while (rawnet_enumdriver(&driver_name, &driver_desc)) {
        driver_list[num].id = lib_strdup(driver_name);
        /*
         * On Windows, the description string seems to be always present, on
         * Unix this isn't the case and NULL can be returned.
         */
        if (driver_desc == NULL) {
            driver_list[num].name = lib_strdup(driver_name);
        } else {
            driver_list[num].name = lib_msprintf("%s (%s)", driver_name, driver_desc);
        }
        lib_free(driver_name);
        if (driver_desc != NULL) {
            lib_free(driver_desc);
        }

        num++;
    }
    driver_list[num].id = NULL;
    driver_list[num].name = NULL;
    rawnet_enumdriver_close();
    return TRUE;
}

/** \brief  Free memory used by the interface list
 */
static void clean_iface_list(void)
{
    if (iface_list != NULL) {
        int num = 0;
        while (iface_list[num].id != NULL) {
            lib_free(iface_list[num].id);
            lib_free(iface_list[num].name);
            num++;
        }
        lib_free(iface_list);
        iface_list = NULL;
    }
}

/** \brief  Free memory used by the driver list
 */
static void clean_driver_list(void)
{
    if (driver_list != NULL) {
        int num = 0;
        while (driver_list[num].id != NULL) {
            lib_free(driver_list[num].id);
            lib_free(driver_list[num].name);
            num++;
        }
        lib_free(driver_list);
        driver_list = NULL;
    }
}


static GtkWidget *create_driver_combo(void)
{
    GtkWidget *combo;

    if (build_driver_list()) {
        combo = vice_gtk3_resource_combo_box_str_new(
                "ETHERNET_DRIVER",
                driver_list);
    } else {
        combo = gtk_combo_box_text_new();
    }
    return combo;
}


/** \brief  Create combo box to select the ethernet interface
 *
 * \return  GtkComboBoxText
 */
static GtkWidget *create_device_combo(void)
{
    GtkWidget *combo;

    if (build_iface_list()) {
        combo = vice_gtk3_resource_combo_box_str_new("ETHERNET_INTERFACE",
                iface_list);
    } else {
        combo = gtk_combo_box_text_new();
    }

    return combo;
}
#endif


/** \brief  Create Ethernet settings widget for the settings UI
 *
 * \param[in]   parent  parent widget (ignored)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_ethernet_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *label;
    char *text;
#ifdef HAVE_RAWNET
    GtkWidget *iface_combo;
    GtkWidget *driver_combo;
    bool available = archdep_ethernet_available();
#endif

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    switch (machine_class) {
        case VICE_MACHINE_C64DTV:   /* fall through */
        case VICE_MACHINE_PLUS4:    /* fall through */
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_CBM5x0:   /* fall through */
        case VICE_MACHINE_CBM6x0:   /* fall through */
        case VICE_MACHINE_VSID:

            text = lib_msprintf(
                    "<b>Error</b>: Ethernet not supported for <b>%s</b>, "
                    "please fix the code that calls this code!",
                    machine_name);
            label = gtk_label_new(NULL);
            gtk_label_set_markup(GTK_LABEL(label), text);
            gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
            gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
            lib_free(text);
            gtk_widget_show_all(grid);
            return grid;
        default:
            break;
    }

#ifdef HAVE_RAWNET
    label = gtk_label_new("Ethernet driver:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    driver_combo = create_driver_combo();
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), driver_combo, 1, 0, 1, 1);

    label = gtk_label_new("Ethernet interface:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);

    iface_combo = create_device_combo();
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), iface_combo, 1, 1, 1, 1);

    if (!available) {
        gtk_widget_set_sensitive(iface_combo, FALSE);
        label = gtk_label_new(NULL);
# ifdef ARCHDEP_OS_UNIX
        gtk_label_set_markup(GTK_LABEL(label),
                "<i>VICE needs TUN/TAP support or the proper permissions (with libpcap) to be able to use ethernet emulation.</i>");
# elif defined(ARCHDEP_OS_WINDOWS)
        gtk_label_set_markup(GTK_LABEL(label),
                "<i><tt>wpcap.dll</tt> not found, please install WinPCAP to use ethernet emulation.</i>");
# else
        gtk_label_set_markup(GTK_LABEL(label),
                "<i>Ethernet emulation disabled due to unsupported OS.</i>");
# endif
        g_object_set(label, "margin-left", 16, NULL);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 2, 1);
    }




#else
    label = gtk_label_new("Ethernet not supported, please compile with "
            "--enable-ethernet.");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
#endif

    g_signal_connect(grid, "destroy", G_CALLBACK(on_settings_ethernet_destroy),
            NULL);

    gtk_widget_show_all(grid);
    return grid;
}
