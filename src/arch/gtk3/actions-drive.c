/** \file   actions-drive.c
 * \brief   UI action implementations for drive-related dialogs and settings
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
 */

#include "vice.h"

#include <gtk/gtk.h>
#include <stddef.h>
#include <stdbool.h>

#include "attach.h"
#include "debug_gtk3.h"
#include "drive.h"
#include "fliplist.h"
#include "uiactions.h"
#include "uidiskattach.h"
#include "uidiskcreate.h"
#include "uifliplist.h"
#include "uismartattach.h"
#include "uistatusbar.h"

#include "actions-drive.h"


/** \brief  Pop up smart attach dialog */
static void smart_attach_action(void)
{
    ui_smart_attach_dialog_show();
}

/* {{{ attach actions */
/** \brief  Pop up disk attach dialog for unit 8, drive 0 */
static void drive_attach_8_0_action(void)
{
    ui_disk_attach_dialog_show(8, 0);
}

/** \brief  Pop up disk attach dialog for unit 8, drive 1 */
static void drive_attach_8_1_action(void)
{
    ui_disk_attach_dialog_show(8, 1);
}

/** \brief  Pop up disk attach dialog for unit 9, drive 0 */
static void drive_attach_9_0_action(void)
{
    ui_disk_attach_dialog_show(9, 0);
}

/** \brief  Pop up disk attach dialog for unit 9, drive 1 */
static void drive_attach_9_1_action(void)
{
    ui_disk_attach_dialog_show(9, 1);
}

/** \brief  Pop up disk attach dialog for unit 10, drive 0 */
static void drive_attach_10_0_action(void)
{
    ui_disk_attach_dialog_show(10, 0);
}

/** \brief  Pop up disk attach dialog for unit 10, drive 1 */
static void drive_attach_10_1_action(void)
{
    ui_disk_attach_dialog_show(10, 1);
}

/** \brief  Pop up disk attach dialog for unit 11, drive 0 */
static void drive_attach_11_0_action(void)
{
    ui_disk_attach_dialog_show(11, 0);
}

/** \brief  Pop up disk attach dialog for unit 11, drive 1 */
static void drive_attach_11_1_action(void)
{
    ui_disk_attach_dialog_show(11, 0);
}
/* }}} */

/** \brief  Pop up dialog to create and attach a disk image */
static void drive_create_action(void)
{
    ui_disk_create_dialog_show(8);
}

/* {{{ detach actions */
/** \brief  Detach disk image from unit 8, drive 0 */
static void drive_detach_8_0_action(void)
{
    file_system_detach_disk(8, 0);
}

/** \brief  Detach disk image from unit 8, drive 1 */
static void drive_detach_8_1_action(void)
{
    file_system_detach_disk(8, 1);
}

/** \brief  Detach disk image from unit 9, drive 0 */
static void drive_detach_9_0_action(void)
{
    file_system_detach_disk(9, 0);
}

/** \brief  Detach disk image from unit 9, drive 1 */
static void drive_detach_9_1_action(void)
{
    file_system_detach_disk(9, 1);
}

/** \brief  Detach disk image from unit 10, drive 0 */
static void drive_detach_10_0_action(void)
{
    file_system_detach_disk(10, 0);
}

/** \brief  Detach disk image from unit 10, drive 1 */
static void drive_detach_10_1_action(void)
{
    file_system_detach_disk(10, 1);
}

/** \brief  Detach disk image from unit 11, drive 0 */
static void drive_detach_11_0_action(void)
{
    file_system_detach_disk(11, 0);
}

/** \brief  Detach disk image from unit 11, drive 1 */
static void drive_detach_11_1_action(void)
{
    file_system_detach_disk(11, 1);
}

/** \brief  Detach all disk images from all units and drives */
static void drive_detach_all_action(void)
{
    int unit;

    for (unit = DRIVE_UNIT_MIN; unit <= DRIVE_UNIT_MAX; unit++) {
        file_system_detach_disk(unit, 0);
        file_system_detach_disk(unit, 1);
    }
}
/* }}} */

/* {{{ drive reset actions */
/** \brief  Trigger reset of unit 8 */
static void reset_drive_8_action(void)
{
    drive_cpu_trigger_reset(0);
}

/** \brief  Trigger reset of unit 9 */
static void reset_drive_9_action(void)
{
    drive_cpu_trigger_reset(1);
}

/** \brief  Trigger reset of unit 10 */
static void reset_drive_10_action(void)
{
    drive_cpu_trigger_reset(2);
}

/** \brief  Trigger reset of unit 11 */
static void reset_drive_11_action(void)
{
    drive_cpu_trigger_reset(3);
}
/* }}} */

