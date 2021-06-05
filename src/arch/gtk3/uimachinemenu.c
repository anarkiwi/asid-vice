/** \file   uimachinemenu.c
 * \brief   Native GTK3 menus for machine emulators (not vsid)
 *
 * \author  Marcus Sutton <loggedoubt@gmail.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES WarpMode        all
 * $VICERES KeySetEnable    -vsid
 * $VICERES Mouse           -vsid
 * $VICERES DtvBlitterLog   x64dtv
 * $VICERES DtvDMALog       x64dtv
 * $VICERES DtvFlashLog     x64dtv
 *
 * These resources are only available with --enable-debug
 *
 * $VICERES MainCPU_TRACE   all
 * $VICERES IEC_TRACE       -vsid
 * $VICERES IEEE_TRACE      -vsid
 * $VICERES Drive0CPU_TRACE -vsid
 * $VICERES Drive1CPU_TRACE -vsid
 * $VICERES Drive2CPU_TRACE -vsid
 * $VICERES Drive3CPU_TRACE -vsid
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

#include <stdlib.h>
#include <gtk/gtk.h>

#include "vice_gtk3.h"
#include "archdep.h"
#include "debug_gtk3.h"
#include "datasette.h"
#include "debug.h"
#include "machine.h"
#include "mainlock.h"
#include "resources.h"
#include "ui.h"
#include "uiabout.h"
#include "uicart.h"
#include "uicmdline.h"
#include "uicommands.h"
#include "uicompiletimefeatures.h"
#include "uidatasette.h"
#include "uidebug.h"
#include "uidiskattach.h"
#include "uiedit.h"
#include "uifliplist.h"
#include "uihotkeys.h"
#include "uimachinemenu.h"
#include "uimedia.h"
#include "uimenu.h"
#include "uimonarch.h"
#include "uidiskcreate.h"
#include "uitapecreate.h"
#include "uisettings.h"
#include "uismartattach.h"
#include "uisnapshot.h"
#include "uitapeattach.h"

/*
 * The following are translation unit local so we can create functions that
 * modify menu contents or even functions that alter the top bar itself.
 */


/** \brief  Main menu bar widget
 *
 * Contains the submenus on the menu main bar
 *
 * This one lives until ui_exit() or thereabouts
 */
static GtkWidget *main_menu_bar = NULL;


/** \brief  File submenu
 */
static GtkWidget *file_submenu = NULL;


/** \brief  Edit submenu
 */
static GtkWidget *edit_submenu = NULL;


/** \brief  Snapshot submenu
 */
static GtkWidget *snapshot_submenu = NULL;


/** \brief  Settings submenu
 */
static GtkWidget *settings_submenu = NULL;


#ifdef DEBUG
/** \brief  Debug submenu, only available when --enable-debug was specified
 */
static GtkWidget *debug_submenu = NULL;
#endif


/** \brief  Help submenu
 */
static GtkWidget *help_submenu = NULL;


/** \brief  Load settings from default file
 *
 * \param[in]   widget  menu item triggering the event (ignored)
 * \param[in]   data    extra even data (ignored)
 *
 * \return  TRUE
 */
static gboolean settings_load_callback(GtkWidget *widget, gpointer data)
{
    if (resources_reset_and_load(NULL) != 0) {
        vice_gtk3_message_error("VICE core error",
                "Failed to load default settings file");
    }
    return TRUE;
}


/** \brief  Calllback for the load-settings dialog
 *
 * \param[in,out]   dialog      dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        mode (0: reset and load, 1: add extra settings)
 */
