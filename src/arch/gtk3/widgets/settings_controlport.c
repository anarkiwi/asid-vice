/** \file   settings_controlport.c
 * \brief   Widget to control settings for control ports
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES JoyPort1Device  x64 x64sc x64dtv xscpu64 x128 xcbm5x0 xplus4 xvic
 * $VICERES JoyPort2Device  x64 x64sc x64dtv xscpu64 x128 xcbm5x0 xplus4
 * $VICERES JoyPort3Device  x64 x64sc x64dtv xscpu64 x128 xcbm2 xvic
 * $VICERES JoyPort4Device  x64 x64sc xscpu64 x128 xcbm2 xpet xvice
 * $VICERES JoyPort5Device  xplus4
 * $VICERES BBRTCSave
 * $VICERES ps2mouse            x64dtv
 * $VICERES SmartMouseRTCSave   x64 x64sc xscpu64 x128 xvic xplus4 xcbm5x0
 * $VICERES UserportJoy     -xcbm5x0 -vsid
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

#include "vice_gtk3.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "joyport.h"
#include "uisettings.h"

#include "settings_controlport.h"


/*
 * Forward declarations
 */

static void joyport_devices_list_shutdown(void);
static void free_combo_list(int port);


/** \brief  Lists of valid devices for each joyport
 */
static joyport_desc_t *joyport_devices[JOYPORT_MAX_PORTS];


/** \brief  Combo box entry lists for each joyport
 */
static vice_gtk3_combo_entry_int_t *joyport_combo_lists[JOYPORT_MAX_PORTS];



/** \brief  Handler for the "destroy" event of the main widget
 *
 * \param[in]   widget      the main widget (unused)
 * \param[in]   user_data   extra data (unused)
 */
static void on_destroy(GtkWidget *widget, gpointer user_data)
{
    int port;

    joyport_devices_list_shutdown();
    for (port = 0; port < JOYPORT_MAX_PORTS; port++) {
        free_combo_list(port);
    }
}



/** \brief  Dynamically generate a list of joyport devices for \a port
 *
 * \param[in]   port    port number
 *
 * \return  TRUE if the list was generated
 */
static gboolean create_combo_list(int port)
{
    int num;
    int i;
    joyport_desc_t *dev;

    dev = joyport_devices[port];
    if (dev == NULL) {
        joyport_combo_lists[port] = NULL;
        return FALSE;
    }

    /* calculate size of list to create */
    num = 0;
    while (dev->name != NULL) {
        dev++;
        num++;
    }

    /* allocate memory for list */
    joyport_combo_lists[port] = lib_malloc((size_t)(num + 1) *
            sizeof *joyport_combo_lists[port]);

    /* populate list */
    i = 0;
    dev = joyport_devices[port];
    while (dev->name != NULL) {
        joyport_combo_lists[port][i].name = dev->name;
        joyport_combo_lists[port][i].id = dev->id;
        dev++;
        i++;
    }
    /* terminate list */
    joyport_combo_lists[port][i].name = NULL;
    joyport_combo_lists[port][i].id = -1;
    return TRUE;
}


/** \brief  Free memory used by the combo box entry list for \a port
 *
 * \param[in]   port    index in the combo box lists (0 == JoyPort1Device)
 */
static void free_combo_list(int port)
{
    if (joyport_combo_lists[port] != NULL) {
        lib_free(joyport_combo_lists[port]);
    }
}


/** \brief  Retrieve valid devices for each joyport
 *
 * joyport_get_valid_devices() returns an empty list for unsupported devices,
 * so no need to check for machine type.
 */
static void joyport_devices_list_init(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        joyport_devices[i] = joyport_get_valid_devices(i, 1);
    }
}


/** \brief  Clean up memory used by the valid devices list
 */
static void joyport_devices_list_shutdown(void)
{
    int i;

    for (i = 0; i < JOYPORT_MAX_PORTS; i++) {
        if (joyport_devices[i] != NULL) {
            lib_free(joyport_devices[i]);
            joyport_devices[i] = NULL;
        }
    }
}



/** \brief  Create combo box for joyport \a port
 *
 * \param[in]   port    Joyport number (0-4, 0 == JoyPort1Device)
 * \param[in]   title   widget title
 *
 * \return  GtkGrid
 */
static GtkWidget *create_joyport_widget(int port, const char *title)
{
    GtkWidget *grid;
    GtkWidget *combo;

    /* generate combo box list */
    if (!create_combo_list(port)) {
        log_error(LOG_ERR,
                "failed to generate joyport devices list for port %d",
                port + 1);
        return NULL;
    }

    grid = vice_gtk3_grid_new_spaced_with_label(-1, -1, title, 1);

    combo = vice_gtk3_resource_combo_box_int_new_sprintf(
            "JoyPort%dDevice",
            joyport_combo_lists[port],
            port + 1);
    g_object_set(combo, "margin-left", 16, NULL);
    gtk_widget_set_hexpand(combo, TRUE);

    gtk_grid_attach(GTK_GRID(grid), combo, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}



/** \brief  Add widgets for the control ports
 *
 * Adds \a count comboboxes to select the emulated device for the control ports.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_control_ports(GtkGrid *layout, int row, int count)
{
    if (count <= 0) {
        return row;
    }

    gtk_grid_attach(layout,
                    create_joyport_widget(JOYPORT_1, "Control Port #1"),
                    0, row, 1, 1);
    if (count > 1) {
        gtk_grid_attach(layout,
                        create_joyport_widget(JOYPORT_2, "Control Port #2"),
                        1, row, 1, 1);
    }

    return row + 1;
}


/** \brief  Add widgets for the joystick adapter ports
 *
 * Adds \a count comboboxes to select the emulated device for the adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 * \param[in]       count   number of widgets to add to \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_adapter_ports(GtkGrid *layout, int row, int count)
{
    int i;
    int r = row;
    int c = 0;
    int d = JOYPORT_3;

    for (i = 0; i < count; i++, d++) {

        char label[256];

        g_snprintf(label, sizeof(label), "Extra Joystick #%d", i + 1);
        gtk_grid_attach(layout,
                        create_joyport_widget(d, label),
                        c, r, 1, 1);
        c ^= 1; /* swap column */
        if (c == 0) {
            r++;
        }
    }

    return r + 1 + c;
}


