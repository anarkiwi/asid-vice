/** \file   settings_hotkeys.c
 * \brief   Hotkeys settings
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
 *
 */

/* Resources manipulated in this file:
 *
 * $VICERES HotkeyFile all
 */

#include "vice.h"
#include <gtk/gtk.h>
#include <time.h>
#include <string.h>
#include "vice_gtk3.h"

#include "archdep.h"
#include "hotkeymap.h"
#include "hotkeys.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "ui.h"
#include "uiactions.h"
#include "uiapi.h"
#include "uihotkeysload.h"
#include "uimenu.h"
#include "uitypes.h"
#include "util.h"

#include "settings_hotkeys.h"


/*
 * Enable debugging messages and widgets
 */
#ifndef DEBUG_HOTKEYS
/* #define DEBUG_HOTKEYS */
#endif

#ifndef ARRAY_LEN
#define ARRAY_LEN(arr)  (sizeof arr / sizeof arr[0])
#endif


typedef struct mod_mask_s {
    guint mask;         /**< GDKModifierType constant */
    const char *name;   /**< stringified macro name */
    int column;         /**< GtkGrid column */
    int row;            /**< GtkGrid row */
} mod_mask_t;


/** \brief  Columns for the hotkeys table
 */
enum {
    COL_ACTION_ID,      /**< action ID (integer) */
    COL_ACTION_NAME,    /**< action name (string) */
    COL_ACTION_DESC,    /**< action description (string) */
    COL_HOTKEY          /**< key and modifiers (string) */
};


/** \brief  Set/Unset hotkey dialog custom response IDs
 */
enum {
    RESPONSE_CLEAR  /**< clear hotkey for selected item */
};


#ifdef DEBUG_HOTKEYS
/*
 * Forward declarations
 */
static void update_modifier_list(void);
#endif


#ifdef DEBUG_HOTKEYS
#define mod_item(M) M, #M
static const mod_mask_t mod_mask_list[] = {
    { mod_item(GDK_SHIFT_MASK),     0, 0 },
    { mod_item(GDK_LOCK_MASK),      0, 1 },
    { mod_item(GDK_CONTROL_MASK),   0, 2 },
    { mod_item(GDK_SUPER_MASK),     0, 3 },
    { mod_item(GDK_HYPER_MASK),     0, 4 },
    { mod_item(GDK_META_MASK),      0, 5 },
    { mod_item(GDK_MOD1_MASK),      1, 0 },
    { mod_item(GDK_MOD2_MASK),      1, 1 },
    { mod_item(GDK_MOD3_MASK),      1, 2 },
    { mod_item(GDK_MOD4_MASK),      1, 3 },
    { mod_item(GDK_MOD5_MASK),      1, 4 }
};
#undef mod_item
#endif


/** \brief  Reference to the hotkeys table widget
 *
 * Used for easier event handling.
 */
static GtkWidget *hotkeys_view;

static GtkWidget *hotkeys_path;

#ifdef DEBUG_HOTKEYS
static GtkWidget *modifiers_grid;
static GtkWidget *modifiers_string;
#endif

/** \brief  Label holding the hotkey string
 */
static GtkWidget *hotkey_string;

#ifdef DEBUG_HOTKEYS
static GtkWidget *keysym_string;
#endif

static guint hotkey_keysym;
static guint hotkey_mask;   /* modifiers mask */

static guint hotkey_keysym_old;
static guint hotkey_mask_old;   /* modifiers mask */

/** \brief  Bitmask with accepted modifiers
 */
static guint accepted_mods = VHK_ACCEPTED_MODIFIERS;


#ifdef DEBUG_HOTKEYS
/** \brief  Label used to display the accepted mods mask as hex */
static GtkWidget *accepted_mods_string;

static GtkWidget *accepted_mods_grid;
#endif


/** \brief  Clear all hotkeys and update the view
 */
static void clear_all_hotkeys(void)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    ui_clear_hotkeys();

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hotkeys_view));
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_HOTKEY, NULL, -1);
        } while (gtk_tree_model_iter_next(model, &iter));
    }
}


/** \brief  Update hotkeys path widget */
static void update_hotkeys_path(void)
{
    const char *hotkeyfile = NULL;

    resources_get_string("HotkeyFile", &hotkeyfile);
    if (hotkeyfile != NULL && *hotkeyfile != '\0') {
        gtk_entry_set_text(GTK_ENTRY(hotkeys_path), hotkeyfile);
    } else {
        /* default location */
        char *path;
        char *datadir = archdep_get_vice_datadir();

        if (machine_class == VICE_MACHINE_VSID) {
            path = util_join_paths(datadir, machine_name, VHK_DEFAULT_NAME_VSID, NULL);
        } else {
            path = util_join_paths(datadir, machine_name, VHK_DEFAULT_NAME, NULL);
        }
        lib_free(datadir);

        gtk_entry_set_text(GTK_ENTRY(hotkeys_path), path);
        lib_free(path);
    }
}



/* Will be replaced with UI actions */
#if 0
/** \brief  Callback for the 'Export' button
 *
 * Save current hotkeys to \a filename and close \a dialog.
 *
 * \param[in]   dialog      Save dialog
 * \param[in]   filename    filename (`NULL` == cancel)
 * \param[in]   data        extra data (unused)
 */