/* {{{ fliplist actions */

/** \brief  Size of buffer used for status bar messages */
#define MSGBUF_SIZE 1024

/** \brief  Add current image to fliplist
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 */
static void fliplist_add_helper(int unit, int drive)
{
    char buffer[MSGBUF_SIZE];

    if (fliplist_add_image(unit)) {
        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: added image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, 1);
    } else {
        /* Display proper error message once we have a decent
         * get_image_filename(unit) function which returns NULL on non-attached
         * images.
         */
        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: failed to add image to unit %d, drive %d.",
                   unit, drive);
        ui_display_statustext(buffer, 1);
    }
}

/** \brief  Add image in unit 8, drive 0 to fliplist */
static void fliplist_add_8_0_action(void)
{
    fliplist_add_helper(8, 0);
}

/** \brief  Add image in unit 9, drive 0 to fliplist */
static void fliplist_add_9_0_action(void)
{
    fliplist_add_helper(9, 0);
}

/** \brief  Add image in unit 10, drive 0 to fliplist */
static void fliplist_add_10_0_action(void)
{
    fliplist_add_helper(10, 0);
}

/** \brief  Add image in unit 11, drive 0 to fliplist */
static void fliplist_add_11_0_action(void)
{
    fliplist_add_helper(11, 0);
}


/** \brief  Remove current image from fliplist
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 */
static void fliplist_remove_helper(int unit, int drive)
{
    const char *image = fliplist_get_head(unit);
    if (image != NULL) {
        char buffer[MSGBUF_SIZE];

        fliplist_remove(unit, NULL);
        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: removed image from unit %d, drive %d: %s.",
                   unit, drive, image);
        ui_display_statustext(buffer, 1);
    } else {
        ui_display_statustext("Fliplist: nothing to remove.", 1);
    }
}

/** \brief  Remove current image in unit 8, drive 0 from the fliplist */
static void fliplist_remove_8_0_action(void)
{
    fliplist_remove_helper(8, 0);
}

/** \brief  Remove current image in unit 9, drive 0 from the fliplist */
static void fliplist_remove_9_0_action(void)
{
    fliplist_remove_helper(9, 0);
}

/** \brief  Remove current image in unit 10, drive 0 from the fliplist */
static void fliplist_remove_10_0_action(void)
{
    fliplist_remove_helper(10, 0);
}

/** \brief  Remove current image in unit 11, drive 0 from the fliplist */
static void fliplist_remove_11_0_action(void)
{
    fliplist_remove_helper(11, 0);
}


/** \brief  Attach next image in fliplist
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 */
static void fliplist_next_helper(int unit, int drive)
{
    char buffer[MSGBUF_SIZE];

    if (fliplist_attach_head(unit, 1)) {

        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: attached next image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, 1);
    } else {
        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: failed to attach next image to unit %d, drive %d.",
                   unit, drive);
        ui_display_statustext(buffer, 1);
    }
}

/** \brief  Attach next image in fliplist to unit 8 */
static void fliplist_next_8_0_action(void)
{
    fliplist_next_helper(8, 0);
}

/** \brief  Attach next image in fliplist to unit 9 */
static void fliplist_next_9_0_action(void)
{
    fliplist_next_helper(9, 0);
}

/** \brief  Attach next image in fliplist to unit 10 */
static void fliplist_next_10_0_action(void)
{
    fliplist_next_helper(10, 0);
}

/** \brief  Attach next image in fliplist to unit 11 */
static void fliplist_next_11_0_action(void)
{
    fliplist_next_helper(11, 0);
}


/** \brief  Attach next image in fliplist
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 */
static void fliplist_previous_helper(int unit, int drive)
{
    char buffer[MSGBUF_SIZE];

    if (fliplist_attach_head(unit, 0)) {

        g_snprintf(buffer, sizeof(buffer),
                   "Fliplist: attached previous image to unit %d, drive %d: %s.",
                   unit, drive, fliplist_get_head(unit));
        ui_display_statustext(buffer, 1);
    } else {
        g_snprintf(buffer, sizeof(buffer),
                  "Fliplist: failed to attach previous image to unit %d, drive %d.",
                  unit, drive);
        ui_display_statustext(buffer, 1);
    }
}

/** \brief  Attach previous image in fliplist to unit 8 */
static void fliplist_previous_8_0_action(void)
{
    fliplist_previous_helper(8, 0);
}

/** \brief  Attach previous image in fliplist to unit 9 */
static void fliplist_previous_9_0_action(void)
{
    fliplist_previous_helper(9, 0);
}

