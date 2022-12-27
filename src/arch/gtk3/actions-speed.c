/** \file   actions-speed.c
 * \brief   UI action implementations for speed-related settings
 *
 * UI actions handling warp mode, pause, CPU speed and FPS.
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

/* Resources altered by this file:
 *
 * $VICERES Speed   -vsid
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "basedialogs.h"
#include "debug_gtk3.h"
#include "hotkeymap.h"
#include "machine.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uimenu.h"
#include "vsync.h"
#include "vsyncapi.h"

#include "actions-speed.h"


/** \brief  Toggle pause
 *
 * Toggles pause and updates UI elements.
 */
static void pause_toggle_action(void)
{
    ui_pause_toggle();
    ui_set_check_menu_item_blocked_by_action(ACTION_PAUSE_TOGGLE,
                                             (gboolean)ui_pause_active());
    /* the pause LED gets updated in in the status bar update code */
}


/** \brief  Advance emulation a single frame if paused, pause otherwise */
static void advance_frame_action(void)
{
    if (ui_pause_active()) {
        vsyncarch_advance_frame();
    } else {
        ui_pause_enable();
        ui_set_check_menu_item_blocked_by_action(ACTION_PAUSE_TOGGLE, TRUE);
    }
}


/** \brief  Toggle warp mode
 *
 * Toggles warp mode and updates UI elements.
 */
static void warp_mode_toggle_action(void)
{
    vsync_set_warp_mode(!vsync_get_warp_mode());
    ui_set_check_menu_item_blocked_by_action(ACTION_WARP_MODE_TOGGLE,
                                             (gboolean)vsync_get_warp_mode());
}


/*
 * CPU speed and FPS
 */

/** \brief  Update main menu CPU speed radio buttons based on "Speed" resource
 */
static void update_cpu_radio_buttons(void)
{
    int action;
    int speed = 0;

    resources_get_int("Speed", &speed);
#if 0
    debug_gtk3("Speed = %d.", speed);
#endif
    /* Update main menu radio buttons */
    switch (speed) {
        case 200:
            action = ACTION_SPEED_CPU_200;
            break;
        case 100:
            action = ACTION_SPEED_CPU_100;
            break;
        case 50:
            action = ACTION_SPEED_CPU_50;
            break;
        case 20:
            action = ACTION_SPEED_CPU_20;
            break;
        case 10:
            action = ACTION_SPEED_CPU_10;
            break;
        default:
            action = ACTION_SPEED_CPU_CUSTOM;
            break;
    }

    /* the radio group takes care of disabling the other radio buttons, so we
     * only need to set the new active item */
#if 0
    debug_gtk3("Selecting action '%s'.", action);
#endif
    ui_set_check_menu_item_blocked_by_action(action, TRUE);
}


/** \brief  Update main menu FPS radio buttons based on "Speed" resource
 */
static void update_fps_radio_buttons(void)
{
    int action;
    int speed = 0;

    resources_get_int("Speed", &speed);
#if 0
    debug_gtk3("Speed = %d.", speed);
#endif
    switch (speed) {
        case 100:
            action = ACTION_SPEED_FPS_REAL;
            break;
        case -50:
            action = ACTION_SPEED_FPS_50;
            break;
        case -60:
            action = ACTION_SPEED_FPS_60;
            break;
        default:
            action = ACTION_SPEED_FPS_CUSTOM;
    }
#if 0
    debug_gtk3("Selecting action '%s'.", action);
#endif
    ui_set_check_menu_item_blocked_by_action(action, TRUE);
}


/** \brief  Set "Speed" resource and update UI elements
 *
 * Set new value for "Speed" resource: postive values are CPU speed in
 * percentage points (100 = 100%), negative values are FPS values (50 = 50Hz,
 * 60 = 60Hz), using 0 means 100% CPU and machine FPS (dictated by the hardware).
 *
 * \param[in]   speed   new value for the resource
 */
static void set_speed_resource(int speed)
{
    int oldval = 0;

    resources_get_int("Speed", &oldval);
    if (oldval != speed) {
        resources_set_int("Speed", speed);
        update_cpu_radio_buttons();
        update_fps_radio_buttons();
    }
}


/** \brief  Set CPU speed to 200%
 */
static void speed_cpu_200_action(void)
{
    set_speed_resource(200);
}

/** \brief  Set CPU speed to 100%
 */
static void speed_cpu_100_action(void)
{
    set_speed_resource(100);
}

/** \brief  Set CPU speed to 50%
 */
static void speed_cpu_50_action(void)
{
    set_speed_resource(50);
}

/** \brief  Set CPU speed to 20%
 */
static void speed_cpu_20_action(void)
{
    set_speed_resource(20);
}

/** \brief  Set CPU speed to 10%
 */
static void speed_cpu_10_action(void)
{
    set_speed_resource(10);
}


/** \brief  Callback for custom speed
 *
 * \param[in]   dialog  integer-dialog reference
 * \param[in]   result  result from the dialog
 * \param[in]   valid   \a result is valid
 */
static void speed_cpu_custom_callback(GtkDialog *dialog,
                                      int result,
                                      gboolean valid)
{
    if (valid) {
        set_speed_resource(result);
    }
    /* notify the action system that the action (and its dialog) has finished */
    ui_action_finish(ACTION_SPEED_CPU_CUSTOM);
}


/** \brief  Set custom CPU speed
 *
 * Pops up a dialog to set a custom emulation speed.
 */