static void export_callback(GtkDialog *dialog, gchar *filename, gpointer data)
{
    if (filename != NULL) {
        if (ui_hotkeys_export(filename)) {
            debug_gtk3("OK, '%s' was written succesfully.", filename);
        } else {
            ui_error("Failed to write '%s' to filesystem.", filename);
        }
    } else {
        debug_gtk3("Canceled.");
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Handler for the 'clicked' event of the export button
 *
 * \param[in]   button  widget triggering the event
 * \param[in]   data    extra event data (unused)
 */
static void on_export_clicked(GtkWidget *button, gpointer data)
{
    GtkWidget *dialog;
    const char *path = NULL;
    char *fname = NULL;
    char *dname = NULL;

    /* split path into basename and directory component */
    if ((resources_get_string("HotkeyFile", &path)) == 0 && path != NULL) {
        util_fname_split(path, &dname, &fname);
    }

    dialog = vice_gtk3_save_file_dialog("Save current hotkeys to file",
                                        fname,
                                        TRUE,
                                        dname,
                                        export_callback,
                                        NULL);
    gtk_widget_show(dialog);

    /* clean up */
    if (fname != NULL) {
        lib_free(fname);
    }
    if (dname != NULL) {
        lib_free(dname);
    }
}


/** \brief  Create widget to select the user-defined hotkeys file
 *
 * \return  GtkGrid
 */
static GtkWidget *create_browse_widget(void)
{
    GtkWidget *widget;
    const char * const patterns[] = { "*.vhk", NULL };

    widget = vice_gtk3_resource_browser_new("HotkeyFile",
                                            patterns,
                                            "VICE hotkeys",
                                            "Select VICE hotkeys file",
                                            "Custom hotkeys file:",
                                            NULL);
    return widget;
}
#endif




/** \brief  Create model for the hotkeys table
 *
 * \return  new list store
 */
static GtkListStore *create_hotkeys_model(void)
{
    GtkListStore *model;
    ui_action_info_t *list;
    const ui_action_info_t *action;

    model = gtk_list_store_new(4,
                               G_TYPE_INT,      /* action ID */
                               G_TYPE_STRING,   /* action name */
                               G_TYPE_STRING,   /* action description */
                               G_TYPE_STRING    /* hotkey as string */
                               );

    list = ui_action_get_info_list();
    for (action = list; action->id > ACTION_NONE; action++) {
        GtkTreeIter iter;
        hotkey_map_t *map;
        gchar *hotkey = NULL;

        /* Is there a hotkey defined for the current action? */
        map = hotkey_map_get_by_action(action->id);
        if (map != NULL) {
            hotkey = hotkey_map_get_accel_label(map);
        }

        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model,
                           &iter,
                           COL_ACTION_ID, action->id,
                           COL_ACTION_NAME, action->name,
                           COL_ACTION_DESC, action->desc,
                           COL_HOTKEY, hotkey,
                           -1);
        if (hotkey != NULL) {
            g_free(hotkey);
        }
    }
    lib_free(list);

    return model;
}


/** \brief  Handler for the 'key-release-event'
 *
 * \param[in]   dialog  window triggering the event
 * \param[in]   event   event info
 * \param[in]   data    extra event data (unused)
 *
 * \return  TRUE to propagate events further
 */
static gboolean on_key_release_event(GtkWidget *dialog,
                                     GdkEventKey *event,
                                     gpointer data)
{
    GdkKeymap *keymap;
    GdkKeymapKey *keys;
    guint *keyvals;
    gint keymap_entry_count;
    gchar *accel_label;
    gchar *escaped;
    gchar text[256];
#ifdef DEBUG_HOTKEYS
    gchar *accel_name;
    time_t t;
    struct tm *tm;
    int n;
#endif
    int i;
    bool base_key_found = false;

    if (event->is_modifier) {
        return TRUE;
    }

    /* Check for plain Return/Enter or Escape: these we want to pass to the
     * dialog's event handler to trigger the response handler. */
    if (((event->state & accepted_mods) == 0)) {
        if (event->keyval == GDK_KEY_Return ||
                event->keyval == GDK_KEY_KP_Enter ||
                event->keyval == GDK_KEY_Escape) {
            log_message(LOG_DEFAULT,
                        "Hotkeys: plain Return/KP-Enter/Escape pressed,"
                        " let event pass to the dialog keyboard handler.");
            return TRUE;
        }
    }

    keymap = gdk_keymap_get_for_display(gtk_widget_get_display(dialog));
    /* Default to what the event provided */
    hotkey_keysym = event->keyval;
    hotkey_mask = event->state & accepted_mods;

#if 0
    debug_gtk3("keysym = %04x, mask = %04x.", hotkey_keysym, hotkey_mask);
#endif

#ifdef DEBUG_HOTKEYS
    t = time(NULL);
    tm = localtime(&t);
    strftime(text, sizeof(text), "%H:%M:%S", tm);

    log_message(LOG_DEFAULT, "Hotkeys: --- accelerator entered [%s] ---", text);

    accel_name = gtk_accelerator_name(hotkey_keysym, hotkey_mask);
    accel_label = gtk_accelerator_get_label(hotkey_keysym, hotkey_mask);
    log_message(LOG_DEFAULT, "Hotkeys: initial gtk accel name : %s", accel_name);
    log_message(LOG_DEFAULT, "Hotkeys: initial gtk accel label: %s", accel_label);
    g_free(accel_name);
    g_free(accel_label);
#endif

    /*
     * Keyboards can generate lots of weird keys, for example alt-w produces Sigma on macOS.
     * We can ask GDK for a list of them given a hardware keycode and a keymap.
     *
     * We use the entry with a group and level of 0 for the raw key with no modifiers,
     * which we need to successfully match with a hotkey combo.
     *
     * We've obvserved that on windows, sometimes the returned array is many hundreds
     * of entries long, and it's clear that beyond the valid entries we see uninitialised
     * memory. This is probably a GDK bug, unless we're writing to a pointer somewhere
     * that we shouldn't.
     */

    /*
     * Workaround for GTK 3.24.31 bug where this output value isn't initialised to zero
     * before being incremented for each key found. GTK have fixed this in newer versions.
     */
    keymap_entry_count = 0;

    if (gdk_keymap_get_entries_for_keycode(keymap, event->hardware_keycode, &keys, &keyvals, &keymap_entry_count)) {
        if (keys && keyvals) {
#ifdef DEBUG_HOTKEYS
            log_message(LOG_DEFAULT, "Hotkeys: keymap entries (%d total):", keymap_entry_count);
#endif
            for (i = 0; i < keymap_entry_count; i++) {
#ifdef DEBUG_HOTKEYS
                log_message(LOG_DEFAULT,
                            "Hotkeys:   index: %d keyval: %04x group: %d level: %d keyname: %s",
                            i, keyvals[i], keys[i].group, keys[i].level, gdk_keyval_name(keyvals[i]));
#endif
                if (keys[i].group == 0 && keys[i].level == 0) {
                    if (base_key_found) {
#ifdef DEBUG_HOTKEYS
                        log_message(LOG_DEFAULT, "Hotkeys: Ignoring additional (group 0, level 0) base key that shouldn't be there");
#endif
                        continue;
                    }

                    base_key_found = true;

                    if (keymap_entry_count && keyvals[i] != hotkey_keysym) {
                        /* Override the detected keyval */
#ifdef DEBUG_HOTKEYS
                        log_message(LOG_DEFAULT, "Hotkeys: Overriding key from %s to %s", gdk_keyval_name(hotkey_keysym), gdk_keyval_name(keyvals[0]));
#endif
                        hotkey_keysym = keyvals[i];
                    }
                }
            }
        }

        if (keys) {
            g_free(keys);
        }
        if (keyvals) {
            g_free(keyvals);
        }
    }

#ifdef DEBUG_HOTKEYS
    accel_name = gtk_accelerator_name(hotkey_keysym, hotkey_mask);
#endif
    accel_label = gtk_accelerator_get_label(hotkey_keysym, hotkey_mask);
    escaped = g_markup_escape_text(accel_label, -1);
    g_snprintf(text, sizeof(text), "<b>%s</b>", escaped);
    gtk_label_set_markup(GTK_LABEL(hotkey_string), text);
    g_free(escaped);

#ifdef DEBUG_HOTKEYS
    log_message(LOG_DEFAULT, "Hotkeys: final gtk accel name : %s", accel_name);
    log_message(LOG_DEFAULT, "Hotkeys: final gtk accel label: %s", accel_label);
    log_message(LOG_DEFAULT, "Hotkeys: keysym        : 0x%04x (GDK_KEY_%s)",
            hotkey_keysym, gdk_keyval_name(hotkey_keysym));
    log_message(LOG_DEFAULT, "Hotkeys: modifier mask : 0x%08x", hotkey_mask);
    for (i = 0, n = 1; i < (int)ARRAY_LEN(mod_mask_list); i++) {
        if (hotkey_mask & mod_mask_list[i].mask) {
            log_message(LOG_DEFAULT, "Hotkeys: modifier %-2d   : 0x%08x (%s)",
                    n, mod_mask_list[i].mask, mod_mask_list[i].name);
            n++;
        }
    }
    update_modifier_list();
#endif

#ifdef DEBUG_HOTKEYS
    g_free(accel_name);
#endif
    g_free(accel_label);

    return TRUE;
}


/** \brief  Update the hotkey string of the currently selected row
 *
 * \param[in]   accel   hotkey string
 *
 * \return  TRUE on success
 */
static gboolean update_treeview_hotkey(const gchar *accel)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;

    /* update entry in tree model */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(hotkeys_view));
    if (gtk_tree_selection_get_selected(selection,
                                        &model,
                                        &iter)) {
        gtk_list_store_set(GTK_LIST_STORE(model),
                          &iter,
                          COL_HOTKEY, accel,
                          -1);
        return TRUE;
    }
    return FALSE;
}