/** \brief  Attach previous image in fliplist to unit 10 */
static void fliplist_previous_10_0_action(void)
{
    fliplist_previous_helper(10, 0);
}

/** \brief  Attach previous image in fliplist to unit 11 */
static void fliplist_previous_11_0_action(void)
{
    fliplist_previous_helper(11, 0);
}


/** \brief  Clear fliplist
 *
 * \param[in]   unit    unit number (8-11)
 * \param[in]   drive   drive number (0-1)
 */
static void fliplist_clear_helper(int unit, int drive)
{
    char buffer[MSGBUF_SIZE];

    fliplist_clear_list(unit);
    g_snprintf(buffer, sizeof(buffer),
              "Fliplist: Cleared for unit %d, drive %d.",
              unit, drive);
    ui_display_statustext(buffer, 1);
}

/** \brief  Clear fliplist of unit 8 */
static void fliplist_clear_8_0_action(void)
{
    fliplist_clear_helper(8, 0);
}

/** \brief  Clear fliplist of unit 9 */
static void fliplist_clear_9_0_action(void)
{
    fliplist_clear_helper(9, 0);
}

/** \brief  Clear fliplist of unit 10 */
static void fliplist_clear_10_0_action(void)
{
    fliplist_clear_helper(10, 0);
}

/** \brief  Clear fliplist of unit 11 */
static void fliplist_clear_11_0_action(void)
{
    fliplist_clear_helper(11, 0);
}


/** \brief  Load fliplist from file for unit 8 */
static void fliplist_load_8_0_action(void)
{
    ui_fliplist_load_dialog_show(8);
}

/** \brief  Load fliplist from file for unit 9*/
static void fliplist_load_9_0_action(void)
{
    ui_fliplist_load_dialog_show(9);
}

/** \brief  Load fliplist from file for unit 10 */
static void fliplist_load_10_0_action(void)
{
    ui_fliplist_load_dialog_show(10);
}

/** \brief  Load fliplist from file for unit 11 */
static void fliplist_load_11_0_action(void)
{
    ui_fliplist_load_dialog_show(11);
}


/** \brief  Save fliplist to file for unit 8 */
static void fliplist_save_8_0_action(void)
{
    ui_fliplist_save_dialog_show(8);
}

/** \brief  Save fliplist to file for unit 9 */
static void fliplist_save_9_0_action(void)
{
    ui_fliplist_save_dialog_show(9);
}

/** \brief  Save fliplist to file for unit 10 */
static void fliplist_save_10_0_action(void)
{
    ui_fliplist_save_dialog_show(10);
}

/** \brief  Save fliplist to file for unit 11 */
static void fliplist_save_11_0_action(void)
{
    ui_fliplist_save_dialog_show(11);
}

/* }}} */