static void settings_load_filename_callback(GtkDialog *dialog,
                                            gchar *filename,
                                            gpointer data)
{
    int res;
    if (filename!= NULL) {
        mainlock_obtain();
        res = (data == NULL) ? resources_reset_and_load(filename)
                             : resources_load(filename);
        if (res != 0) {
            vice_gtk3_message_error("VICE core error",
                    "Failed to load settings from '%s'", filename);
        }
        mainlock_release();
        g_free(filename);
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Load settings from user-specified file
 *
 * \param[in]   widget  menu item triggering the event (ignored)
 * \param[in]   data    mode (0: reset and load, 1: add extra settings)
 *
 * \return  TRUE
 */
static gboolean settings_load_custom_callback(GtkWidget *widget, gpointer data)
{
    vice_gtk3_open_file_dialog(
            "Load settings file",
            NULL, NULL, NULL,
            settings_load_filename_callback,
            data);
    return TRUE;
}


/** \brief  Save settings to default file
 *
 * \param[in]   widget  menu item triggering the event (ignored)
 * \param[in]   data    extra even datat (ignored)
 *
 * \return  TRUE to signal the event has been handled
 */
static gboolean settings_save_callback(GtkWidget *widget, gpointer data)
{
    if (resources_save(NULL) != 0) {
        vice_gtk3_message_error("VICE core error",
                "Failed to save default settings file");
    }
    return TRUE;
}


/** \brief  Callback for the save-custom-settings dialog
 *
 * \param[in,out]   dialog      save-file dialog
 * \param[in,out]   filename    filename
 * \param[in]       data        extra data (unused)
 */
static void on_settings_save_custom_filename(GtkDialog *dialog,
                                             gchar *filename,
                                             gpointer data)
{
    if (filename!= NULL) {
        mainlock_obtain();
        if (resources_save(filename) != 0) {
            vice_gtk3_message_error("VICE core error",
                    "Failed to save settings as '%s'", filename);
        }
        mainlock_release();
        g_free(filename);
    }
    mainlock_release();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    mainlock_obtain();

}


/** \brief  Save settings to user-specified file
 *
 * \param[in]   widget  menu item triggering the event (ignored)
 * \param[in]   data    extra even data (ignored)
 *
 * \return  TRUE
 */
static gboolean settings_save_custom_callback(GtkWidget *widget, gpointer data)
{
    vice_gtk3_save_file_dialog(
            "Save settings as ...",
            NULL, TRUE, NULL,
            on_settings_save_custom_filename,
            NULL);

    return TRUE;
}


/** \brief  Encode unit and drive number for a detach callback argument
 *
 * Encodes unit and drive number into a value to be used in the
 * ui_disk_detach_callback() calls.
 *
 * \param[in]   U   unit number (8-11)
 * \param[in]   D   drive number (0 or 1)
 */
#define UNIT_DRIVE_TO_PTR(U, D) GINT_TO_POINTER(((U) << 8) | ((D) & 0xff))


/** \brief  File->Detach disk submenu
 */
static ui_menu_item_t detach_submenu[] = {
    { "Drive 8:0", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive8:0", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(8, 0),
        GDK_KEY_8, VICE_MOD_MASK|GDK_CONTROL_MASK, false },
    { "Drive 8:1", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive8:1", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(8, 1),
        0, 0, false },
    { "Drive 9:0", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive9:0", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(9, 0),
        GDK_KEY_9, VICE_MOD_MASK|GDK_CONTROL_MASK, false },
    { "Drive 9:1", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive9:0", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(9, 1),
        0, 0, false },
    { "Drive 10:0", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive10", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(10, 0),
        GDK_KEY_0, VICE_MOD_MASK|GDK_CONTROL_MASK, false },
    { "Drive 10:1", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive10", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(10, 1),
        0, 0, false },
    { "Drive 11:0", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive11", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(11, 0),
        GDK_KEY_1, VICE_MOD_MASK|GDK_CONTROL_MASK },
    { "Drive 11:1", UI_MENU_TYPE_ITEM_ACTION,
        "detach-drive11", ui_disk_detach_callback, UNIT_DRIVE_TO_PTR(11, 1),
        0, 0, false },
    /* Detach all, do we even have code to do that? */
    { "Detach all", UI_MENU_TYPE_ITEM_ACTION,
        "detach-all", ui_disk_detach_all_callback, NULL,
        0, 0, false },

    UI_MENU_TERMINATOR
};


/** \brief  File->Attach disk submenu
 */
static ui_menu_item_t attach_submenu[] = {
    { "Drive #8", UI_MENU_TYPE_ITEM_ACTION,
        "attach-drive8", ui_disk_attach_dialog_show, GINT_TO_POINTER(8),
        GDK_KEY_8, VICE_MOD_MASK, true },
    { "Drive #9", UI_MENU_TYPE_ITEM_ACTION,
        "attach-drive9", ui_disk_attach_dialog_show, GINT_TO_POINTER(9),
        GDK_KEY_9, VICE_MOD_MASK, true },
    { "Drive #10", UI_MENU_TYPE_ITEM_ACTION,
        "attach-drive10", ui_disk_attach_dialog_show, GINT_TO_POINTER(10),
        GDK_KEY_0, VICE_MOD_MASK, true },
    { "Drive #11", UI_MENU_TYPE_ITEM_ACTION,
        "attach-drive11", ui_disk_attach_dialog_show, GINT_TO_POINTER(11),
        GDK_KEY_1, VICE_MOD_MASK, true },

    UI_MENU_TERMINATOR
};



/** \brief  File->Flip list submenu
 */
static ui_menu_item_t fliplist_submenu[] = {
    { "Add current image (Unit #8)", UI_MENU_TYPE_ITEM_ACTION,
        "fliplist-add", ui_fliplist_add_current_cb, GINT_TO_POINTER(8),
        GDK_KEY_I, VICE_MOD_MASK, false },
    { "Remove current image (Unit #8)", UI_MENU_TYPE_ITEM_ACTION,
       "fliplist-remove", ui_fliplist_remove_current_cb, GINT_TO_POINTER(8),
        GDK_KEY_K, VICE_MOD_MASK, false },
    { "Attach next image (Unit #8)", UI_MENU_TYPE_ITEM_ACTION,
        "fliplist-next", ui_fliplist_next_cb, GINT_TO_POINTER(8),
        GDK_KEY_N, VICE_MOD_MASK, false },
    { "Attach previous image (Unit #8)", UI_MENU_TYPE_ITEM_ACTION,
        "fliplist-prev", ui_fliplist_prev_cb, GINT_TO_POINTER(8),
        GDK_KEY_N, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Load flip list file...", UI_MENU_TYPE_ITEM_ACTION,
        "fliplist-load", ui_fliplist_load_callback, GINT_TO_POINTER(8),
        0, 0, true },
    { "Save flip list file...", UI_MENU_TYPE_ITEM_ACTION,
        "fliplist-save", ui_fliplist_save_callback, GINT_TO_POINTER(8),
        0, 0, true },

    UI_MENU_TERMINATOR
};

/** \brief  File->Datasette control submenu
 */

static ui_menu_item_t datasette_control_submenu[] = {
    { "Stop", UI_MENU_TYPE_ITEM_ACTION,
        "tape-stop", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_STOP),
        0, 0, false },
    { "Start", UI_MENU_TYPE_ITEM_ACTION,
        "tape-start", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_START),
        0, 0, false },
    { "Forward", UI_MENU_TYPE_ITEM_ACTION,
        "tape-ff", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_FORWARD),
        0, 0, false },
    { "Rewind", UI_MENU_TYPE_ITEM_ACTION,
        "tape-rew", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_REWIND),
        0, 0, false },
    { "Record", UI_MENU_TYPE_ITEM_ACTION,
        "tape-record", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_RECORD),
        0, 0, false },
    { "Reset", UI_MENU_TYPE_ITEM_ACTION,
        "tape-reset", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_RESET),
        0, 0, false },
    { "Reset Counter", UI_MENU_TYPE_ITEM_ACTION,
        "tape-counter", ui_datasette_tape_action_cb, GINT_TO_POINTER(DATASETTE_CONTROL_RESET_COUNTER),
        0, 0, false },
    UI_MENU_TERMINATOR
};