/** \brief  Update treeview by regenerating model with current hotkeys
 */
static void update_treeview_full(void)
{
    GtkListStore *model = create_hotkeys_model();

    gtk_tree_view_set_model(GTK_TREE_VIEW(hotkeys_view), GTK_TREE_MODEL(model));
}



/** \brief  Remove the accelerator label \a accel from the model
 *
 * \param[in]   accel   accelerator label
 *
 * \return  TRUE on success
 */
static gboolean remove_treeview_hotkey(const gchar *accel)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hotkeys_view));
    if (model == NULL) {
        return FALSE;   /* shouldn't happen */
    }
    if (!gtk_tree_model_get_iter_first(model, &iter)) {
        return FALSE;   /* shouldn't happen either */
    }

    do {
        gchar *s = NULL;

        gtk_tree_model_get(model, &iter, COL_HOTKEY, &s, -1);
        if (s != NULL && strcmp(s, accel) == 0) {
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, COL_HOTKEY, NULL, -1);
            g_free(s);
            return TRUE;
        }
        if (s != NULL) {
            g_free(s);
        }
    } while (gtk_tree_model_iter_next(model, &iter));

    return FALSE;
}


/** \brief  Set/update hotkey in accelerator group, connect to item(s), if any
 *
 * \param[in]   action  UI action ID
 */