/** \brief  Add widget for the Plus4 SIDCart joystick port
 *
 * Adds a widget for the Plus4-specific SIDCard extra joystick port.
 *
 * Currently the resource "JoyPort5Device" is used for this widget, but once
 * eight adapter ports are implemented for Plus4 like the other emulators, we
 * probably should switch the widget to "JoyPort11Device".
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     starting row in \a layout
 *
 * \return  row in the \a layout for additional widgets
 */
static int layout_add_sidcard_port(GtkGrid *layout, int row)
{
    gtk_grid_attach(layout,
                    create_joyport_widget(JOYPORT_5, "SIDCard Joystick Port"),
                    0, row, 1, 1);
    return row + 1;
}


/** \brief  Add checkbox for the battery-backed RTC save option
 *
 * Add a checkbox for the "BBRTCSave" resource.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_bbrtc_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("BBRTCSave",
            "Save battery-backed real time clock data when changed");
    g_object_set(check, "margin-top", 16, NULL);
    gtk_grid_attach(layout, check, 0, row, 2, 1);

    return row + 1;
}


/** \brief  Add checkbox for the SmartMouse RTC save option
 *
 * Add a checkbox for the "SmartMouseRTCSave" resource.
 *
 * Valid for x64, x64sc, xscpu64, x128, xcbm5x0 and xvic.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_smartmouse_rtc_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("SmartMouseRTCSave",
            "Enable SmartMouse RTC Saving");
    gtk_grid_attach(layout, check, 0, row, 2, 1);

    return row + 1;
}


/** \brief  Add checkbox for the userport PS/2 mouse
 *
 * Add a checkbox for the "ps2mouse" resource.
 *
 * Valid for x64dtv.
 *
 * \param[in,out]   layout  main widget grid
 * \param[in]       row     row in \a layout to add the checkbox
 *
 * \return  row in the \a layout for additional widgets
 *
 * \note    the added widget spans two columns in the layout
 */
static int layout_add_ps2mouse_widget(GtkGrid *layout, int row)
{
    GtkWidget *check;

    check = vice_gtk3_resource_check_button_new("ps2mouse",
            "Enable PS/2 mouse on Userport");
    gtk_grid_attach(layout, check, 0, row, 2, 1);

    return row + 1;
}



/*
 * Functions to create the layouts for the various emulators
 */

/** \brief  Create layout for x64, x64sc, xscpu64 and x128
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_c64_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}


/** \brief  Create layout for x64dtv
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_c64dtv_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_ps2mouse_widget(layout, row);

    return row;
}



/** \brief  Create layout for xvic
 *
 * One control port and eight joystick adapter ports.
 *
 * \param[in,out]   layout    main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_vic20_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 1);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}


/** \brief  Create layout for xplus4
 *
 * Two control ports, two userport adapter ports and one SIDCard control port.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_plus4_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 2);
    row = layout_add_sidcard_port(layout, row);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}


/** \brief  Create layout for xpet
 *
 * Two userport adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_pet_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_adapter_ports(layout, row, 2);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}


/** \brief  Create layout for xcbm5x0
 *
 * Two control ports and eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm5x0_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_control_ports(layout, row, 2);
    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);
    row = layout_add_smartmouse_rtc_widget(layout, row);

    return row;
}


/** \brief  Create layout for xcbm2
 *
 * Eight joystick adapter ports.
 *
 * \param[in,out]   layout  main widget grid
 *
 * \return  row in the \a layout for additional widgets
 */
static int create_cbm6x0_layout(GtkGrid *layout)
{
    int row = 0;

    row = layout_add_adapter_ports(layout, row, 8);
    row = layout_add_bbrtc_widget(layout, row);

    return row;
}



/** \brief  Create widget to control control ports
 *
 * Creates a widget to control the settings for the control ports, userport
 * joystick adapter ports and the SIDCard control port, depending on the
 * currently emulated machine.
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_controlport_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;

    joyport_devices_list_init();

    layout = vice_gtk3_grid_new_spaced(16, 8);

    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_SCPU64:   /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
            create_c64_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_C64DTV:
            create_c64dtv_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VIC20:
            create_vic20_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PLUS4:
            create_plus4_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_PET:
            create_pet_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM5x0:
            create_cbm5x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_CBM6x0:
            create_cbm6x0_layout(GTK_GRID(layout));
            break;
        case VICE_MACHINE_VSID: /* fallthrough */
        default:
            debug_gtk3("Warning: should never get here!");
            break;
    }

    g_signal_connect_unlocked(layout, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show_all(layout);
    return layout;
}