/** \brief  File->Reset submenu
 */
static ui_menu_item_t reset_submenu[] = {
    { "Soft reset", UI_MENU_TYPE_ITEM_ACTION,
        "reset-soft", ui_machine_reset_callback, GINT_TO_POINTER(MACHINE_RESET_MODE_SOFT),
        GDK_KEY_F9, VICE_MOD_MASK, false },
    { "Hard reset", UI_MENU_TYPE_ITEM_ACTION,
        "reset-hard", ui_machine_reset_callback, GINT_TO_POINTER(MACHINE_RESET_MODE_HARD),
        GDK_KEY_F12, VICE_MOD_MASK, false },

    UI_MENU_SEPARATOR,

    { "Reset drive #8", UI_MENU_TYPE_ITEM_ACTION,
        "reset-drive8", ui_drive_reset_callback, GINT_TO_POINTER(8),
        0, 0, false },
    { "Reset drive #9", UI_MENU_TYPE_ITEM_ACTION,
        "reset-drive9", ui_drive_reset_callback, GINT_TO_POINTER(9),
        0, 0, false },
    { "Reset drive #10", UI_MENU_TYPE_ITEM_ACTION,
        "reset-drive10", ui_drive_reset_callback, GINT_TO_POINTER(10),
        0, 0, false },
    { "Reset drive #11", UI_MENU_TYPE_ITEM_ACTION,
        "reset-drive11", ui_drive_reset_callback, GINT_TO_POINTER(11),
        0, 0, false },

    UI_MENU_TERMINATOR
};


/** \brief  'File' menu - head section
 */