static void dialog_accept_handler(int action)
{
    gchar *accel;

    accel = gtk_accelerator_get_label(hotkey_keysym, hotkey_mask);
    debug_gtk3("Setting accelerator: %s (keysym: %04x, mask: %04x)"
               " for action %d (%s).",
               accel, hotkey_keysym, hotkey_mask, action, ui_action_get_name(action));

    /* Look up item for new hotkey and remove the hotkey from that
     * action/menu item.
     *
     * XXX: somehow the mask gets OR'ed with $2000, which is a reserved
     *      flag, so we mask out any reserved bits:
     */
    hotkey_map_t *map = hotkey_map_get_by_hotkey(hotkey_keysym,
                                                 hotkey_mask & accepted_mods);
    if (map == NULL) {
        debug_gtk3("No previously registered action for hotkey %s. OK.", accel);
    } else if (map->keysym != 0) {
        debug_gtk3("Removing old accelerator: %s from action %d (%s)",
                   accel, map->action, ui_action_get_name(map->action));
        hotkey_map_clear_hotkey(map);
        remove_treeview_hotkey(accel);
    }

    map = hotkey_map_get_by_action(action);
    if (map == NULL) {
        debug_gtk3("Error: Couldn't find hotkey map for action %d!", action);
        g_free(accel);
        return;
    }
    debug_gtk3("Setting new accelerator %s for action %d (%s)",
               accel, action, ui_action_get_name(action));
    if (!hotkey_map_update_hotkey(map, hotkey_keysym, hotkey_mask & accepted_mods)) {
        debug_gtk3("Error: Failed to update hotkey!");
    } else {
        update_treeview_hotkey(accel);
    }
    g_free(accel);
}

/** \brief  Clear hotkey from accelerator group and item(s), if any
 *
 * \param[in]   action  UI action ID
 */
static void dialog_clear_handler(int action)
{
    if (hotkey_map_clear_hotkey_by_action(action)) {
        update_treeview_hotkey(NULL);
    }
}

/** \brief  Handler for the 'response' event of the dialog
 *
 * \param[in]   dialog      hotkey dialog
 * \param[in]   response_id response ID
 * \param[in]   data        action ID
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    int action = GPOINTER_TO_INT(data);

    switch (response_id) {

        case GTK_RESPONSE_ACCEPT:
            debug_gtk3("Response ID %d: Accept '%s'",
                       response_id,
                       ui_action_get_name(action));

            if (hotkey_keysym == 0) {
                debug_gtk3("User clicked accepted/pressed enter without having set a hotkey.");
                break;
            }
            dialog_accept_handler(action);
            break;

        case RESPONSE_CLEAR:
            debug_gtk3("Response ID %d: Clear '%s'",
                       response_id, ui_action_get_name(action));
            dialog_clear_handler(action);
            break;

        case GTK_RESPONSE_CANCEL:       /* fall through */
        case GTK_RESPONSE_CLOSE:        /* fall through */
        case GTK_RESPONSE_DELETE_EVENT:
            debug_gtk3("Response ID %d: cancel/close/delete event.", response_id);
            break;

        default:
            debug_gtk3("Unhandled response ID: %d.", response_id);
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
}


#ifdef DEBUG_HOTKEYS
/** \brief  Update modifier check buttons
 */
static void update_modifier_list(void)
{
    gchar markup[256];
    int i = 0;

    while (TRUE) {
        GtkWidget *widget;
        guint mod;

        widget = gtk_grid_get_child_at(GTK_GRID(modifiers_grid), 0, i);
        if (widget == NULL) {
            break;
        }

        mod = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "ModifierMask"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), hotkey_mask & mod);

        i++;
    }

    g_snprintf(markup, sizeof(markup), "<tt>0x%08x</tt>", hotkey_mask);
    gtk_label_set_markup(GTK_LABEL(modifiers_string), markup);

    g_snprintf(markup, sizeof(markup), "<tt>0x%04x, GDK_KEY_%s</tt>",
               hotkey_keysym, gdk_keyval_name(hotkey_keysym));
    gtk_label_set_markup(GTK_LABEL(keysym_string), markup);

}
#endif


#ifdef DEBUG_HOTKEYS
/** \brief  Create list of check buttons for modifiers
 *
 * \return  GtkGrid
 */
static GtkWidget *create_modifier_list(void)
{
    const hotkeys_modifier_t *list;
    GtkWidget *grid;
    int row;
    int i;

    list = ui_hotkeys_get_modifier_list();
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    row = 0;
    for (i = 0; list[i].name != NULL; i++) {
        GtkWidget *check;
        GtkWidget *label;
        gpointer data;
        gchar buffer[256];


        if (list[i].id == HOTKEYS_MOD_ID_ALT) {
            /* Alt and Option are the same, merge: */
            check = gtk_check_button_new_with_label("Alt / Option \u2325");
        } else if (list[i].id == HOTKEYS_MOD_ID_OPTION) {
            /* skip, already taken care of */
            continue;
        } else {
            /* not Alt nor Option */
            check = gtk_check_button_new_with_label(list[i].utf8);
        }
        data = GINT_TO_POINTER(list[i].id);
        g_object_set_data(G_OBJECT(check), "ModifierID", data);
        data = GUINT_TO_POINTER(list[i].mask);
        g_object_set_data(G_OBJECT(check), "ModifierMask", data);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     hotkey_mask & list[i].mask);
        /* gtk_widget_set_sensitive(check, FALSE); */
        gtk_grid_attach(GTK_GRID(grid), check, 0, row, 1, 1);

        g_snprintf(buffer, sizeof(buffer), "<tt>GDK_%s_MASK</tt>", list[i].mask_str);
        label = gtk_label_new(NULL);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_markup(GTK_LABEL(label), buffer);
        gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);

        g_snprintf(buffer, sizeof(buffer), "<tt>0x%08x</tt>", list[i].mask);
        label = gtk_label_new(NULL);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_label_set_markup(GTK_LABEL(label), buffer);
        gtk_grid_attach(GTK_GRID(grid), label, 2, row, 1, 1);

        row ++;
    }
    return grid;
}
#endif


