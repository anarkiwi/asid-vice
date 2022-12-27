/** \file   hotkeys.h
 * \brief   Gtk3 custom hotkeys handling - header
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

#ifndef VICE_HOTKEYS_H
#define VICE_HOTKEYS_H

#include "vice.h"

#include <gtk/gtk.h>
#include <stdbool.h>
#include "archdep_defs.h"


/** \brief  Name of Gtk3 main hotkeys files
 */
#ifdef MACOS_COMPILE
# define VHK_PREFIX         "gtk3-hotkeys-mac"
# define VHK_PREFIX_VSID    "gtk3-vsid-hotkeys-mac"
#else
# define VHK_PREFIX         "gtk3-hotkeys"
# define VHK_PREFIX_VSID    "gtk3-vsid-hotkeys"
#endif

/** \brief  Extension of Gtk3 hotkeys files
 *
 * Although the extension is the same as for the SDL UI, the format is slightly
 * different.
 */
#define VHK_EXT     ".vhk"

/** \brief  Filename of default Gtk3 hotkeys files
 */
#define VHK_DEFAULT_NAME        VHK_PREFIX VHK_EXT

/** \brief  Filename of default Gtk3 VSID hotkeys file
 */
#define VHK_DEFAULT_NAME_VSID   VHK_PREFIX_VSID VHK_EXT


/** \brief  Accepted GDK modifiers for hotkeys
 *
 * This is required to avoid keys like NumLock showing up in the accelerators,
 * and sometimes GDK will pass along reserved bits (MOD27 etc).
 *
 * GDK_MOD1_MASK refers to Alt/Option.
 * GDK_MOD2_MASK refers to NumLock, so we filter it out.
 * GDK_META_MASK refers to the Command key on MacOS, doesn't appear to do
 * anything on Linux.
 * GDK_SUPER_MASK refers to the "Windows key" on PC keyboards. Since window
 * managers on Linux, and Windows itself, use this key for all sorts of things,
 * we filter it out.
 */
#ifdef MACOS_COMPILE
/* Command, Control, Option, Shift */
# define VHK_ACCEPTED_MODIFIERS \
    (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_META_MASK)
#else
/* Control, Alt, Shift */
# define VHK_ACCEPTED_MODIFIERS \
    (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK)
#endif


/** \brief  Modifier IDs
 */
typedef enum hotkeys_modifier_id_e {
    HOTKEYS_MOD_ID_ILLEGAL = -1,    /**< illegal modifier */
    HOTKEYS_MOD_ID_NONE,            /**< no modifer */
    HOTKEYS_MOD_ID_ALT,             /**< Alt */
    HOTKEYS_MOD_ID_COMMAND,         /**< Command (MacOS) */
    HOTKEYS_MOD_ID_CONTROL,         /**< Control */
    HOTKEYS_MOD_ID_HYPER,           /**< Hyper (MacOS) */
    HOTKEYS_MOD_ID_META,            /**< Meta, on MacOS GDK_META_MASK maps to
                                         Command */
    HOTKEYS_MOD_ID_OPTION,          /**< Option (MacOS), GDK_MOD1_MASK, same as
                                         Alt */
    HOTKEYS_MOD_ID_SHIFT,           /**< Shift */
    HOTKEYS_MOD_ID_SUPER            /**< Super ("Windows" key), could be Apple
                                         key on MacOS */
} hotkeys_modifier_id_t;


/** \brief  Parser modifier type
 *
 * The modifier IDs are there to allow dumping a hotkeys file with PC-specific
 * modifier names on Linux, BSD, Windows and MacOS-specific modifier names on
 * MacOS. So "<Control><Alt>X" would be dumped as "<Command><Option>X" on MacOS,
 * but the parser wouldn't care when reading back the file.
 */
typedef struct hotkeys_modifier_s {
    const char *            name;       /**< modifier name */
    hotkeys_modifier_id_t   id;         /**< modifier ID */
    GdkModifierType         mask;       /**< GDK modifier mask */
    const char *            mask_str;   /**< string form of macro, without the
                                             "GDK_" prefix or the "_MASK" suffix */
    const char *            utf8;       /**< used for hotkeys UI display */
} hotkeys_modifier_t;



int     ui_hotkeys_resources_init(void);
int     ui_hotkeys_cmdline_options_init(void);

void    ui_hotkeys_init(void);
void    ui_hotkeys_shutdown(void);

bool    ui_hotkeys_parse(const char *path);
bool    ui_hotkeys_export(const char *path);
void    ui_hotkeys_load_default(void);

char *  ui_hotkeys_get_hotkey_string_for_action(gint action_id);
const hotkeys_modifier_t *ui_hotkeys_get_modifier_list(void);

#endif

