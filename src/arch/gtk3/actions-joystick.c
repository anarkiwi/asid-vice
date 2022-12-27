/** \file   actions-joystick.c
 * \brief   UI action implementations for joysticks and mouse
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

/* The following resources are manipulated:
 *
 * $VICERES JoyDevice1      -vsid -xvic
 * $VICERES JoyDevice2      -vsid -xvic
 * $VICERES JoyPort1Device  -vsid -xvic
 * $VICERES JoyPort2Device  -vsid -xvic
 * $VICERES KeySetEnable    -vsid
 * $VICERES Mouse           -vsid
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "hotkeymap.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"

#include "actions-joystick.h"


/** \brief  Joysticks swapped state
 */
static bool controlport_swapped = false;


/** \brief  Get controlport swapped state
 *
 * \return  `true` if swapped
 */
bool ui_get_controlport_swapped(void)
{
    return controlport_swapped;
}


/** \brief  Swap joysticks and their associated devices */
static void swap_controlport_toggle_action(void)
{
    int joy1 = -1;
    int joy2 = -1;
    int type1 = -1;
    int type2 = -1;

    resources_get_int("JoyPort1Device", &type1);
    resources_get_int("JoyPort2Device", &type2);

    /* unset both resources first to avoid assigning for example the mouse to
     * two ports. here might be dragons!
     */
    resources_set_int("JoyPort1Device", 0);
    resources_set_int("JoyPort2Device", 0);

    /* try setting port #2 first, some devices only work in port #1 */
    if (resources_set_int("JoyPort2Device", type1) < 0) {
        /* restore config */
        resources_set_int("JoyPort1Device", type1);
        resources_set_int("JoyPort2Device", type2);
        return;
    }
    if (resources_set_int("JoyPort1Device", type2) < 0) {
        /* restore config */
        resources_set_int("JoyPort1Device", type1);
        resources_set_int("JoyPort2Device", type2);
        return;
    }

    resources_get_int("JoyDevice1", &joy1);
    resources_get_int("JoyDevice2", &joy2);
    resources_set_int("JoyDevice1", joy2);
    resources_set_int("JoyDevice2", joy1);

    controlport_swapped = !controlport_swapped;

    ui_set_check_menu_item_blocked_by_action(ACTION_SWAP_CONTROLPORT_TOGGLE,
                                             controlport_swapped);
}

/** \brief  Toggle keyset joysticks */
static void keyset_joystick_toggle_action(void)
{
    int enable;

    resources_get_int("KeySetEnable", &enable);
    resources_set_int("KeySetEnable", !enable);

    ui_set_check_menu_item_blocked_by_action(ACTION_KEYSET_JOYSTICK_TOGGLE,
                                             !enable);
}

/** \brief  Toggle mouse grab */
static void mouse_grab_toggle_action(void)
{
    GtkWidget *window;
    gchar title[256];
    int mouse;

    resources_get_int("Mouse", &mouse);
    resources_set_int("Mouse", !mouse);
    mouse = !mouse;

    if (mouse) {
        gchar *name = hotkey_map_get_accel_label_for_action(ACTION_MOUSE_GRAB_TOGGLE);
        g_snprintf(title, sizeof(title),
                   "VICE (%s) (Use %s to disable mouse grab)",
                   machine_get_name(), name);
        g_free(name);
    } else {
       g_snprintf(title, sizeof(title), "VICE (%s)", machine_get_name());
    }

    window = ui_get_main_window_by_index(PRIMARY_WINDOW);
    if (window != NULL) {
        gtk_window_set_title(GTK_WINDOW(window), title);
    }
    window = ui_get_main_window_by_index(SECONDARY_WINDOW);
    if (window != NULL) {
        gtk_window_set_title(GTK_WINDOW(window), title);
    }

    ui_set_check_menu_item_blocked_by_action(ACTION_MOUSE_GRAB_TOGGLE, mouse);
}


/** \brief  List of joystick/mouse avtions */
static const ui_action_map_t joystick_actions[] = {
    {
        .action = ACTION_SWAP_CONTROLPORT_TOGGLE,
        .handler = swap_controlport_toggle_action,
        .uithread = true
    },
    {
        .action = ACTION_KEYSET_JOYSTICK_TOGGLE,
        .handler = keyset_joystick_toggle_action,
        .uithread = true
    },
    {
        .action = ACTION_MOUSE_GRAB_TOGGLE,
        .handler = mouse_grab_toggle_action,
        .uithread = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register joystick/mouse actions */
void actions_joystick_register(void)
{
    ui_actions_register(joystick_actions);
}


/** \brief  Set joystick/mouse-related check buttons
 *
 * Set check buttons for 'controlport-swapped', mouse-grab' and 'keyset-joystick'.
 */
void actions_joystick_setup_ui(void)
{
    int enabled;

    /* mouse grab */
    resources_get_int("Mouse", &enabled);
    ui_set_check_menu_item_blocked_by_action(ACTION_MOUSE_GRAB_TOGGLE,
                                             (gboolean)enabled);

    /* swap joysticks */
    if (ui_action_is_valid(ACTION_SWAP_CONTROLPORT_TOGGLE)) {
        ui_set_check_menu_item_blocked_by_action(ACTION_SWAP_CONTROLPORT_TOGGLE,
                                                 (gboolean)controlport_swapped);
    }

    /* keyset joysticks */
    resources_get_int("KeySetEnable", &enabled);
    ui_set_check_menu_item_blocked_by_action(ACTION_KEYSET_JOYSTICK_TOGGLE,
                                             (gboolean)enabled);
}