#ifdef DEBUG_HOTKEYS
/** \brief  Handler for the 'toggled' event of the modifier check buttons
 *
 * \param[in]   toggle  toggle button
 * \param[in]   data    extra event data (unused)
 */
static void on_accepted_mod_toggled(GtkToggleButton *toggle, gpointer data)
{
    int i;
    gchar text[256];
    time_t t;
    struct tm *tm;

    t = time(NULL);
    tm = localtime(&t);
    strftime(text, sizeof(text), "%H:%M:%S", tm);

    log_message(LOG_DEFAULT, "Hotkeys: --- accepted modifiers changed [%s] ---", text);
    log_message(LOG_DEFAULT, "Hotkeys: old mask: 0x%08x", accepted_mods);

    accepted_mods = 0;
    for (i = 0; i < (int)ARRAY_LEN(mod_mask_list); i++) {
        GtkWidget *check;

        check = gtk_grid_get_child_at(GTK_GRID(accepted_mods_grid),
                                      mod_mask_list[i].column,
                                      mod_mask_list[i].row);
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check))) {
            accepted_mods |= mod_mask_list[i].mask;
            log_message(LOG_DEFAULT, "Hotkeys: modifier: 0x%08x (%s)",
                    mod_mask_list[i].mask, mod_mask_list[i].name);
        }
    }

    log_message(LOG_DEFAULT, "Hotkeys: new mask: 0x%08x", accepted_mods);
    g_snprintf(text, sizeof(text), "<tt><b>0x%08x</b></tt>", accepted_mods);
    gtk_label_set_markup(GTK_LABEL(accepted_mods_string), text);

}
#endif


#ifdef DEBUG_HOTKEYS
/** \brief  Create modifier check buttons
 *
 * \return  GtkGrid
 */
static GtkWidget *create_accepted_mods_widget(void)
{
    GtkWidget *grid;
    GtkWidget *wrapper;
    GtkWidget *label;
    gchar text[256];
    int i;
    int maxrow = 0;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 32);

    /* add check boxes for the modifier masks */
    for (i = 0; i < (int)ARRAY_LEN(mod_mask_list); i++) {
        GtkWidget *check;
        guint mask = mod_mask_list[i].mask;

        g_snprintf(text, sizeof(text), "%s (0x%08x)", mod_mask_list[i].name, mask);
        check = gtk_check_button_new_with_label(text);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                     accepted_mods & mask);
        gtk_grid_attach(GTK_GRID(grid), check,
                        mod_mask_list[i].column, mod_mask_list[i].row, 1, 1);
        g_signal_connect(check, "toggled", G_CALLBACK(on_accepted_mod_toggled), NULL);

        if (mod_mask_list[i].row > maxrow) {
            maxrow = mod_mask_list[i].row;
        }
    }

    wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(wrapper), 16);

    label = gtk_label_new("Current modifier mask filter:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(wrapper), label, 0, 0, 1, 1);

    g_snprintf(text, sizeof(text), "<tt><b>0x%08x</b></tt>", accepted_mods);
    accepted_mods_string = gtk_label_new(NULL);
    gtk_widget_set_halign(accepted_mods_string, GTK_ALIGN_START);
    gtk_label_set_markup(GTK_LABEL(accepted_mods_string), text);
    gtk_grid_attach(GTK_GRID(wrapper), accepted_mods_string, 1, 0, 1, 1);
    gtk_widget_set_margin_top(wrapper, 8);
    gtk_widget_show_all(wrapper);

    gtk_grid_attach(GTK_GRID(grid), wrapper, 0, maxrow + 1, 2, 1);
    gtk_widget_show_all(grid);
    return grid;
}
#endif


/** \brief  Create content widget for the hotkey dialog
 *
 * \param[in]   action  hotkey action name
 * \param[in]   hotkey  hotkey string
 *
 * \return  GtkGrid
 */
static GtkWidget *create_content_widget(const gchar *action, const gchar *hotkey)
{
    GtkWidget *grid;
    GtkWidget *label;
    gchar text[1024];
    gchar *escaped;
#ifdef DEBUG_HOTKEYS
    gchar *keyname;
#endif
    int row = 0;

    grid = vice_gtk3_grid_new_spaced(16, 0);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);

    label = gtk_label_new(NULL);
    g_snprintf(text,
               sizeof(text),
               "Press a key or key combination to set the hotkey"
               " for '<b>%s</b>'.\n\n"
               "Click Accept to use the new hotkey and remove the current one, if any.\n"
               "Click Clear to remove the current hotkey."
#ifdef DEBUG_HOTKEYS
               "Please note the <i>reported modifiers</i> check boxes for are"
               " for debugging and are set when\npushing a new hotkey."
               " Toggling them by hand does nothing at the moment.",
#else
               ,
#endif
               action);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_widget_set_margin_bottom(label, 32);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;