static void speed_cpu_custom_action(void)
{
    GtkWidget *widget;

    widget = ui_get_menu_item_by_action_for_window(ACTION_SPEED_CPU_CUSTOM,
                                                   ui_get_main_window_index());
    if (widget != NULL) {
        int curval = 0;

        resources_get_int("Speed", &curval);

        vice_gtk3_integer_input_box(
                speed_cpu_custom_callback,
                "Set new emulation speed",
                "Enter a new custom emulation speed",
                curval,
                1, 100000);
    } else {
        debug_gtk3("Failed to get menu item for action %d (%s).",
                   ACTION_SPEED_CPU_CUSTOM,
                   ui_action_get_name(ACTION_SPEED_CPU_CUSTOM));
    }
}


/** \brief  Set FPS to machine (real) FPS
 */
static void speed_fps_real_action(void)
{
    set_speed_resource(100);
}


/** \brief  Set FPS to exactly 50 FPS
 */
static void speed_fps_50_action(void)
{
    set_speed_resource(-50);
}

/** \brief  Set FPS to exactly 60 FPS
 */
static void speed_fps_60_action(void)
{
    set_speed_resource(-60);
}


/** \brief  Callback for custom FPS target
 *
 * \param[in]   dialog  integer-dialog reference
 * \param[in]   result  result from the dialog
 * \param[in]   valid   \a result is valid
 */
static void fps_custom_callback(GtkDialog *dialog, int result, gboolean valid)
{
    if (valid) {
        set_speed_resource(0 - result);
    }
    /* notify the action system that the action (and its dialog) has finished */
    ui_action_finish(ACTION_SPEED_FPS_CUSTOM);
}


/** \brief  Set FPS to a custom value using a dialog
 *
 * Pops up a dialog to set a custom FPS.
 */
static void speed_fps_custom_action(void)
{
    GtkWidget *widget;

    /* only show the dialog when the radio/check button is toggled ON */
    widget = ui_get_menu_item_by_action_for_window(ACTION_SPEED_FPS_CUSTOM,
                                                   ui_get_main_window_index());
    if (widget != NULL) {
        int curval = 0;

        resources_get_int("Speed", &curval);
        if (curval > 0) {
            curval = 50;
        } else {
            curval = 0 - curval;
        }

        vice_gtk3_integer_input_box(
                fps_custom_callback,
                "Set new FPS target",
                "Enter a new custom FPS target",
                curval,
                1, 100000);
    } else {
        debug_gtk3("Failed to get menu item for action %d (%s).",
                   ACTION_SPEED_FPS_CUSTOM,
                   ui_action_get_name(ACTION_SPEED_FPS_CUSTOM));
    }
}


/** \brief  Speed-related UI action mappings
 */
static const ui_action_map_t speed_actions[] = {
    {
        .action = ACTION_PAUSE_TOGGLE,
        .handler = pause_toggle_action,
        .uithread = true

    },
    {
        .action = ACTION_ADVANCE_FRAME,
        .handler = advance_frame_action,
        .uithread = true
    },
    {
        .action = ACTION_WARP_MODE_TOGGLE,
        .handler = warp_mode_toggle_action,
        .uithread = true
    },

    /* CPU speed actions */
    {
        .action = ACTION_SPEED_CPU_200,
        .handler = speed_cpu_200_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_CPU_100,
        .handler = speed_cpu_100_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_CPU_50,
        .handler = speed_cpu_50_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_CPU_20,
        .handler = speed_cpu_20_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_CPU_10,
        .handler = speed_cpu_10_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_CPU_CUSTOM,
        .handler = speed_cpu_custom_action,
        .blocks = true,
        .dialog = true
    },

    /* FPS actions */
    {
        .action = ACTION_SPEED_FPS_REAL,
        .handler = speed_fps_real_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_FPS_50,
        .handler = speed_fps_50_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_FPS_60,
        .handler = speed_fps_60_action,
        .uithread = true
    },
    {
        .action = ACTION_SPEED_FPS_CUSTOM,
        .handler = speed_fps_custom_action,
        .blocks = true,
        .dialog = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register speed-related UI actions
 */
void actions_speed_register(void)
{
    ui_actions_register(speed_actions);
}


/** \brief  Set the correct radio buttons and set "${EMU} FPS" label
 */
void actions_speed_setup_ui(void)
{
    GtkWidget *item;
    char buffer[256];

    /* set '$MACHINE FPS' label */
    g_snprintf(buffer, sizeof(buffer), "%s FPS", machine_get_name());
    item = ui_get_menu_item_by_action_for_window(ACTION_SPEED_FPS_REAL,
                                                 PRIMARY_WINDOW);
    if (item != NULL) {
        gtk_menu_item_set_label(GTK_MENU_ITEM(item), buffer);
    }
    if (machine_class == VICE_MACHINE_C128) {
        item = ui_get_menu_item_by_action_for_window(ACTION_SPEED_FPS_REAL,
                                                     SECONDARY_WINDOW);
        if (item != NULL) {
            gtk_menu_item_set_label(GTK_MENU_ITEM(item), buffer);
        }
    }

    /* activate correct radio buttons */
    update_cpu_radio_buttons();
    update_fps_radio_buttons();

    /* pause */
    ui_set_check_menu_item_blocked_by_action(ACTION_PAUSE_TOGGLE,
                                             (gboolean)ui_pause_active());

    /* warp */
    ui_set_check_menu_item_blocked_by_action(ACTION_WARP_MODE_TOGGLE,
                                             (gboolean)vsync_get_warp_mode());

}