static ui_menu_item_t file_menu_head[] = {
    { "Smart attach disk/tape/cartridge ...", UI_MENU_TYPE_ITEM_ACTION,
        "smart-attach", ui_smart_attach_dialog_show, NULL,
        GDK_KEY_A, VICE_MOD_MASK, true },

    UI_MENU_SEPARATOR,

    /* disk */
    { "Attach disk image", UI_MENU_TYPE_SUBMENU,
        NULL, NULL, attach_submenu,
        GDK_KEY_8, VICE_MOD_MASK, false },
    { "Create and attach an empty disk image ...", UI_MENU_TYPE_ITEM_ACTION,
        "create-disk", ui_disk_create_dialog_show, GINT_TO_POINTER(8),
        0, 0, true },
    { "Detach disk image", UI_MENU_TYPE_SUBMENU,
        NULL, NULL, detach_submenu,
        0, 0, false },
    { "Flip list", UI_MENU_TYPE_SUBMENU,
        NULL, NULL, fliplist_submenu,
        0, 0, false },

    UI_MENU_SEPARATOR,

    UI_MENU_TERMINATOR
};


/** \brief  'File' menu - tape section pointer
 *
 * Set by ...
 */
static ui_menu_item_t *file_menu_tape_section = NULL;


/** \brief  'File' menu - tape section
 */
static ui_menu_item_t file_menu_tape[] = {
    { "Attach tape image ...", UI_MENU_TYPE_ITEM_ACTION,
        "attach-tape", ui_tape_attach_callback, NULL,
        GDK_KEY_T, VICE_MOD_MASK, true },
    { "Create and attach an empty tape image ...", UI_MENU_TYPE_ITEM_ACTION,
        "create-tape", ui_tape_create_dialog_show, NULL,
        0, 0, true },
    { "Detach tape image", UI_MENU_TYPE_ITEM_ACTION,
        "detach-tape", ui_tape_detach_callback, NULL,
        0, 0, false },
    { "Datasette controls", UI_MENU_TYPE_SUBMENU,
        NULL, NULL, datasette_control_submenu,
        0, 0, false },

    UI_MENU_SEPARATOR,

    UI_MENU_TERMINATOR
};


/** \brief  'File' menu - tail section
 */
static ui_menu_item_t file_menu_tail[] = {
    /* cart */
    { "Attach cartridge image ...", UI_MENU_TYPE_ITEM_ACTION,
        "cart-attach", ui_cart_show_dialog, NULL,
        GDK_KEY_C, VICE_MOD_MASK, true },
    { "Detach cartridge image(s)", UI_MENU_TYPE_ITEM_ACTION,
        "cart-detach", (void *)ui_cart_detach, NULL,
        GDK_KEY_C, VICE_MOD_MASK|GDK_SHIFT_MASK, false },
    { "Cartridge freeze", UI_MENU_TYPE_ITEM_ACTION,
        "cart-freeze", (void *)ui_cart_trigger_freeze, NULL,
        GDK_KEY_Z, VICE_MOD_MASK, false },

    UI_MENU_SEPARATOR,

    /* monitor */
    { "Activate monitor", UI_MENU_TYPE_ITEM_ACTION,
        "monitor", ui_monitor_activate_callback, NULL,
#ifdef MACOSX_SUPPORT
        /* use Command-Option-M on Mac */
        GDK_KEY_M, VICE_MOD_MASK | GDK_MOD1_MASK,
#else
        GDK_KEY_H, VICE_MOD_MASK,
#endif
        false
    },

    UI_MENU_SEPARATOR,

    { "Reset", UI_MENU_TYPE_SUBMENU,
        NULL, NULL, reset_submenu,
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "Exit emulator", UI_MENU_TYPE_ITEM_ACTION,
        "exit", ui_close_callback, NULL,
        GDK_KEY_Q, VICE_MOD_MASK, true },

    UI_MENU_TERMINATOR
};


/** \brief  'Edit' menu
 */
static ui_menu_item_t edit_menu[] = {
    { "Copy", UI_MENU_TYPE_ITEM_ACTION,
        "copy", (void *)ui_copy_callback, NULL,
        GDK_KEY_Delete, VICE_MOD_MASK, false },
    { "Paste", UI_MENU_TYPE_ITEM_ACTION,
        "paste", (void *)ui_paste_callback, NULL,
        GDK_KEY_Insert, VICE_MOD_MASK, false },

    UI_MENU_TERMINATOR
};


/** \brief  'Snapshot' menu
 */