#ifdef DEBUG_HOTKEYS
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Reported modifiers:</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 32);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;

    modifiers_grid = create_modifier_list();
    gtk_widget_set_margin_top(modifiers_grid, 8);
    gtk_widget_set_margin_bottom(modifiers_grid, 16);
    gtk_grid_attach(GTK_GRID(grid), modifiers_grid, 0, row, 2, 1);
    row++;

    label = gtk_label_new("GDK modifier mask:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    modifiers_string = gtk_label_new(NULL);
    gtk_widget_set_halign(modifiers_string, GTK_ALIGN_START);
    g_snprintf(text, sizeof(text), "<tt>0x%08x</tt>", hotkey_mask);
    gtk_label_set_markup(GTK_LABEL(modifiers_string), text);
    gtk_grid_attach(GTK_GRID(grid), modifiers_string, 1, row, 1, 1);
    row++;

    label = gtk_label_new("GDK keysym:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    keysym_string = gtk_label_new(NULL);
    keyname = gdk_keyval_name(hotkey_keysym);
    if (keyname == NULL) {
        /* no valid key -> no hotkey defined */
        strncpy(text, "<i>Undefined</i>", sizeof(text) - 1UL);
        text[sizeof(text) - 1UL] = '\0';
    } else {
        g_snprintf(text, sizeof(text), "GDK_KEY_%s", keyname);
    }
    gtk_widget_set_halign(keysym_string, GTK_ALIGN_START);
    gtk_label_set_markup(GTK_LABEL(keysym_string), text);
    gtk_widget_set_halign(keysym_string, GTK_ALIGN_START);
    gtk_widget_set_hexpand(keysym_string, TRUE);
    gtk_grid_attach(GTK_GRID(grid), keysym_string, 1, row, 1, 1);
    row++;
#endif

    /* Currently defined hotkey */
    label = gtk_label_new("Current hotkey:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    label = gtk_label_new(NULL);
    if (hotkey != NULL && *hotkey != '\0') {
        escaped = g_markup_escape_text(hotkey, -1);
        g_snprintf(text, sizeof(text), "<b>%s</b>", escaped);
        g_free(escaped);
        gtk_label_set_markup(GTK_LABEL(label), text);
    } else {
        gtk_label_set_markup(GTK_LABEL(label), "<i>Undefined</i>");
    }
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label, 1, row, 1, 1);
    row++;

    /* New hotkey definition */
    label = gtk_label_new("New hotkey:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    hotkey_string = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(hotkey_string), "<i>Undefined</i>");
    gtk_widget_set_halign(hotkey_string, GTK_ALIGN_START);
    gtk_widget_set_hexpand(hotkey_string, TRUE);
    gtk_grid_attach(GTK_GRID(grid), hotkey_string, 1, row, 1, 1);
    row++;

#ifdef DEBUG_HOTKEYS
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>Accepted GDK modifier masks:</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 24);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 2, 1);
    row++;

    accepted_mods_grid = create_accepted_mods_widget();
    gtk_grid_attach(GTK_GRID(grid), accepted_mods_grid, 0, row, 2, 1);
    row++;
#endif

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Show dialog to set/edit hotkey for an action
 *
 * \param[in]   path    tree path to selected item in tree view
 */
static void show_hotkey_dialog(GtkTreePath *path)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hotkeys_view));
    if (gtk_tree_model_get_iter(model, &iter, path)) {

        GtkWidget *dialog;
        GtkWidget *content_area;
        GtkWidget *content_widget;
        gint action_id = 0;
        gchar *action_name = NULL;
        gchar *hotkey = NULL;
        guint keysym = 0;
        GdkModifierType mask = 0;

        /* get current hotkey info */
        gtk_tree_model_get(model, &iter,
                           COL_ACTION_ID, &action_id,
                           COL_ACTION_NAME, &action_name,
                           COL_HOTKEY, &hotkey,
                           -1);

        /* This needs to happen *before* calling create_content_widget(),
         * since that function uses these values */
        if (hotkey != NULL) {
            gtk_accelerator_parse(hotkey, &keysym, &mask);
            hotkey_keysym = keysym;
            hotkey_keysym_old = keysym;
            hotkey_mask = mask;
            hotkey_mask_old = mask;

        } else {
            hotkey_keysym = 0;
            hotkey_keysym_old = 0;
            hotkey_mask = 0;
            hotkey_mask_old = 0;
        }

        debug_gtk3("Row clicked: '%s' -> %s (mask: %04x, keysym: %04x).",
                   action_name,
                   hotkey != NULL ? hotkey : NULL,
                   mask,
                   keysym);

        dialog = gtk_dialog_new_with_buttons("Set/Edit hotkey",
                                             ui_get_active_window(),
                                             GTK_DIALOG_MODAL,
                                             "Accept", GTK_RESPONSE_ACCEPT,
                                             "Clear", RESPONSE_CLEAR,
                                             "Cancel", GTK_RESPONSE_REJECT,
                                             NULL);
        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        content_widget = create_content_widget(action_name, hotkey);

        gtk_box_pack_start(GTK_BOX(content_area), content_widget, TRUE, TRUE, 16);

        g_signal_connect(dialog,
                         "response",
                         G_CALLBACK(on_response),
                         GINT_TO_POINTER(action_id));
        g_signal_connect(dialog,
                         "key-release-event",
                         G_CALLBACK(on_key_release_event),
                         NULL);
        gtk_widget_show_all(dialog);

        if (action_name != NULL) {
            g_free(action_name);
        }
        if (hotkey != NULL) {
            g_free(hotkey);
        }
    }
}



/** \brief  Handler for the 'row-activated' event of the treeview
 *
 * Handler triggered by double-clicked or pressing Return on a tree row.
 *
 * \param[in]   view    tree view (unused)
 * \param[in]   path    tree path to activated row
 * \param[in]   column  activated column (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_row_activated(GtkTreeView *view,
                             GtkTreePath *path,
                             GtkTreeViewColumn *column,
                             gpointer data)
{
    show_hotkey_dialog(path);
}


/******************************************************************************
 *  Treeview context menu event handlers                                      *
 *****************************************************************************/

/** \brief  Handler for the 'activate' event of the context menu 'set'/'edit' item
 *
 * Pops up dialog to set/edit/clear the selected hotkey.
 *
 * \param[in]   unused      menu item (unused)
 * \param[in]   treepath    GtkTreePath to the selected row in the table
 */