/** \brief  List of drive-related actions */
static const ui_action_map_t drive_actions[] = {
    /* Smart attach, technically not just disk-related, but let's put it here */
    {
        .action = ACTION_SMART_ATTACH,
        .handler = smart_attach_action,
        .blocks = true,
        .dialog = true
    },

    /* Attach disk actions */
    {
        .action = ACTION_DRIVE_ATTACH_8_0,
        .handler = drive_attach_8_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_8_1,
        .handler = drive_attach_8_1_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_9_0,
        .handler = drive_attach_9_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_9_1,
        .handler = drive_attach_9_1_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_10_0,
        .handler = drive_attach_10_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_10_1,
        .handler = drive_attach_10_1_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_11_0,
        .handler = drive_attach_11_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_DRIVE_ATTACH_11_1,
        .handler = drive_attach_11_1_action,
        .blocks = true,
        .dialog = true
    },

    /* Create and attach new image */
    {
        .action = ACTION_DRIVE_CREATE,
        .handler = drive_create_action,
        .blocks = true,
        .dialog = true
    },

    /* Detach disk actions */
    {
        .action = ACTION_DRIVE_DETACH_8_0,
        .handler = drive_detach_8_0_action
    },
    {
        .action = ACTION_DRIVE_DETACH_8_1,
        .handler = drive_detach_8_1_action
    },
    {
        .action = ACTION_DRIVE_DETACH_9_0,
        .handler = drive_detach_9_0_action
    },
    {
        .action = ACTION_DRIVE_DETACH_9_1,
        .handler = drive_detach_9_1_action
    },
    {
        .action = ACTION_DRIVE_DETACH_10_0,
        .handler = drive_detach_10_0_action
    },
    {
        .action = ACTION_DRIVE_DETACH_10_1,
        .handler = drive_detach_10_1_action
    },
    {
        .action = ACTION_DRIVE_DETACH_11_0,
        .handler = drive_detach_11_0_action
    },
    {
        .action = ACTION_DRIVE_DETACH_11_1,
        .handler = drive_detach_11_1_action
    },
    {
        .action = ACTION_DRIVE_DETACH_ALL,
        .handler = drive_detach_all_action
    },

    /* Drive reset actions */
    {
        .action = ACTION_RESET_DRIVE_8,
        .handler = reset_drive_8_action
    },
    {
        .action = ACTION_RESET_DRIVE_9,
        .handler = reset_drive_9_action
    },
    {
        .action = ACTION_RESET_DRIVE_10,
        .handler = reset_drive_10_action
    },
    {
        .action = ACTION_RESET_DRIVE_11,
        .handler = reset_drive_11_action
    },

    /* Fliplist actions
     *
     * Although the non-dialog actions display a message on the status bar,
     * they do not require to be run on the UI thread: the function
     * `ui_display_statustext()` can be called from any thread since the status
     * bar code has its own locking mechanism.
     */
    {
        .action = ACTION_FLIPLIST_ADD_8_0,
        .handler = fliplist_add_8_0_action
    },
    {
        .action = ACTION_FLIPLIST_ADD_9_0,
        .handler = fliplist_add_9_0_action
    },
    {
        .action = ACTION_FLIPLIST_ADD_10_0,
        .handler = fliplist_add_10_0_action
    },
    {
        .action = ACTION_FLIPLIST_ADD_11_0,
        .handler = fliplist_add_11_0_action
    },
    {
        .action = ACTION_FLIPLIST_REMOVE_8_0,
        .handler = fliplist_remove_8_0_action
    },
    {
        .action = ACTION_FLIPLIST_REMOVE_9_0,
        .handler = fliplist_remove_9_0_action
    },
    {
        .action = ACTION_FLIPLIST_REMOVE_10_0,
        .handler = fliplist_remove_10_0_action
    },
    {
        .action = ACTION_FLIPLIST_REMOVE_11_0,
        .handler = fliplist_remove_11_0_action
    },

    {
        .action = ACTION_FLIPLIST_NEXT_8_0,
        .handler = fliplist_next_8_0_action
    },
    {
        .action = ACTION_FLIPLIST_NEXT_9_0,
        .handler = fliplist_next_9_0_action
    },
     {
        .action = ACTION_FLIPLIST_NEXT_10_0,
        .handler = fliplist_next_10_0_action
    },
     {
        .action = ACTION_FLIPLIST_NEXT_11_0,
        .handler = fliplist_next_11_0_action
    },
    {
        .action = ACTION_FLIPLIST_PREVIOUS_8_0,
        .handler = fliplist_previous_8_0_action
    },
    {
        .action = ACTION_FLIPLIST_PREVIOUS_9_0,
        .handler = fliplist_previous_9_0_action
    },
    {
        .action = ACTION_FLIPLIST_PREVIOUS_10_0,
        .handler = fliplist_previous_10_0_action
    },
    {
        .action = ACTION_FLIPLIST_PREVIOUS_11_0,
        .handler = fliplist_previous_11_0_action
    },

    {
        .action = ACTION_FLIPLIST_CLEAR_8_0,
        .handler = fliplist_clear_8_0_action
    },
    {
        .action = ACTION_FLIPLIST_CLEAR_9_0,
        .handler = fliplist_clear_9_0_action
    },
    {
        .action = ACTION_FLIPLIST_CLEAR_10_0,
        .handler = fliplist_clear_10_0_action
    },
    {
        .action = ACTION_FLIPLIST_CLEAR_11_0,
        .handler = fliplist_clear_11_0_action
    },

    {
        .action = ACTION_FLIPLIST_LOAD_8_0,
        .handler = fliplist_load_8_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_LOAD_9_0,
        .handler = fliplist_load_9_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_LOAD_10_0,
        .handler = fliplist_load_10_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_LOAD_11_0,
        .handler = fliplist_load_11_0_action,
        .blocks = true,
        .dialog = true
    },

    {
        .action = ACTION_FLIPLIST_SAVE_8_0,
        .handler = fliplist_save_8_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_SAVE_9_0,
        .handler = fliplist_save_9_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_SAVE_10_0,
        .handler = fliplist_save_10_0_action,
        .blocks = true,
        .dialog = true
    },
    {
        .action = ACTION_FLIPLIST_SAVE_11_0,
        .handler = fliplist_save_11_0_action,
        .blocks = true,
        .dialog = true
    },

    UI_ACTION_MAP_TERMINATOR
};


/** \brief  Register drive-related actions */
void actions_drive_register(void)
{
    ui_actions_register(drive_actions);
}