static ui_menu_item_t snapshot_menu[] = {
    { "Load snapshot image ...", UI_MENU_TYPE_ITEM_ACTION,
        "snapshot-load", ui_snapshot_open_file, NULL,
        GDK_KEY_L, VICE_MOD_MASK, false },
    { "Save snapshot image ...", UI_MENU_TYPE_ITEM_ACTION,
        "snapshot-save", ui_snapshot_save_file, NULL,
        GDK_KEY_S, VICE_MOD_MASK, false },

    UI_MENU_SEPARATOR,

    { "Quickload snapshot", UI_MENU_TYPE_ITEM_ACTION,
        "snapshot-quickload", ui_snapshot_quickload_snapshot, NULL,
        GDK_KEY_F10, VICE_MOD_MASK, false },   /* Shortcut doesn't work in MATE, key
                                         is mapped to Maximize Window. Using
                                         the menu to active this item does
                                         work though -- compyx */
    { "Quicksave snapshot", UI_MENU_TYPE_ITEM_ACTION,
        "snapshot-quicksave", ui_snapshot_quicksave_snapshot, NULL,
        GDK_KEY_F11, VICE_MOD_MASK, false },

    UI_MENU_SEPARATOR,
#if 0
    { "Select history directory ...", UI_MENU_TYPE_ITEM_ACTION,
        "history-select-dir", ui_snapshot_history_select_dir, "0:3",
        0, 0, false },
#endif
    { "Start recording events", UI_MENU_TYPE_ITEM_ACTION,
        "history-record-start", ui_snapshot_history_record_start, NULL,
        0, 0, false },
    { "Stop recording events", UI_MENU_TYPE_ITEM_ACTION,
        "history-record-stop", ui_snapshot_history_record_stop, NULL,
        0, 0, false },
    { "Start playing back events", UI_MENU_TYPE_ITEM_ACTION,
        "history-playback-start", ui_snapshot_history_playback_start, NULL,
        0, 0, false },
    { "Stop playing back events", UI_MENU_TYPE_ITEM_ACTION,
        "history-playback-stop", ui_snapshot_history_playback_stop, NULL,
        0, 0, false },
    { "Set recording milestone", UI_MENU_TYPE_ITEM_ACTION,
        "history-milestone-set", ui_snapshot_history_milestone_set, NULL,
        GDK_KEY_E, VICE_MOD_MASK, false },
    { "Return to milestone", UI_MENU_TYPE_ITEM_ACTION,
        "history-milestone-reset", ui_snapshot_history_milestone_reset, NULL,
        GDK_KEY_U, VICE_MOD_MASK, false },

    UI_MENU_SEPARATOR,
#if 0
    { "Recording start mode ...", UI_MENU_TYPE_ITEM_ACTION,
        "history-recording-start-mode", ui_settings_dialog_create, "20,0",
        0, 0, true },

    UI_MENU_SEPARATOR,
#endif

    { "Save media file ...", UI_MENU_TYPE_ITEM_ACTION,
        "media-save", ui_media_dialog_show, NULL,
        GDK_KEY_R, VICE_MOD_MASK | GDK_SHIFT_MASK, false },

    { "Stop media recording", UI_MENU_TYPE_ITEM_ACTION,
        "media-stop", (void *)ui_media_stop_recording, NULL,
        GDK_KEY_S, VICE_MOD_MASK | GDK_SHIFT_MASK, false },

    UI_MENU_TERMINATOR
};


/** \brief  'Settings' menu - head section
 */