static void on_context_edit_activate(GtkWidget *item, gpointer treepath)
{
    show_hotkey_dialog((GtkTreePath*)treepath);
}


/** \brief  Handler for the 'activate' event of the context menu 'clear' item
 *
 * Clears selected hotkey from the menu(s) and updated the table.
 *
 * \param[in]   unused      menu item (unused)
 * \param[in]   treepath    GtkTreePath to the selected row in the table
 */
static void on_context_clear_activate(GtkWidget *unused, gpointer treepath)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path = (GtkTreePath*)treepath;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hotkeys_view));
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gint action_id;

        gtk_tree_model_get(model, &iter, COL_ACTION_ID, &action_id, -1);
        if (hotkey_map_clear_hotkey_by_action(action_id)) {
            update_treeview_hotkey(NULL);   /* clear hotkey of selected row */
        }
    }
}


/** \brief  Callback for the 'clear all' confirmation dialog
 *
 * \param[in]   dialog  confirmation dialog
 * \param[in]   result  result of the dialog
 */
static void clear_dialog_callback(GtkDialog *dialog, gboolean result)
{
    if (result) {
        clear_all_hotkeys();
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}


/** \brief  Show confirmation dialog for clearing all hotkeys
 */
static void show_clear_all_confirm_dialog(void)
{
    vice_gtk3_message_confirm(clear_dialog_callback,
                              "Clear all hotkeys",
                              "Are you sure you want to clear all hotkeys? "
                              "This action cannot be undone.");
}


/** \brief  Handler for the 'activate' event of the context menu 'clear all' item
 *
 * Clears all hotkeys.
 *
 * \param[in]   unused      menu item (unused)
 * \param[in]   treepath    GtkTreePath to the selected row in the table
 */
static void on_context_clear_all_activate(GtkWidget *item, gpointer unused)
{
    show_clear_all_confirm_dialog();
}


/** \brief  Create context menu for the tree view
 *
 * \param[in]   path    tree view path of selected row
 *
 * \return  GtkMenu
 */
static GtkWidget *create_context_menu(GtkTreePath *path)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(hotkeys_view));
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        GtkWidget *menu;
        GtkWidget *item;
        gint action_id;
        gchar *action_name = NULL;
        gchar *hotkey = NULL;
        gchar buffer[1024];

        gtk_tree_model_get(model, &iter,
                           COL_ACTION_ID, &action_id,
                           COL_ACTION_NAME, &action_name,
                           COL_HOTKEY, &hotkey,
                           -1);

        menu = gtk_menu_new();
        debug_gtk3("ID = %d, Name = %s, Hotkey = %s",
                action_id, action_name, hotkey);

        /* Determine if to use "Edit" or "Set" */
        if (hotkey == NULL || *hotkey == '\0') {
            g_snprintf(buffer, sizeof(buffer), "Set hotkey for '%s'", action_name);
        } else {
            g_snprintf(buffer, sizeof(buffer), "Update hotkey for '%s'", action_name);
        }
        item = gtk_menu_item_new_with_label(buffer);
        g_signal_connect(item,
                         "activate",
                         G_CALLBACK(on_context_edit_activate),
                         (gpointer)path);
        gtk_container_add(GTK_CONTAINER(menu), item);

        if (hotkey != NULL && *hotkey != '\0') {
            g_snprintf(buffer, sizeof(buffer), "Clear hotkey %s", hotkey);
            item = gtk_menu_item_new_with_label(buffer);
            g_signal_connect(item,
                             "activate",
                             G_CALLBACK(on_context_clear_activate),
                             (gpointer)path);
            gtk_container_add(GTK_CONTAINER(menu), item);
        }

        item = gtk_separator_menu_item_new();
        gtk_container_add(GTK_CONTAINER(menu), item);

        item = gtk_menu_item_new_with_label("Clear all hotkeys");
        g_signal_connect(item,
                         "activate",
                         G_CALLBACK(on_context_clear_all_activate),
                         (gpointer)path);
        gtk_container_add(GTK_CONTAINER(menu), item);

        if (action_name != NULL) {
            g_free(action_name);
        }
        if (hotkey != NULL) {
            g_free(hotkey);
        }

        gtk_widget_show_all(menu);
        return menu;
    }
    return NULL;
}


/** \brief  Handler for the 'button-press-event' event of the treeview
 *
 * \param[in]   view    tree view
 * \param[in]   event   button press event data
 * \param[in]   unused  extra event data (unused)
 *
 * \return  `TRUE` to stop event propagation (right-click), `FALSE` to continue
 *          event propagation (other clicks)
 */
static gboolean on_button_press_event(GtkWidget *view,
                                      GdkEvent *event,
                                      gpointer data)
{
    GdkEventButton *eb = (GdkEventButton*)event;

    if (eb->button == GDK_BUTTON_SECONDARY) {
        GtkTreePath *path = NULL;

        /* grab tree path at pointer */
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(view),
                                          eb->x,
                                          eb->y,
                                          &path,
                                          NULL, /* column */
                                          NULL, /* cell_x */
                                          NULL  /* cell_y */)) {
            GtkTreeSelection *selection;
            GtkWidget *menu;

            /* select current row so updating the table works via its current
             * selection */
            selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
            gtk_tree_selection_select_path(selection, path);

            /* pop up context menu at row */
            menu = create_context_menu(path);
            if (menu != NULL) {
                gtk_menu_popup_at_pointer(GTK_MENU(menu), event);
            }
            return TRUE;    /* don't propagate further */
        }
    }
    return FALSE;    /* propagate further so double-clicking a row still works */
}