static ui_menu_item_t settings_menu_head[] = {
    { "Toggle fullscreen", UI_MENU_TYPE_ITEM_ACTION,
        "fullscreen", ui_fullscreen_callback, NULL,
        GDK_KEY_D, VICE_MOD_MASK, true },
    { "Restore display state", UI_MENU_TYPE_ITEM_ACTION,
        "restore-display", (void *)ui_restore_display, NULL,
        GDK_KEY_r, VICE_MOD_MASK, true },
#if 1
    { "Show menu/status in fullscreen", UI_MENU_TYPE_ITEM_ACTION,
        "fullscreen-widgets", ui_fullscreen_decorations_callback, NULL,
        GDK_KEY_B, VICE_MOD_MASK, true },
#else
    /* Mac menubar version */
    { "Show statusbar in fullscreen", UI_MENU_TYPE_ITEM_ACTION,
        "fullscreen-widgets", ui_fullscreen_decorations_callback, NULL,
        GDK_KEY_B, VICE_MOD_MASK, false },
#endif

#if 0
    UI_MENU_SEPARATOR,
    { "Toggle warp mode", UI_MENU_TYPE_ITEM_CHECK,
        "warp", (void *)(ui_toggle_resource), (void *)"WarpMode",
        GDK_KEY_W, VICE_MOD_MASK, false },
    { "Pause emulation", UI_MENU_TYPE_ITEM_CHECK,
        "pause", (void *)(ui_toggle_pause), NULL,
        GDK_KEY_P, VICE_MOD_MASK, false },
    { "Advance frame", UI_MENU_TYPE_ITEM_ACTION,
        "frame-advance", (void *)(ui_advance_frame), NULL,
        GDK_KEY_P, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
#endif
    UI_MENU_TERMINATOR
};


/** \brief  'Settings' menu - joystick section pointer
 *
 * Set by ...
 */
static ui_menu_item_t *settings_menu_joy_section = NULL;


/** \brief  'Settings' menu - all joystick items
 *
 * Only valid for x64/x64sc/xscpu64/x128/xplus4
 */
static ui_menu_item_t settings_menu_all_joy[] = {

#if 0
    { "Swap joysticks", UI_MENU_TYPE_ITEM_ACTION,
        "joystick-swap", (void *)(ui_swap_joysticks_callback), NULL,
        GDK_KEY_J, VICE_MOD_MASK, false },
    { "Swap userport joysticks", UI_MENU_TYPE_ITEM_ACTION,
        "userportjoy-swap", (void *)(ui_swap_userport_joysticks_callback), NULL,
        GDK_KEY_U, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Allow keyset joystick", UI_MENU_TYPE_ITEM_CHECK,
        "keyset", (void *)(ui_toggle_resource), (void *)"KeySetEnable",
        GDK_KEY_J, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Enable mouse grab", UI_MENU_TYPE_ITEM_CHECK,
        "mouse", (void *)(ui_toggle_resource), (void *)"Mouse",
        GDK_KEY_M, VICE_MOD_MASK, false },

#endif
    UI_MENU_TERMINATOR
};

/** \brief  'Settings' menu - control port joystick items
 *
 * Only valid for x64dtv/xcbm5x0
 */
static ui_menu_item_t settings_menu_cbm5x0_joy[] = {
#if 0
    { "Swap joysticks", UI_MENU_TYPE_ITEM_ACTION,
        "joystick-swap", (void *)(ui_swap_joysticks_callback), NULL,
        GDK_KEY_J, VICE_MOD_MASK, false },
    { "Allow keyset joystick", UI_MENU_TYPE_ITEM_CHECK,
        "keyset", (void *)(ui_toggle_resource), (void *)"KeySetEnable",
        GDK_KEY_J, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Enable mouse grab", UI_MENU_TYPE_ITEM_CHECK,
        "mouse", (void *)(ui_toggle_resource), (void *)"Mouse",
        GDK_KEY_M, VICE_MOD_MASK, false },

#endif
    UI_MENU_TERMINATOR
};

/** \brief  'Settings' menu - userport joystick items
 *
 * Only valid for xvic/xpet/xcbm2
 */
static ui_menu_item_t settings_menu_userport_joy[] = {
#if 0
    { "Swap userport joysticks", UI_MENU_TYPE_ITEM_ACTION,
        "userportjoy-swap", (void *)(ui_swap_userport_joysticks_callback), NULL,
        GDK_KEY_U, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Allow keyset joystick", UI_MENU_TYPE_ITEM_CHECK,
        "keyset", (void *)(ui_toggle_resource), (void *)"KeySetEnable",
        GDK_KEY_J, VICE_MOD_MASK | GDK_SHIFT_MASK, false },
    { "Enable mouse grab", UI_MENU_TYPE_ITEM_CHECK,
        "mouse", (void *)(ui_toggle_resource), (void *)"Mouse",
        GDK_KEY_M, VICE_MOD_MASK, false },

#endif
    UI_MENU_TERMINATOR
};

/** \brief  'Settings' menu tail section
 */
static ui_menu_item_t settings_menu_tail[] = {

    UI_MENU_SEPARATOR,

    /* the settings dialog */
    { "Settings ...", UI_MENU_TYPE_ITEM_ACTION,
        "settings", ui_settings_dialog_create_and_activate_node_callback, NULL,
        GDK_KEY_O, VICE_MOD_MASK, true },
    { "Load settings", UI_MENU_TYPE_ITEM_ACTION,
        "settings-load", settings_load_callback, NULL,
        0, 0, false },
    { "Load settings from ...", UI_MENU_TYPE_ITEM_ACTION,
        "settings-load-custom", settings_load_custom_callback, NULL,
        0, 0, true },
    { "Load extra settings from ...", UI_MENU_TYPE_ITEM_ACTION,
        "settings-load-extra", settings_load_custom_callback, (void*)1,
        0, 0, true },
    { "Save settings", UI_MENU_TYPE_ITEM_ACTION,
        "settings-save", settings_save_callback, NULL,
        0, 0, false },
    { "Save settings to ...", UI_MENU_TYPE_ITEM_ACTION,
        "settings-save-custom", settings_save_custom_callback, NULL,
        0, 0, true },
    { "Restore default settings", UI_MENU_TYPE_ITEM_ACTION,
        "settings-restore", (void *)ui_restore_default_settings, NULL,
        0, 0, true },
    UI_MENU_TERMINATOR
};


#ifdef DEBUG

/** \brief  'Debug' menu items for emu's except x64dtv
 */
static ui_menu_item_t debug_menu[] = {
    { "Trace mode ...", UI_MENU_TYPE_ITEM_ACTION,
        "tracemode", ui_debug_trace_mode_dialog_show, NULL,
        0, 0, true },

    UI_MENU_SEPARATOR,

    { "Main CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-maincpu", (void *)(ui_toggle_resource), (void *)"MainCPU_TRACE",
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "IEC bus trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-iec", (void *)(ui_toggle_resource), (void *)"IEC_TRACE",
        0, 0, false },

    { "IEEE-488 bus trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-ieee", (void *)(ui_toggle_resource), (void *)"IEEE_TRACE",
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "Drive #8 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive8", (void *)(ui_toggle_resource), (void *)"Drive0CPU_TRACE",
        0, 0, false },
    { "Drive #9 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive9", (void *)(ui_toggle_resource), (void *)"Drive1CPU_TRACE",
        0, 0, false },
    { "Drive #10 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive10", (void *)(ui_toggle_resource), (void *)"Drive2CPU_TRACE",
        0, 0, false },
    { "Drive #11 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive11", (void *)(ui_toggle_resource), (void *)"Drive3CPU_TRACE",
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "Autoplay playback frames ...", UI_MENU_TYPE_ITEM_ACTION,
        "playframes", ui_debug_playback_frames_dialog_show, NULL,
        0, 0, true },
    { "Save core dump", UI_MENU_TYPE_ITEM_CHECK,
        "coredump", (void *)(ui_toggle_resource), (void *)"DoCoreDump",
        0, 0, false },

    UI_MENU_TERMINATOR
};


/** \brief  'Debug' menu items for x64dtv
 */
static ui_menu_item_t debug_menu_c64dtv[] = {
    { "Trace mode ...", UI_MENU_TYPE_ITEM_ACTION,
        "tracemode", ui_debug_trace_mode_dialog_show, NULL,
        0, 0, true },

    UI_MENU_SEPARATOR,

    { "Main CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-maincpu", (void *)(ui_toggle_resource), (void *)"MainCPU_TRACE",
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "IEC bus trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-iec", (void *)(ui_toggle_resource), (void *)"IEC_TRACE",
        0, 0, false },

    { "Drive #8 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive8", (void *)(ui_toggle_resource), (void *)"Drive0CPU_TRACE",
        0, 0, false },
    { "Drive #9 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive9", (void *)(ui_toggle_resource), (void *)"Drive1CPU_TRACE",
        0, 0, false },
    { "Drive #10 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive10", (void *)(ui_toggle_resource), (void *)"Drive2CPU_TRACE",
        0, 0, false },
    { "Drive #11 CPU trace", UI_MENU_TYPE_ITEM_CHECK,
        "trace-drive11", (void *)(ui_toggle_resource), (void *)"Drive3CPU_TRACE",
        0, 0, false },

    UI_MENU_SEPARATOR,

    { "Blitter log", UI_MENU_TYPE_ITEM_CHECK,
      "blitter-log", ui_toggle_resource, (void *)"DtvBlitterLog",
      0, 0, false },
    { "DMA log", UI_MENU_TYPE_ITEM_CHECK,
      "dma-log", ui_toggle_resource, (void *)"DtvDMALog",
      0, 0, false },
    { "Flash log", UI_MENU_TYPE_ITEM_CHECK,
      "flash-log", ui_toggle_resource, (void*)"DtvFlashLog",
      0, 0, false },

    UI_MENU_SEPARATOR,

    { "Autoplay playback frames ...", UI_MENU_TYPE_ITEM_ACTION,
        "playframes", ui_debug_playback_frames_dialog_show, NULL,
        0, 0, true },
    { "Save core dump", UI_MENU_TYPE_ITEM_CHECK,
        "coredump", ui_toggle_resource, (void *)"DoCoreDump",
        0, 0, false },

    UI_MENU_TERMINATOR
};

#endif


/** \brief  'Help' menu items
 */
static ui_menu_item_t help_menu[] = {
    { "Browse manual", UI_MENU_TYPE_ITEM_ACTION,
        "manual", ui_open_manual_callback, NULL,
        0, 0, true },
    { "Command line options ...", UI_MENU_TYPE_ITEM_ACTION,
        "cmdline", uicmdline_dialog_show, NULL,
        0, 0, true },
    { "Compile time features ...", UI_MENU_TYPE_ITEM_ACTION,
        "features", uicompiletimefeatures_dialog_show, NULL,
        0, 0, true },
    { "Hotkeys", UI_MENU_TYPE_ITEM_ACTION,
        "hotkeys", uihotkeys_dialog_show, NULL,
        0, 0, true },
    { "About VICE", UI_MENU_TYPE_ITEM_ACTION,
        "about", ui_about_dialog_callback, NULL,
        0, 0, true },

    UI_MENU_TERMINATOR
};


/** \brief  Create the top menu bar with standard submenus
 *
 * \return  GtkMenuBar
 */
GtkWidget *ui_machine_menu_bar_create(void)
{
    GtkWidget *menu_bar;

    /* create the top menu bar */
    menu_bar = gtk_menu_bar_new();

    /* create the top-level 'File' menu */
    file_submenu = ui_menu_submenu_create(menu_bar, "File");

    /* create the top-level 'Edit' menu */
    edit_submenu = ui_menu_submenu_create(menu_bar, "Edit");

    /* create the top-level 'Snapshot' menu */
    snapshot_submenu = ui_menu_submenu_create(menu_bar, "Snapshot");

    /* create the top-level 'Settings' menu */
    settings_submenu = ui_menu_submenu_create(menu_bar, "Settings");

#ifdef DEBUG
    /* create the top-level 'Debug' menu (when --enable-debug is used) */
    debug_submenu = ui_menu_submenu_create(menu_bar, "Debug");
#endif

    /* create the top-level 'Help' menu */
    help_submenu = ui_menu_submenu_create(menu_bar, "Help");

    /* determine which joystick swap menu items should be added */
    switch (machine_class) {
        case VICE_MACHINE_C64:      /* fall through */
        case VICE_MACHINE_C64SC:    /* fall through */
        case VICE_MACHINE_C128:     /* fall through */
        case VICE_MACHINE_PLUS4:
            /* add tape section */
            file_menu_tape_section = file_menu_tape;
            /* fall through */
        case VICE_MACHINE_SCPU64:
            /* add both swap-joy and swap-userport-joy */
            settings_menu_joy_section = settings_menu_all_joy;
            break;
        case VICE_MACHINE_CBM5x0:
            /* add tape section */
            file_menu_tape_section = file_menu_tape;
            /* fall through */
        case VICE_MACHINE_C64DTV:
            /* only add swap-joy */
            settings_menu_joy_section = settings_menu_cbm5x0_joy;
            break;
        case VICE_MACHINE_PET:      /* fall through */
        case VICE_MACHINE_VIC20:    /* fall through */
        case VICE_MACHINE_CBM6x0:
            /* add tape section */
            file_menu_tape_section = file_menu_tape;
            /* only add swap-userport-joy */
            settings_menu_joy_section = settings_menu_userport_joy;
            break;
        case VICE_MACHINE_VSID:
            archdep_vice_exit(1);
            break;
        default:
            break;
    }

    /* add items to the File menu */
    ui_menu_add(file_submenu, file_menu_head);
    if (file_menu_tape_section != NULL) {
        ui_menu_add(file_submenu, file_menu_tape_section);
    }
    ui_menu_add(file_submenu, file_menu_tail);

    /* add items to the Edit menu */
    ui_menu_add(edit_submenu, edit_menu);
    /* add items to the Snapshot menu */
    ui_menu_add(snapshot_submenu, snapshot_menu);

    /* add items to the Settings menu */
    ui_menu_add(settings_submenu, settings_menu_head);
    if (settings_menu_joy_section != NULL) {
        ui_menu_add(settings_submenu, settings_menu_joy_section);
    }
    ui_menu_add(settings_submenu, settings_menu_tail);

#ifdef DEBUG
    /* add items to the Debug menu */
    if (machine_class == VICE_MACHINE_C64DTV) {
        ui_menu_add(debug_submenu, debug_menu_c64dtv);
    } else {
        ui_menu_add(debug_submenu, debug_menu);
    }
#endif

    /* add items to the Help menu */
    ui_menu_add(help_submenu, help_menu);

    main_menu_bar = menu_bar;    /* XXX: do I need g_object_ref()/g_object_unref()
                                         for this */
    return menu_bar;
}


/** \brief  Add missing settings load/save items
 *
 * \param[in,out]   menu    GtkMenu
 */
void ui_machine_menu_bar_vsid_patch(GtkWidget *menu)
{
    ui_menu_add(menu, settings_menu_tail);
}