/** \brief  Create the table view of the hotkeys
 *
 * Create a table with 'action', 'description' and 'hotkey' columns.
 * Columns are resizable and the table is sortable by clicking the column
 * headers.
 *
 * \return  GtkTreeView
 */
static GtkWidget *create_hotkeys_view(void)
{
    GtkWidget *view;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;

    model = create_hotkeys_model();
    view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));

    /* name */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Action",
                                                      renderer,
                                                      "text", COL_ACTION_NAME,
                                                      NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_ACTION_NAME);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    /* description */
    renderer = gtk_cell_renderer_text_new();
    /* g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL); */
    column = gtk_tree_view_column_new_with_attributes("Description",
                                                      renderer,
                                                      "text", COL_ACTION_DESC,
                                                      NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_ACTION_DESC);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    /* hotkey */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Hotkey",
                                                      renderer,
                                                      "text", COL_HOTKEY,
                                                      NULL);
    gtk_tree_view_column_set_sort_column_id(column, COL_HOTKEY);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

    g_signal_connect(view, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(view, "button-press-event", G_CALLBACK(on_button_press_event), NULL);

    return view;
}


/** \brief  Handler for the 'clicked' event handler of the 'Clear all hotkeys'
 *          button
 *
 * \param[in]   button  button (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_clear_clicked(GtkWidget *button, gpointer unused)
{
    show_clear_all_confirm_dialog();
}

/** \brief  Handler for the 'clicked' event handler of the 'Reset to default'
 *          button
 *
 * \param[in]   button  button (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_defaults_clicked(GtkButton *button, gpointer unused)
{
    ui_hotkeys_load_default();
    update_treeview_full();
    update_hotkeys_path();
}

/** \brief  Handler for the 'clicked' event handler of the 'Reload hotkeys'
 *          button
 *
 * \param[in]   button  button (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_load_clicked(GtkButton *button, gpointer unused)
{
    /* We can trigger an action here since we're on the UI thread: the action's
     * handler will not be pused onto a thread or timeout but simply execute
     * in this thread */
    ui_action_trigger(ACTION_HOTKEYS_LOAD);
    update_treeview_full();
    update_hotkeys_path();
}

/** \brief  Callback for the hotkeys load dialog
 *
 * \param[in]   result  user loaded a file
 */
static void load_from_callback(gboolean result)
{
    if (result) {
        update_treeview_full();
        update_hotkeys_path();
    }
}

/** \brief  Handler for the 'clicked' event handler of the 'Load hotkeys from...'
 *          button
 *
 * \param[in]   button  button (unused)
 * \param[in]   unused  extra event data (unused)
 */
static void on_load_from_clicked(GtkButton *button, gpointer unused)
{
    ui_hotkeys_load_dialog_show(load_from_callback);
}


/** \brief  Create main widget
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *settings_hotkeys_widget_create(GtkWidget *parent)
{
    GtkWidget *grid;
    GtkWidget *scroll;
#if 0
    GtkWidget *browse;
    GtkWidget *export;
#endif
    GtkWidget *button;
    GtkWidget *wrapper;
    GtkWidget *label;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    /* create view, pack into scrolled window and add to grid */
    hotkeys_view = create_hotkeys_view();
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), hotkeys_view);
    gtk_widget_show_all(scroll);
    gtk_grid_attach(GTK_GRID(grid), scroll, 0, 0, 2, 1);
#if 0
    browse = create_browse_widget();
    gtk_grid_attach(GTK_GRID(grid), browse, 0, 1, 2, 1);
    export = gtk_button_new_with_label("Save hotkeys...");
    g_signal_connect(export, "clicked", G_CALLBACK(on_export_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), export, 0, 2, 1, 1);
#endif

    /* add widget to show path to current hotkeys file */
    wrapper = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(wrapper), 16);

    label = gtk_label_new("Current hotkeys file:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, FALSE);
    gtk_grid_attach(GTK_GRID(wrapper), label, 0, 0, 1, 1);

    hotkeys_path = gtk_entry_new();
    gtk_widget_set_hexpand(hotkeys_path, TRUE);
    gtk_editable_set_editable(GTK_EDITABLE(hotkeys_path), FALSE);
    gtk_widget_set_can_focus(hotkeys_path, FALSE);
    gtk_grid_attach(GTK_GRID(wrapper), hotkeys_path, 1, 0, 1, 1);
    update_hotkeys_path();

    gtk_widget_set_margin_top(wrapper, 8);
    gtk_widget_set_margin_bottom(wrapper, 8);
    gtk_grid_attach(GTK_GRID(grid), wrapper, 0, 1, 2, 1);

    /* need to regenerate the view after this action is triggered */
    button = gtk_button_new_with_label("Reset to default");
    g_signal_connect(button, "clicked", G_CALLBACK(on_defaults_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 2, 1 ,1);

    /* this one pops up a confirmation dialog, so we don't use an action ID */
    button = gtk_button_new_with_label("Clear all hotkeys");
    g_signal_connect(button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 1, 2, 1 ,1);

    button = gtk_button_new_with_label("Reload hotkeys");
    g_signal_connect(button, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 3, 1, 1);

    button = gtk_button_new_with_label("Load hotkeys from...");
    g_signal_connect(button, "clicked", G_CALLBACK(on_load_from_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 1, 3, 1, 1);

    button = ui_action_button_new(ACTION_HOTKEYS_SAVE, "Save hotkeys");
    gtk_grid_attach(GTK_GRID(grid), button, 0, 4, 1, 1);

    button = ui_action_button_new(ACTION_HOTKEYS_SAVE_TO, "Save hotkeys to...");
    gtk_grid_attach(GTK_GRID(grid), button, 1, 4, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
